/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpstringaction.c
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "gimpaction.h"
#include "gimpaction-history.h"
#include "gimpstringaction.h"


enum
{
  PROP_0,
  PROP_VALUE
};


static void   gimp_string_action_g_action_iface_init (GActionInterface *iface);

static void   gimp_string_action_finalize            (GObject          *object);
static void   gimp_string_action_set_property        (GObject          *object,
                                                      guint             prop_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void   gimp_string_action_get_property        (GObject          *object,
                                                      guint             prop_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static void   gimp_string_action_activate            (GAction          *action,
                                                      GVariant         *parameter);


G_DEFINE_TYPE_WITH_CODE (GimpStringAction, gimp_string_action, GIMP_TYPE_ACTION_IMPL,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, gimp_string_action_g_action_iface_init))

#define parent_class gimp_string_action_parent_class


static void
gimp_string_action_class_init (GimpStringActionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_string_action_finalize;
  object_class->set_property = gimp_string_action_set_property;
  object_class->get_property = gimp_string_action_get_property;

  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_string ("value",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_string_action_g_action_iface_init (GActionInterface *iface)
{
  iface->activate = gimp_string_action_activate;
}

static void
gimp_string_action_init (GimpStringAction *action)
{
}

static void
gimp_string_action_finalize (GObject *object)
{
  GimpStringAction *action = GIMP_STRING_ACTION (object);

  g_clear_pointer (&action->value, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_string_action_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpStringAction *action = GIMP_STRING_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_value_set_string (value, action->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_string_action_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpStringAction *action = GIMP_STRING_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_free (action->value);
      action->value = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

GimpStringAction *
gimp_string_action_new (const gchar *name,
                        const gchar *label,
                        const gchar *short_label,
                        const gchar *tooltip,
                        const gchar *icon_name,
                        const gchar *help_id,
                        const gchar *value,
                        GimpContext *context)
{
  GimpStringAction *action;

  action = g_object_new (GIMP_TYPE_STRING_ACTION,
                         "name",        name,
                         "label",       label,
                         "short-label", short_label,
                         "tooltip",     tooltip,
                         "icon-name",   icon_name,
                         "value",       value,
                         "context",     context,
                         NULL);

  gimp_action_set_help_id (GIMP_ACTION (action), help_id);

  return action;
}

static void
gimp_string_action_activate (GAction  *action,
                             GVariant *parameter)
{
  GimpStringAction *string_action = GIMP_STRING_ACTION (action);

  gimp_action_emit_activate (GIMP_ACTION (action),
                             g_variant_new_string (string_action->value));

  gimp_action_history_action_activated (GIMP_ACTION (action));
}
