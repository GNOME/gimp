/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaenumaction.c
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
#include "ligmaenumaction.h"


enum
{
  PROP_0,
  PROP_VALUE,
  PROP_VALUE_VARIABLE
};


static void   ligma_enum_action_set_property (GObject      *object,
                                             guint         prop_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void   ligma_enum_action_get_property (GObject      *object,
                                             guint         prop_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);

static void   ligma_enum_action_activate     (GtkAction    *action);


G_DEFINE_TYPE (LigmaEnumAction, ligma_enum_action, LIGMA_TYPE_ACTION_IMPL)

#define parent_class ligma_enum_action_parent_class


static void
ligma_enum_action_class_init (LigmaEnumActionClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkActionClass *action_class = GTK_ACTION_CLASS (klass);

  object_class->set_property = ligma_enum_action_set_property;
  object_class->get_property = ligma_enum_action_get_property;

  action_class->activate     = ligma_enum_action_activate;

  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_int ("value",
                                                     NULL, NULL,
                                                     G_MININT, G_MAXINT, 0,
                                                     LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_VALUE_VARIABLE,
                                   g_param_spec_boolean ("value-variable",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_enum_action_init (LigmaEnumAction *action)
{
}

static void
ligma_enum_action_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaEnumAction *action = LIGMA_ENUM_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_value_set_int (value, action->value);
      break;
    case PROP_VALUE_VARIABLE:
      g_value_set_boolean (value, action->value_variable);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ligma_enum_action_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaEnumAction *action = LIGMA_ENUM_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      action->value = g_value_get_int (value);
      break;
    case PROP_VALUE_VARIABLE:
      action->value_variable = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

LigmaEnumAction *
ligma_enum_action_new (const gchar *name,
                      const gchar *label,
                      const gchar *tooltip,
                      const gchar *icon_name,
                      const gchar *help_id,
                      gint         value,
                      gboolean     value_variable)
{
  LigmaEnumAction *action;

  action = g_object_new (LIGMA_TYPE_ENUM_ACTION,
                         "name",           name,
                         "label",          label,
                         "tooltip",        tooltip,
                         "icon-name",      icon_name,
                         "value",          value,
                         "value-variable", value_variable,
                         NULL);

  ligma_action_set_help_id (LIGMA_ACTION (action), help_id);

  return action;
}

static void
ligma_enum_action_activate (GtkAction *action)
{
  LigmaEnumAction *enum_action = LIGMA_ENUM_ACTION (action);

  ligma_action_emit_activate (LIGMA_ACTION (enum_action),
                             g_variant_new_int32 (enum_action->value));

  ligma_action_history_action_activated (LIGMA_ACTION (action));
}
