/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpactiongroup.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpviewable.h"

#include "gimpactiongroup.h"
#include "gimpaction.h"
#include "gimpenumaction.h"
#include "gimppluginaction.h"
#include "gimpradioaction.h"
#include "gimpstringaction.h"
#include "gimptoggleaction.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_LABEL,
  PROP_ICON_NAME
};


static void   gimp_action_group_constructed   (GObject      *object);
static void   gimp_action_group_dispose       (GObject      *object);
static void   gimp_action_group_finalize      (GObject      *object);
static void   gimp_action_group_set_property  (GObject      *object,
                                               guint         prop_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void   gimp_action_group_get_property  (GObject      *object,
                                               guint         prop_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);


G_DEFINE_TYPE (GimpActionGroup, gimp_action_group, GTK_TYPE_ACTION_GROUP)

#define parent_class gimp_action_group_parent_class


static void
gimp_action_group_class_init (GimpActionGroupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_action_group_constructed;
  object_class->dispose      = gimp_action_group_dispose;
  object_class->finalize     = gimp_action_group_finalize;
  object_class->set_property = gimp_action_group_set_property;
  object_class->get_property = gimp_action_group_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  klass->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
gimp_action_group_init (GimpActionGroup *group)
{
}

static void
gimp_action_group_constructed (GObject *object)
{
  GimpActionGroup *group = GIMP_ACTION_GROUP (object);
  const gchar     *name;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_GIMP (group->gimp));

  name = gtk_action_group_get_name (GTK_ACTION_GROUP (object));

  if (name)
    {
      GimpActionGroupClass *group_class;
      GList                *list;

      group_class = GIMP_ACTION_GROUP_GET_CLASS (object);

      list = g_hash_table_lookup (group_class->groups, name);

      list = g_list_append (list, object);

      g_hash_table_replace (group_class->groups,
                            g_strdup (name), list);
    }
}

static void
gimp_action_group_dispose (GObject *object)
{
  const gchar *name = gtk_action_group_get_name (GTK_ACTION_GROUP (object));

  if (name)
    {
      GimpActionGroupClass *group_class;
      GList                *list;

      group_class = GIMP_ACTION_GROUP_GET_CLASS (object);

      list = g_hash_table_lookup (group_class->groups, name);

      if (list)
        {
          list = g_list_remove (list, object);

          if (list)
            g_hash_table_replace (group_class->groups,
                                  g_strdup (name), list);
          else
            g_hash_table_remove (group_class->groups, name);
        }
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_action_group_finalize (GObject *object)
{
  GimpActionGroup *group = GIMP_ACTION_GROUP (object);

  if (group->label)
    {
      g_free (group->label);
      group->label = NULL;
    }

  if (group->icon_name)
    {
      g_free (group->icon_name);
      group->icon_name = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_action_group_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpActionGroup *group = GIMP_ACTION_GROUP (object);

  switch (prop_id)
    {
    case PROP_GIMP:
      group->gimp = g_value_get_object (value);
      break;
    case PROP_LABEL:
      group->label = g_value_dup_string (value);
      break;
    case PROP_ICON_NAME:
      group->icon_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_action_group_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpActionGroup *group = GIMP_ACTION_GROUP (object);

  switch (prop_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, group->gimp);
      break;
    case PROP_LABEL:
      g_value_set_string (value, group->label);
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, group->icon_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
gimp_action_group_check_unique_action (GimpActionGroup *group,
				       const gchar     *action_name)
{
  if (G_UNLIKELY (gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                               action_name)))
    {
      g_warning ("Refusing to add non-unique action '%s' to action group '%s'",
	 	 action_name,
                 gtk_action_group_get_name (GTK_ACTION_GROUP (group)));
      return FALSE;
    }

  return TRUE;

}

/**
 * gimp_action_group_new:
 * @gimp:        the @Gimp instance this action group belongs to
 * @name:        the name of the action group.
 * @label:       the user visible label of the action group.
 * @icon_name:   the icon of the action group.
 * @user_data:   the user_data for #GtkAction callbacks.
 * @update_func: the function that will be called on
 *               gimp_action_group_update().
 *
 * Creates a new #GimpActionGroup object. The name of the action group
 * is used when associating <link linkend="Action-Accel">keybindings</link>
 * with the actions.
 *
 * Returns: the new #GimpActionGroup
 */
GimpActionGroup *
gimp_action_group_new (Gimp                      *gimp,
                       const gchar               *name,
                       const gchar               *label,
                       const gchar               *icon_name,
                       gpointer                   user_data,
                       GimpActionGroupUpdateFunc  update_func)
{
  GimpActionGroup *group;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  group = g_object_new (GIMP_TYPE_ACTION_GROUP,
                        "gimp",      gimp,
                        "name",      name,
                        "label",     label,
                        "icon-name", icon_name,
                        NULL);

  group->user_data   = user_data;
  group->update_func = update_func;

  return group;
}

GList *
gimp_action_groups_from_name (const gchar *name)
{
  GimpActionGroupClass *group_class;
  GList                *list;

  g_return_val_if_fail (name != NULL, NULL);

  group_class = g_type_class_ref (GIMP_TYPE_ACTION_GROUP);

  list = g_hash_table_lookup (group_class->groups, name);

  g_type_class_unref (group_class);

  return list;
}

void
gimp_action_group_update (GimpActionGroup *group,
                          gpointer         update_data)
{
  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  if (group->update_func)
    group->update_func (group, update_data);
}

void
gimp_action_group_add_actions (GimpActionGroup       *group,
			       const gchar           *msg_context,
                               const GimpActionEntry *entries,
                               guint                  n_entries)
{
  gint i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GimpAction  *action;
      const gchar *label;
      const gchar *tooltip = NULL;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        continue;

      if (msg_context)
        {
          label = g_dpgettext2 (NULL, msg_context, entries[i].label);

          if (entries[i].tooltip)
            tooltip = g_dpgettext2 (NULL, msg_context, entries[i].tooltip);
        }
      else
        {
          label   = gettext (entries[i].label);
          tooltip = gettext (entries[i].tooltip);
        }

      action = gimp_action_new (entries[i].name, label, tooltip,
                                entries[i].icon_name);

      if (entries[i].callback)
        g_signal_connect (action, "activate",
                          entries[i].callback,
                          group->user_data);

      gtk_action_group_add_action_with_accel (GTK_ACTION_GROUP (group),
                                              GTK_ACTION (action),
                                              entries[i].accelerator);

      if (entries[i].help_id)
        g_object_set_qdata_full (G_OBJECT (action), GIMP_HELP_ID,
                                 g_strdup (entries[i].help_id),
                                 (GDestroyNotify) g_free);

      g_object_unref (action);
    }
}

void
gimp_action_group_add_toggle_actions (GimpActionGroup             *group,
                                      const gchar                 *msg_context,
                                      const GimpToggleActionEntry *entries,
                                      guint                        n_entries)
{
  gint i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GtkToggleAction *action;
      const gchar     *label;
      const gchar     *tooltip = NULL;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        continue;

      if (msg_context)
        {
          label = g_dpgettext2 (NULL, msg_context, entries[i].label);

          if (entries[i].tooltip)
            tooltip = g_dpgettext2 (NULL, msg_context, entries[i].tooltip);
        }
      else
        {
          label   = gettext (entries[i].label);
          tooltip = gettext (entries[i].tooltip);
        }

      action = gimp_toggle_action_new (entries[i].name, label, tooltip,
                                       entries[i].icon_name);

      gtk_toggle_action_set_active (action, entries[i].is_active);

      if (entries[i].callback)
        g_signal_connect (action, "toggled",
                          entries[i].callback,
                          group->user_data);

      gtk_action_group_add_action_with_accel (GTK_ACTION_GROUP (group),
                                              GTK_ACTION (action),
                                              entries[i].accelerator);

      if (entries[i].help_id)
        g_object_set_qdata_full (G_OBJECT (action), GIMP_HELP_ID,
                                 g_strdup (entries[i].help_id),
                                 (GDestroyNotify) g_free);

      g_object_unref (action);
    }
}

GSList *
gimp_action_group_add_radio_actions (GimpActionGroup            *group,
                                     const gchar                *msg_context,
                                     const GimpRadioActionEntry *entries,
                                     guint                       n_entries,
                                     GSList                     *radio_group,
                                     gint                        value,
                                     GCallback                   callback)
{
  GtkRadioAction *first_action = NULL;
  gint            i;

  g_return_val_if_fail (GIMP_IS_ACTION_GROUP (group), NULL);

  for (i = 0; i < n_entries; i++)
    {
      GtkRadioAction *action;
      const gchar    *label;
      const gchar    *tooltip = NULL;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        continue;

      if (msg_context)
        {
          label = g_dpgettext2 (NULL, msg_context, entries[i].label);

          if (entries[i].tooltip)
            tooltip = g_dpgettext2 (NULL, msg_context, entries[i].tooltip);
        }
      else
        {
          label   = gettext (entries[i].label);
          tooltip = gettext (entries[i].tooltip);
        }

      action = gimp_radio_action_new (entries[i].name, label, tooltip,
                                      entries[i].icon_name,
                                      entries[i].value);

      if (i == 0)
        first_action = action;

      gtk_radio_action_set_group (action, radio_group);
      radio_group = gtk_radio_action_get_group (action);

      if (value == entries[i].value)
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);

      gtk_action_group_add_action_with_accel (GTK_ACTION_GROUP (group),
                                              GTK_ACTION (action),
                                              entries[i].accelerator);

      if (entries[i].help_id)
        g_object_set_qdata_full (G_OBJECT (action), GIMP_HELP_ID,
                                 g_strdup (entries[i].help_id),
                                 (GDestroyNotify) g_free);

      g_object_unref (action);
    }

  if (callback && first_action)
    g_signal_connect (first_action, "changed",
                      callback,
                      group->user_data);

  return radio_group;
}

void
gimp_action_group_add_enum_actions (GimpActionGroup           *group,
                                    const gchar               *msg_context,
                                    const GimpEnumActionEntry *entries,
                                    guint                      n_entries,
                                    GCallback                  callback)
{
  gint i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GimpEnumAction *action;
      const gchar    *label;
      const gchar    *tooltip = NULL;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        continue;

      if (msg_context)
        {
          label = g_dpgettext2 (NULL, msg_context, entries[i].label);

          if (entries[i].tooltip)
            tooltip = g_dpgettext2 (NULL, msg_context, entries[i].tooltip);
        }
      else
        {
          label   = gettext (entries[i].label);
          tooltip = gettext (entries[i].tooltip);
        }

      action = gimp_enum_action_new (entries[i].name, label, tooltip,
                                     entries[i].icon_name,
                                     entries[i].value,
                                     entries[i].value_variable);

      if (callback)
        g_signal_connect (action, "selected",
                          callback,
                          group->user_data);

      gtk_action_group_add_action_with_accel (GTK_ACTION_GROUP (group),
                                              GTK_ACTION (action),
                                              entries[i].accelerator);

      if (entries[i].help_id)
        g_object_set_qdata_full (G_OBJECT (action), GIMP_HELP_ID,
                                 g_strdup (entries[i].help_id),
                                 (GDestroyNotify) g_free);

      g_object_unref (action);
    }
}

void
gimp_action_group_add_string_actions (GimpActionGroup             *group,
                                      const gchar                 *msg_context,
                                      const GimpStringActionEntry *entries,
                                      guint                        n_entries,
                                      GCallback                    callback)
{
  gint i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GimpStringAction *action;
      const gchar      *label;
      const gchar      *tooltip = NULL;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        continue;

      if (msg_context)
        {
          label = g_dpgettext2 (NULL, msg_context, entries[i].label);

          if (entries[i].tooltip)
            tooltip = g_dpgettext2 (NULL, msg_context, entries[i].tooltip);
        }
      else
        {
          label   = gettext (entries[i].label);
          tooltip = gettext (entries[i].tooltip);
        }

      action = gimp_string_action_new (entries[i].name, label, tooltip,
                                       entries[i].icon_name,
                                       entries[i].value);

      if (callback)
        g_signal_connect (action, "selected",
                          callback,
                          group->user_data);

      gtk_action_group_add_action_with_accel (GTK_ACTION_GROUP (group),
                                              GTK_ACTION (action),
                                              entries[i].accelerator);

      if (entries[i].help_id)
        g_object_set_qdata_full (G_OBJECT (action), GIMP_HELP_ID,
                                 g_strdup (entries[i].help_id),
                                 (GDestroyNotify) g_free);

      g_object_unref (action);
    }
}

void
gimp_action_group_add_plug_in_actions (GimpActionGroup             *group,
                                       const GimpPlugInActionEntry *entries,
                                       guint                        n_entries,
                                       GCallback                    callback)
{
  gint i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GimpPlugInAction *action;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        continue;

      action = gimp_plug_in_action_new (entries[i].name,
                                        entries[i].label,
                                        entries[i].tooltip,
                                        entries[i].icon_name,
                                        entries[i].procedure);

      if (callback)
        g_signal_connect (action, "selected",
                          callback,
                          group->user_data);

      gtk_action_group_add_action_with_accel (GTK_ACTION_GROUP (group),
                                              GTK_ACTION (action),
                                              entries[i].accelerator);

      if (entries[i].help_id)
        g_object_set_qdata_full (G_OBJECT (action), GIMP_HELP_ID,
                                 g_strdup (entries[i].help_id),
                                 (GDestroyNotify) g_free);

      g_object_unref (action);
    }
}

void
gimp_action_group_activate_action (GimpActionGroup *group,
                                   const gchar     *action_name)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to activate action which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gtk_action_activate (action);
}

void
gimp_action_group_set_action_visible (GimpActionGroup *group,
                                      const gchar     *action_name,
                                      gboolean         visible)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set visibility of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gtk_action_set_visible (action, visible);
}

void
gimp_action_group_set_action_sensitive (GimpActionGroup *group,
                                        const gchar     *action_name,
                                        gboolean         sensitive)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set sensitivity of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gtk_action_set_sensitive (action, sensitive);
}

void
gimp_action_group_set_action_active (GimpActionGroup *group,
                                     const gchar     *action_name,
                                     gboolean         active)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set \"active\" of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  if (! GTK_IS_TOGGLE_ACTION (action))
    {
      g_warning ("%s: Unable to set \"active\" of action "
                 "which is not a GtkToggleAction: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
                                active ? TRUE : FALSE);
}

void
gimp_action_group_set_action_label (GimpActionGroup *group,
                                    const gchar     *action_name,
                                    const gchar     *label)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set label of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gtk_action_set_label (action, label);
}

void
gimp_action_group_set_action_tooltip (GimpActionGroup     *group,
                                      const gchar         *action_name,
                                      const gchar         *tooltip)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set tooltip of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gtk_action_set_tooltip (action, tooltip);
}

const gchar *
gimp_action_group_get_action_tooltip (GimpActionGroup     *group,
                                      const gchar         *action_name)
{
  GtkAction *action;

  g_return_val_if_fail (GIMP_IS_ACTION_GROUP (group), NULL);
  g_return_val_if_fail (action_name != NULL, NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to get tooltip of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return NULL;
    }

  return gtk_action_get_tooltip (action);
}

void
gimp_action_group_set_action_color (GimpActionGroup *group,
                                    const gchar     *action_name,
                                    const GimpRGB   *color,
                                    gboolean         set_label)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set color of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  if (! GIMP_IS_ACTION (action))
    {
      g_warning ("%s: Unable to set \"color\" of action "
                 "which is not a GimpAction: %s",
                 G_STRFUNC, action_name);
      return;
    }

  if (set_label)
    {
      gchar *label;

      if (color)
        label = g_strdup_printf (_("RGBA (%0.3f, %0.3f, %0.3f, %0.3f)"),
                                 color->r, color->g, color->b, color->a);
      else
        label = g_strdup (_("(none)"));

      g_object_set (action,
                    "color", color,
                    "label", label,
                    NULL);
      g_free (label);
    }
  else
    {
      g_object_set (action, "color", color, NULL);
    }
}

void
gimp_action_group_set_action_viewable (GimpActionGroup *group,
                                       const gchar     *action_name,
                                       GimpViewable    *viewable)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);
  g_return_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable));

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set viewable of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  if (! GIMP_IS_ACTION (action))
    {
      g_warning ("%s: Unable to set \"viewable\" of action "
                 "which is not a GimpAction: %s",
                 G_STRFUNC, action_name);
      return;
    }

  g_object_set (action, "viewable", viewable, NULL);
}

void
gimp_action_group_set_action_hide_empty (GimpActionGroup *group,
                                         const gchar     *action_name,
                                         gboolean         hide_empty)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set \"hide-if-empty\" of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  g_object_set (action, "hide-if-empty", hide_empty ? TRUE : FALSE, NULL);
}

void
gimp_action_group_set_action_always_show_image (GimpActionGroup *group,
                                                const gchar     *action_name,
                                                gboolean         always_show_image)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set \"always-show-image\" of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gtk_action_set_always_show_image (action, always_show_image);
}
