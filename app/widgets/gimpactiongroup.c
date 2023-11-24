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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "plug-in/gimppluginprocedure.h"

#include "gimpaction.h"
#include "gimpactiongroup.h"
#include "gimpactionimpl.h"
#include "gimpdoubleaction.h"
#include "gimpenumaction.h"
#include "gimpprocedureaction.h"
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


static void        gimp_action_group_iface_init        (GActionGroupInterface *iface);

static void        gimp_action_group_constructed       (GObject               *object);
static void        gimp_action_group_dispose           (GObject               *object);
static void        gimp_action_group_finalize          (GObject               *object);
static void        gimp_action_group_set_property      (GObject               *object,
                                                        guint                  prop_id,
                                                        const GValue          *value,
                                                        GParamSpec            *pspec);
static void        gimp_action_group_get_property      (GObject               *object,
                                                        guint                  prop_id,
                                                        GValue                *value,
                                                        GParamSpec            *pspec);

static void        gimp_action_group_activate_action   (GActionGroup          *group,
                                                        const gchar           *action_name,
                                                        GVariant              *parameter G_GNUC_UNUSED);
static gboolean    gimp_action_group_has_action        (GActionGroup          *group,
                                                        const gchar           *action_name);
static gchar    ** gimp_action_group_list_action_names (GActionGroup          *group);


G_DEFINE_TYPE_WITH_CODE (GimpActionGroup, gimp_action_group, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION_GROUP, gimp_action_group_iface_init))

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
  group->actions = NULL;
}

static void
gimp_action_group_iface_init (GActionGroupInterface *iface)
{
  iface->activate_action = gimp_action_group_activate_action;
  iface->has_action      = gimp_action_group_has_action;
  iface->list_actions    = gimp_action_group_list_action_names;
}

static void
gimp_action_group_constructed (GObject *object)
{
  GimpActionGroup *group = GIMP_ACTION_GROUP (object);
  const gchar     *name;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (group->gimp));

  name = gimp_action_group_get_name (group);

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
  GimpActionGroup *group = GIMP_ACTION_GROUP (object);
  const gchar     *name  = gimp_action_group_get_name (group);

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

  g_list_free_full (g_steal_pointer (&group->actions), g_object_unref);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_action_group_finalize (GObject *object)
{
  GimpActionGroup *group = GIMP_ACTION_GROUP (object);

  g_clear_pointer (&group->label,     g_free);
  g_clear_pointer (&group->icon_name, g_free);

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


/* Virtual methods */

static void
gimp_action_group_activate_action (GActionGroup *group,
                                   const gchar  *action_name,
                                   GVariant     *parameter G_GNUC_UNUSED)
{
  GimpAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gimp_action_group_get_action (GIMP_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to activate action which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gimp_action_activate (action);
}

static gboolean
gimp_action_group_has_action (GActionGroup *group,
                              const gchar     *action_name)
{
  return (gimp_action_group_get_action (GIMP_ACTION_GROUP (group), action_name) != NULL);
}

static gchar **
gimp_action_group_list_action_names (GActionGroup *group)
{
  GimpActionGroup  *action_group = GIMP_ACTION_GROUP (group);
  gchar           **actions;
  GList            *iter;
  gint              i;

  actions = g_new0 (gchar *, g_list_length (action_group->actions) + 1);
  for (i = 0, iter = action_group->actions; iter; i++, iter = iter->next)
    actions[i] = g_strdup (gimp_action_get_name (iter->data));

  return actions;
}

/* Private functions */

static gboolean
gimp_action_group_check_unique_action (GimpActionGroup *group,
                                       const gchar     *action_name)
{
  if (G_UNLIKELY (gimp_action_group_get_action (group, action_name)))
    {
      g_printerr ("Refusing to add non-unique action '%s' to action group '%s'\n",
                  action_name, gimp_action_group_get_name (group));
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

const gchar *
gimp_action_group_get_name (GimpActionGroup *group)
{
  return gimp_object_get_name (GIMP_OBJECT (group));
}

void
gimp_action_group_add_action (GimpActionGroup *group,
                              GimpAction      *action)
{
  gimp_action_group_add_action_with_accel (group, action, NULL);
}

void
gimp_action_group_add_action_with_accel (GimpActionGroup    *group,
                                         GimpAction         *action,
                                         const gchar * const*accelerators)
{
  g_return_if_fail (GIMP_IS_ACTION (action));

  if (! g_list_find (group->actions, action))
    {
      group->actions = g_list_prepend (group->actions, g_object_ref (action));

      g_signal_emit_by_name (group, "action-added", gimp_action_get_name (action));

      if ((accelerators != NULL && accelerators[0] != NULL &&
           g_strcmp0 (accelerators[0], "") != 0))
        {
          guint       i = 0;
          GdkDisplay *display;
          GdkKeymap  *keymap;
          gchar      *accel_strs[] = { NULL, NULL, NULL, NULL};

          display = gdk_display_get_default ();
          keymap = gdk_keymap_get_for_display (display);

          while (accelerators[i] != NULL)
            {
              /**
               * Shifted numeric key accelerators ("<shift>0" .. "<shift>9") do
               * not work with GTK3 so the following code converts these
               * accelerators to the shifted character (or the un-shifted
               * character in the case of an azerty keyboard for example) that
               * relates to the selected numeric character. This takes into
               * account the keyboard layout that is being used when GIMP starts.
               * This means that the appropriate characters will be shown for the
               * shortcuts in the menus and keyboard preferences.
               **/
              guint           accelerator_key;
              GdkModifierType accelerator_modifier;
              gboolean        accelerator_string_set;

              accelerator_string_set = FALSE;

              gtk_accelerator_parse (accelerators[i],
                                     &accelerator_key,
                                     &accelerator_modifier);

              if ((accelerator_key >= '0') &&
                  (accelerator_key <= '9') &&
                  (accelerator_modifier == GDK_SHIFT_MASK))
                {
                  gboolean      result;
                  gint          count;
                  GdkKeymapKey  key;
                  GdkKeymapKey *keys = NULL;
                  guint         non_number_keyval;

                  result = gdk_keymap_get_entries_for_keyval (keymap,
                                                              accelerator_key,
                                                              &keys,
                                                              &count);
                  if (result && (count > 0))
                    {
                      key.keycode = keys[0].keycode;
                      key.group   = 0;
                      key.level   = 1;

                      non_number_keyval = gdk_keymap_lookup_key (keymap, &key);

                      if (non_number_keyval == accelerator_key)
                        {
                          /**
                           * the number shifted is the number - assume keyboard
                           * such as azerty where the numbers are on the shifted
                           * key and the other characters are obtained without
                           * the shift
                           **/
                          key.level = 0;
                          non_number_keyval = gdk_keymap_lookup_key (keymap, &key);
                        }
                      accel_strs[i] = g_strdup (gdk_keyval_name (non_number_keyval));
                      accelerator_string_set = TRUE;
                    }
                  g_free (keys);
                }

              if (! accelerator_string_set)
                accel_strs[i] = g_strdup (accelerators[i]);

              i++;
            }
          gimp_action_set_default_accels (action, (const gchar **) accel_strs);

          /* free up to 3 accelerator strings (4th entry is always NULL) */
          for (guint i = 0; i < 3; i++)
            g_free (accel_strs[i]);
        }

      gimp_action_set_group (action, group);
    }
}

void
gimp_action_group_remove_action (GimpActionGroup *group,
                                 GimpAction      *action)
{
  if (g_list_find (group->actions, action))
    {
      group->actions = g_list_remove (group->actions, action);
      g_signal_emit_by_name (group, "action-removed", gimp_action_get_name (action));
      g_object_unref (action);
    }
}

GimpAction *
gimp_action_group_get_action (GimpActionGroup *group,
                              const gchar     *action_name)
{
  for (GList *iter = group->actions; iter; iter = iter->next)
    {
      GimpAction *action = iter->data;

      if (g_strcmp0 (gimp_action_get_name (action), action_name) == 0)
        return action;
    }

  return NULL;
}

GList *
gimp_action_group_list_actions (GimpActionGroup *group)
{
  return g_list_copy (group->actions);
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
  GimpContext *context = gimp_get_user_context (group->gimp);
  gint         i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GimpAction  *action;
      const gchar *label;
      const gchar *short_label = NULL;
      const gchar *tooltip     = NULL;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        continue;

      if (msg_context)
        {
          label = g_dpgettext2 (NULL, msg_context, entries[i].label);

          if (entries[i].short_label)
            short_label = g_dpgettext2 (NULL, msg_context, entries[i].short_label);

          if (entries[i].tooltip)
            tooltip = g_dpgettext2 (NULL, msg_context, entries[i].tooltip);
        }
      else
        {
          label = gettext (entries[i].label);

          if (entries[i].short_label)
            short_label = gettext (entries[i].short_label);

          if (entries[i].tooltip)
            tooltip = gettext (entries[i].tooltip);
        }

      action = gimp_action_impl_new (entries[i].name,
                                     label, short_label, tooltip,
                                     entries[i].icon_name,
                                     entries[i].help_id, context);

      if (entries[i].callback)
        g_signal_connect (action, "activate",
                          G_CALLBACK (entries[i].callback),
                          group->user_data);

      gimp_action_group_add_action_with_accel (group, GIMP_ACTION (action),
                                               entries[i].accelerator);

      g_object_unref (action);
    }
}

void
gimp_action_group_add_toggle_actions (GimpActionGroup             *group,
                                      const gchar                 *msg_context,
                                      const GimpToggleActionEntry *entries,
                                      guint                        n_entries)
{
  GimpContext *context = gimp_get_user_context (group->gimp);
  gint         i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GimpAction  *action;
      const gchar *label;
      const gchar *short_label = NULL;
      const gchar *tooltip     = NULL;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        continue;

      if (msg_context)
        {
          label = g_dpgettext2 (NULL, msg_context, entries[i].label);

          if (entries[i].short_label)
            short_label = g_dpgettext2 (NULL, msg_context, entries[i].short_label);

          if (entries[i].tooltip)
            tooltip = g_dpgettext2 (NULL, msg_context, entries[i].tooltip);
        }
      else
        {
          label = gettext (entries[i].label);

          if (entries[i].short_label)
            short_label = gettext (entries[i].short_label);

          if (entries[i].tooltip)
            tooltip = gettext (entries[i].tooltip);
        }

      action = gimp_toggle_action_new (entries[i].name,
                                       label, short_label, tooltip,
                                       entries[i].icon_name,
                                       entries[i].help_id, context);

      gimp_toggle_action_set_active (GIMP_TOGGLE_ACTION (action),
                                     entries[i].is_active);

      if (entries[i].callback)
        g_signal_connect (action, "change-state",
                          G_CALLBACK (entries[i].callback),
                          group->user_data);

      gimp_action_group_add_action_with_accel (group, GIMP_ACTION (action),
                                               entries[i].accelerator);

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
                                     GimpActionCallback          callback)
{
  GimpAction  *first_action = NULL;
  GimpContext *context      = gimp_get_user_context (group->gimp);
  gint         i;

  g_return_val_if_fail (GIMP_IS_ACTION_GROUP (group), NULL);

  for (i = 0; i < n_entries; i++)
    {
      GimpAction  *action;
      const gchar *label;
      const gchar *short_label = NULL;
      const gchar *tooltip     = NULL;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        continue;

      if (msg_context)
        {
          label = g_dpgettext2 (NULL, msg_context, entries[i].label);

          if (entries[i].short_label)
            short_label = g_dpgettext2 (NULL, msg_context, entries[i].short_label);

          if (entries[i].tooltip)
            tooltip = g_dpgettext2 (NULL, msg_context, entries[i].tooltip);
        }
      else
        {
          label = gettext (entries[i].label);

          if (entries[i].short_label)
            short_label = gettext (entries[i].short_label);

          if (entries[i].tooltip)
            tooltip = gettext (entries[i].tooltip);
        }

      action = gimp_radio_action_new (entries[i].name,
                                      label, short_label, tooltip,
                                      entries[i].icon_name,
                                      entries[i].help_id,
                                      entries[i].value, context);

      if (i == 0)
        first_action = action;

      gimp_radio_action_set_group (GIMP_RADIO_ACTION (action), radio_group);
      radio_group = gimp_radio_action_get_group (GIMP_RADIO_ACTION (action));

      if (value == entries[i].value)
        gimp_toggle_action_set_active (GIMP_TOGGLE_ACTION (action), TRUE);

      gimp_action_group_add_action_with_accel (group, action, entries[i].accelerator);

      g_object_unref (action);
    }

  if (callback && first_action)
    g_signal_connect (first_action, "change-state",
                      G_CALLBACK (callback),
                      group->user_data);

  return radio_group;
}

void
gimp_action_group_add_enum_actions (GimpActionGroup           *group,
                                    const gchar               *msg_context,
                                    const GimpEnumActionEntry *entries,
                                    guint                      n_entries,
                                    GimpActionCallback         callback)
{
  GimpContext *context = gimp_get_user_context (group->gimp);
  gint         i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GimpEnumAction *action;
      const gchar    *label;
      const gchar    *short_label = NULL;
      const gchar    *tooltip     = NULL;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        continue;

      if (msg_context)
        {
          label = g_dpgettext2 (NULL, msg_context, entries[i].label);

          if (entries[i].short_label)
            short_label = g_dpgettext2 (NULL, msg_context, entries[i].short_label);

          if (entries[i].tooltip)
            tooltip = g_dpgettext2 (NULL, msg_context, entries[i].tooltip);
        }
      else
        {
          label = gettext (entries[i].label);

          if (entries[i].short_label)
            short_label = gettext (entries[i].short_label);

          if (entries[i].tooltip)
            tooltip = gettext (entries[i].tooltip);
        }

      action = gimp_enum_action_new (entries[i].name,
                                     label, short_label, tooltip,
                                     entries[i].icon_name,
                                     entries[i].help_id,
                                     entries[i].value,
                                     entries[i].value_variable,
                                     context);

      if (callback)
        g_signal_connect (action, "activate",
                          G_CALLBACK (callback),
                          group->user_data);

      gimp_action_group_add_action_with_accel (group, GIMP_ACTION (action),
                                               entries[i].accelerator);

      g_object_unref (action);
    }
}

void
gimp_action_group_add_string_actions (GimpActionGroup             *group,
                                      const gchar                 *msg_context,
                                      const GimpStringActionEntry *entries,
                                      guint                        n_entries,
                                      GimpActionCallback           callback)
{
  GimpContext *context = gimp_get_user_context (group->gimp);
  gint         i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GimpStringAction *action;
      const gchar      *label;
      const gchar      *short_label = NULL;
      const gchar      *tooltip     = NULL;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        continue;

      if (msg_context)
        {
          label = g_dpgettext2 (NULL, msg_context, entries[i].label);

          if (entries[i].short_label)
            short_label = g_dpgettext2 (NULL, msg_context, entries[i].short_label);

          if (entries[i].tooltip)
            tooltip = g_dpgettext2 (NULL, msg_context, entries[i].tooltip);
        }
      else
        {
          label = gettext (entries[i].label);

          if (entries[i].short_label)
            short_label = gettext (entries[i].short_label);

          if (entries[i].tooltip)
            tooltip = gettext (entries[i].tooltip);
        }

      action = gimp_string_action_new (entries[i].name,
                                       label, short_label, tooltip,
                                       entries[i].icon_name,
                                       entries[i].help_id,
                                       entries[i].value, context);
      gimp_action_group_add_action_with_accel (group, GIMP_ACTION (action),
                                               entries[i].accelerator);

      if (callback)
        g_signal_connect (action, "activate",
                          G_CALLBACK (callback),
                          group->user_data);

      g_object_unref (action);
    }
}

void
gimp_action_group_add_double_actions (GimpActionGroup             *group,
                                      const gchar                 *msg_context,
                                      const GimpDoubleActionEntry *entries,
                                      guint                        n_entries,
                                      GimpActionCallback           callback)
{
  GimpContext *context = gimp_get_user_context (group->gimp);
  gint         i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GimpDoubleAction *action;
      const gchar      *label;
      const gchar      *short_label = NULL;
      const gchar      *tooltip     = NULL;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        continue;

      if (msg_context)
        {
          label = g_dpgettext2 (NULL, msg_context, entries[i].label);

          if (entries[i].short_label)
            short_label = g_dpgettext2 (NULL, msg_context, entries[i].short_label);

          if (entries[i].tooltip)
            tooltip = g_dpgettext2 (NULL, msg_context, entries[i].tooltip);
        }
      else
        {
          label = gettext (entries[i].label);

          if (entries[i].short_label)
            short_label = gettext (entries[i].short_label);

          if (entries[i].tooltip)
            tooltip = gettext (entries[i].tooltip);
        }

      action = gimp_double_action_new (entries[i].name,
                                       label, short_label, tooltip,
                                       entries[i].icon_name,
                                       entries[i].help_id,
                                       entries[i].value, context);

      if (callback)
        g_signal_connect (action, "activate",
                          G_CALLBACK (callback),
                          group->user_data);

      gimp_action_group_add_action_with_accel (group, GIMP_ACTION (action),
                                               entries[i].accelerator);

      g_object_unref (action);
    }
}

void
gimp_action_group_add_procedure_actions (GimpActionGroup                *group,
                                         const GimpProcedureActionEntry *entries,
                                         guint                           n_entries,
                                         GimpActionCallback              callback)
{
  GimpContext *context = gimp_get_user_context (group->gimp);
  gint         i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GimpProcedureAction *action;

      if (! gimp_action_group_check_unique_action (group, entries[i].name))
        {
          if (entries[i].procedure != NULL &&
              GIMP_IS_PLUG_IN_PROCEDURE (entries[i].procedure))
            {
              GFile *file;

              file = gimp_plug_in_procedure_get_file (GIMP_PLUG_IN_PROCEDURE (entries[i].procedure));

              /* Unlike other existence checks, this is not a program error (hence
               * no WARNINGs nor CRITICALs). It is more likely a third-party
               * plug-in bug, in particular 2 plug-ins with procedure name
               * clashing with each other.
               * Let's warn to help plug-in developers to debug their issue.
               */
              gimp_message (group->gimp, NULL, GIMP_MESSAGE_WARNING,
                            "Discarded action '%s' was registered in plug-in: '%s'\n",
                            entries[i].name, g_file_peek_path (file));
            }
          continue;
        }

      action = gimp_procedure_action_new (entries[i].name,
                                          entries[i].label,
                                          entries[i].tooltip,
                                          entries[i].icon_name,
                                          entries[i].help_id,
                                          entries[i].procedure,
                                          context);

      if (callback)
        g_signal_connect (action, "activate",
                          G_CALLBACK (callback),
                          group->user_data);

      gimp_action_group_add_action_with_accel (group, GIMP_ACTION (action),
                                               (const gchar *[]) { entries[i].accelerator, NULL });

      g_object_unref (action);
    }
}

/**
 * gimp_action_group_remove_action_and_accel:
 * @group:  the #GimpActionGroup to which @action belongs.
 * @action: the #GimpAction.
 *
 * This function removes @action from @group and clean any
 * accelerator this action may have set.
 * If you wish to only remove the action from the group, use
 * gimp_action_group_remove_action() instead.
 */
void
gimp_action_group_remove_action_and_accel (GimpActionGroup *group,
                                           GimpAction      *action)
{
  gimp_action_set_accels (action, (const char*[]) { NULL });
  gimp_action_group_remove_action (group, action);
}

void
gimp_action_group_set_action_visible (GimpActionGroup *group,
                                      const gchar     *action_name,
                                      gboolean         visible)
{
  GimpAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gimp_action_group_get_action (group, action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set visibility of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gimp_action_set_visible (action, visible);
}

void
gimp_action_group_set_action_sensitive (GimpActionGroup *group,
                                        const gchar     *action_name,
                                        gboolean         sensitive,
                                        const gchar     *reason)
{
  GimpAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gimp_action_group_get_action (group, action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set sensitivity of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gimp_action_set_sensitive (action, sensitive, reason);
}

void
gimp_action_group_set_action_active (GimpActionGroup *group,
                                     const gchar     *action_name,
                                     gboolean         active)
{
  GimpAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gimp_action_group_get_action (group, action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set \"active\" of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  if (GIMP_IS_TOGGLE_ACTION (action))
    gimp_toggle_action_set_active (GIMP_TOGGLE_ACTION (action),
                                   active ? TRUE : FALSE);
  else
    g_warning ("%s: Unable to set \"active\" of action "
               "which is not a GimpToggleAction: %s",
               G_STRFUNC, action_name);
}

void
gimp_action_group_set_action_label (GimpActionGroup *group,
                                    const gchar     *action_name,
                                    const gchar     *label)
{
  GimpAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gimp_action_group_get_action (group, action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set label of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gimp_action_set_label (action, label);
}

void
gimp_action_group_set_action_pixbuf (GimpActionGroup *group,
                                     const gchar     *action_name,
                                     GdkPixbuf       *pixbuf)
{
  GimpAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gimp_action_group_get_action (group, action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set pixbuf of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gimp_action_set_gicon (action, G_ICON (pixbuf));
}


void
gimp_action_group_set_action_tooltip (GimpActionGroup     *group,
                                      const gchar         *action_name,
                                      const gchar         *tooltip)
{
  GimpAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gimp_action_group_get_action (group, action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set tooltip of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

  gimp_action_set_tooltip (action, tooltip);
}

const gchar *
gimp_action_group_get_action_tooltip (GimpActionGroup     *group,
                                      const gchar         *action_name)
{
  GimpAction *action;

  g_return_val_if_fail (GIMP_IS_ACTION_GROUP (group), NULL);
  g_return_val_if_fail (action_name != NULL, NULL);

  action = gimp_action_group_get_action (group, action_name);

  if (! action)
    {
      g_warning ("%s: Unable to get tooltip of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return NULL;
    }

  return gimp_action_get_tooltip (action);
}

void
gimp_action_group_set_action_color (GimpActionGroup *group,
                                    const gchar     *action_name,
                                    GeglColor       *color,
                                    gboolean         set_label)
{
  GimpAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gimp_action_group_get_action (group, action_name);

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
        {
          gfloat rgba[4];

          gegl_color_get_pixel (color, babl_format ("R'G'B'A float"), rgba);

          label = g_strdup_printf (_("sRGB+A (%0.3f, %0.3f, %0.3f, %0.3f)"),
                                   rgba[0], rgba[1], rgba[2], rgba[3]);
        }
      else
        {
          label = g_strdup (_("(none)"));
        }

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
  GimpAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);
  g_return_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable));

  action = gimp_action_group_get_action (group, action_name);

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
  GimpAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gimp_action_group_get_action (group, action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set \"hide-if-empty\" of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

#if 0
  /* TODO GAction: currently a no-op. Did we actually ever use this property of
   * GtkAction?
   */
  g_object_set (action, "hide-if-empty", hide_empty ? TRUE : FALSE, NULL);
#endif
}

void
gimp_action_group_set_action_always_show_image (GimpActionGroup *group,
                                                const gchar     *action_name,
                                                gboolean         always_show_image)
{
  GimpAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gimp_action_group_get_action (group, action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set \"always-show-image\" of action "
                 "which doesn't exist: %s",
                 G_STRFUNC, action_name);
      return;
    }

#if 0
  /* TODO GAction: currently a no-op, though I think it was already a no-op to
   * us, at least for proxy images. We could still use this for other menu items
   * and button icons. Let's investigate further later to decide whether we
   * should get rid of all related code or instead make it work as expected.
   */
  gtk_action_set_always_show_image ((GtkAction *) action, TRUE);
#endif
}
