/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpactiongroup.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimpactiongroup.h"
#include "gimpenumaction.h"
#include "gimpstringaction.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_TRANSLATION_DOMAIN
};


static void   gimp_action_group_init         (GimpActionGroup      *group);
static void   gimp_action_group_class_init   (GimpActionGroupClass *klass);

static void   gimp_action_group_finalize     (GObject              *object);
static void   gimp_action_group_set_property (GObject              *object,
                                              guint                 prop_id,
                                              const GValue         *value,
                                              GParamSpec           *pspec);
static void   gimp_action_group_get_property (GObject              *object,
                                              guint                 prop_id,
                                              GValue               *value,
                                              GParamSpec           *pspec);


static GtkActionGroupClass *parent_class = NULL;


GType
gimp_action_group_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GimpActionGroupClass),
	NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_action_group_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpActionGroup),
        0, /* n_preallocs */
        (GInstanceInitFunc) gimp_action_group_init,
      };

      type = g_type_register_static (GTK_TYPE_ACTION_GROUP,
                                     "GimpActionGroup",
				     &type_info, 0);
    }

  return type;
}

static void
gimp_action_group_class_init (GimpActionGroupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_action_group_finalize;
  object_class->set_property = gimp_action_group_set_property;
  object_class->get_property = gimp_action_group_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_TRANSLATION_DOMAIN,
                                   g_param_spec_string ("translation-domain",
                                                        NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE));

}

static void
gimp_action_group_init (GimpActionGroup *group)
{
  group->translation_domain = NULL;
}

static void
gimp_action_group_finalize (GObject *object)
{
  GimpActionGroup *group = GIMP_ACTION_GROUP (object);

  if (group->translation_domain)
    {
      g_free (group->translation_domain);
      group->translation_domain = NULL;
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
    case PROP_TRANSLATION_DOMAIN:
      g_free (group->translation_domain);
      group->translation_domain = g_value_dup_string (value);
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
    case PROP_TRANSLATION_DOMAIN:
      g_value_set_string (value, group->translation_domain);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * gimp_action_group_new:
 * @gimp: the @Gimp instance this action group belongs to
 * @name: the name of the action group.
 *
 * Creates a new #GimpActionGroup object. The name of the action group
 * is used when associating <link linkend="Action-Accel">keybindings</link>
 * with the actions.
 *
 * Returns: the new #GimpActionGroup
 */
GimpActionGroup *
gimp_action_group_new (Gimp        *gimp,
                       const gchar *name)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (GIMP_TYPE_ACTION_GROUP,
                       "gimp", gimp,
                       "name", name,
                       NULL);
}

void
gimp_action_group_add_actions (GimpActionGroup *group,
                               GimpActionEntry *entries,
                               guint            n_entries,
                               gpointer         user_data)
{
  gint i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GtkAction   *action;
      const gchar *label;
      const gchar *tooltip;

      label   = dgettext (group->translation_domain, entries[i].label);
      tooltip = dgettext (group->translation_domain, entries[i].tooltip);

      action = gtk_action_new (entries[i].name, label, tooltip,
			       entries[i].stock_id);

      if (entries[i].callback)
        g_signal_connect (action, "activate",
                          entries[i].callback,
                          user_data);

      gtk_action_group_add_action_with_accel (GTK_ACTION_GROUP (group),
					      action,
					      entries[i].accelerator);
      g_object_unref (action);
    }
}

void
gimp_action_group_add_toggle_actions (GimpActionGroup       *group,
                                      GimpToggleActionEntry *entries,
                                      guint                  n_entries,
                                      gpointer               user_data)
{
  gint i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GtkToggleAction *action;
      const gchar     *label;
      const gchar     *tooltip;

      label   = dgettext (group->translation_domain, entries[i].label);
      tooltip = dgettext (group->translation_domain, entries[i].tooltip);

      action = gtk_toggle_action_new (entries[i].name, label, tooltip,
				      entries[i].stock_id);

      gtk_toggle_action_set_active (action, entries[i].is_active);

      if (entries[i].callback)
        g_signal_connect (action, "toggled",
                          entries[i].callback,
                          user_data);

      gtk_action_group_add_action_with_accel (GTK_ACTION_GROUP (group),
					      GTK_ACTION (action),
					      entries[i].accelerator);
      g_object_unref (action);
    }
}

void
gimp_action_group_add_radio_actions (GimpActionGroup      *group,
                                     GimpRadioActionEntry *entries,
                                     guint                 n_entries,
                                     gint                  value,
                                     GCallback             callback,
                                     gpointer              user_data)
{
  GtkRadioAction *first_action = NULL;
  GSList         *radio_group  = NULL;
  gint            i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GtkRadioAction *action;
      const gchar    *label;
      const gchar    *tooltip;

      label   = dgettext (group->translation_domain, entries[i].label);
      tooltip = dgettext (group->translation_domain, entries[i].tooltip);

      action = gtk_radio_action_new (entries[i].name, label, tooltip,
				     entries[i].stock_id,
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
      g_object_unref (action);
    }

  if (callback && first_action)
    g_signal_connect (first_action, "changed",
                      callback,
                      user_data);
}

void
gimp_action_group_add_enum_actions (GimpActionGroup     *group,
                                    GimpEnumActionEntry *entries,
                                    guint                n_entries,
                                    GCallback            callback,
                                    gpointer             user_data)
{
  gint i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GimpEnumAction *action;
      const gchar    *label;
      const gchar    *tooltip;

      label   = dgettext (group->translation_domain, entries[i].label);
      tooltip = dgettext (group->translation_domain, entries[i].tooltip);

      action = gimp_enum_action_new (entries[i].name, label, tooltip,
				     entries[i].stock_id,
				     entries[i].value);

      if (callback)
        g_signal_connect (action, "selected",
                          callback,
                          user_data);

      gtk_action_group_add_action_with_accel (GTK_ACTION_GROUP (group),
					      GTK_ACTION (action),
					      entries[i].accelerator);
      g_object_unref (action);
    }
}

void
gimp_action_group_add_string_actions (GimpActionGroup       *group,
                                      GimpStringActionEntry *entries,
                                      guint                  n_entries,
                                      GCallback              callback,
                                      gpointer               user_data)
{
  gint i;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));

  for (i = 0; i < n_entries; i++)
    {
      GimpStringAction *action;
      const gchar      *label;
      const gchar      *tooltip;

      label   = dgettext (group->translation_domain, entries[i].label);
      tooltip = dgettext (group->translation_domain, entries[i].tooltip);

      action = gimp_string_action_new (entries[i].name, label, tooltip,
                                       entries[i].stock_id,
                                       entries[i].value);

      if (callback)
        g_signal_connect (action, "selected",
                          callback,
                          user_data);

      gtk_action_group_add_action_with_accel (GTK_ACTION_GROUP (group),
					      GTK_ACTION (action),
					      entries[i].accelerator);
      g_object_unref (action);
    }
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
                 G_STRLOC, action_name);
      return;
    }

  g_object_set (action, "visible", visible, NULL);
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
                 G_STRLOC, action_name);
      return;
    }

  g_object_set (action, "sensitive", sensitive, NULL);
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
                 G_STRLOC, action_name);
      return;
    }

  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), active);
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
                 G_STRLOC, action_name);
      return;
    }

  g_object_set (action, "label", label, NULL);
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
                 G_STRLOC, action_name);
      return;
    }
}

void
gimp_action_group_set_action_important (GimpActionGroup *group,
                                        const gchar     *action_name,
                                        gboolean         is_important)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (action_name != NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (! action)
    {
      g_warning ("%s: Unable to set \"is_important\" of action "
                 "which doesn't exist: %s",
                 G_STRLOC, action_name);
      return;
    }

  g_object_set (action, "is-important", is_important, NULL);
}
