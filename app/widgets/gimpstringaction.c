/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmastringaction.c
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"

#include "widgets-types.h"

#include "ligmaaction.h"
#include "ligmaaction-history.h"
#include "ligmastringaction.h"


enum
{
  PROP_0,
  PROP_VALUE
};


static void   ligma_string_action_finalize     (GObject      *object);
static void   ligma_string_action_set_property (GObject      *object,
                                               guint         prop_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void   ligma_string_action_get_property (GObject      *object,
                                               guint         prop_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);

static void   ligma_string_action_activate     (GtkAction    *action);


G_DEFINE_TYPE (LigmaStringAction, ligma_string_action, LIGMA_TYPE_ACTION_IMPL)

#define parent_class ligma_string_action_parent_class


static void
ligma_string_action_class_init (LigmaStringActionClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkActionClass *action_class = GTK_ACTION_CLASS (klass);

  object_class->finalize     = ligma_string_action_finalize;
  object_class->set_property = ligma_string_action_set_property;
  object_class->get_property = ligma_string_action_get_property;

  action_class->activate = ligma_string_action_activate;

  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_string ("value",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_string_action_init (LigmaStringAction *action)
{
}

static void
ligma_string_action_finalize (GObject *object)
{
  LigmaStringAction *action = LIGMA_STRING_ACTION (object);

  g_clear_pointer (&action->value, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_string_action_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaStringAction *action = LIGMA_STRING_ACTION (object);

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
ligma_string_action_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaStringAction *action = LIGMA_STRING_ACTION (object);

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

LigmaStringAction *
ligma_string_action_new (const gchar *name,
                        const gchar *label,
                        const gchar *tooltip,
                        const gchar *icon_name,
                        const gchar *help_id,
                        const gchar *value)
{
  LigmaStringAction *action;

  action = g_object_new (LIGMA_TYPE_STRING_ACTION,
                         "name",      name,
                         "label",     label,
                         "tooltip",   tooltip,
                         "icon-name", icon_name,
                         "value",     value,
                         NULL);

  ligma_action_set_help_id (LIGMA_ACTION (action), help_id);

  return action;
}

static void
ligma_string_action_activate (GtkAction *action)
{
  LigmaStringAction *string_action = LIGMA_STRING_ACTION (action);

  ligma_action_emit_activate (LIGMA_ACTION (action),
                             g_variant_new_string (string_action->value));

  ligma_action_history_action_activated (LIGMA_ACTION (action));
}
