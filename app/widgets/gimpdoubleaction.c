/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadoubleaction.c
 * Copyright (C) 2022 Jehan
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
#include "ligmadoubleaction.h"


enum
{
  PROP_0,
  PROP_VALUE
};


static void   ligma_double_action_set_property (GObject      *object,
                                               guint         prop_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void   ligma_double_action_get_property (GObject      *object,
                                               guint         prop_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);

static void   ligma_double_action_activate     (GtkAction    *action);


G_DEFINE_TYPE (LigmaDoubleAction, ligma_double_action, LIGMA_TYPE_ACTION_IMPL)

#define parent_class ligma_double_action_parent_class


static void
ligma_double_action_class_init (LigmaDoubleActionClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkActionClass *action_class = GTK_ACTION_CLASS (klass);

  object_class->set_property = ligma_double_action_set_property;
  object_class->get_property = ligma_double_action_get_property;

  action_class->activate = ligma_double_action_activate;

  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_double ("value",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE, G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_double_action_init (LigmaDoubleAction *action)
{
}

static void
ligma_double_action_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaDoubleAction *action = LIGMA_DOUBLE_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_value_set_double (value, action->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ligma_double_action_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaDoubleAction *action = LIGMA_DOUBLE_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      action->value = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

LigmaDoubleAction *
ligma_double_action_new (const gchar *name,
                        const gchar *label,
                        const gchar *tooltip,
                        const gchar *icon_name,
                        const gchar *help_id,
                        gdouble      value)
{
  LigmaDoubleAction *action;

  action = g_object_new (LIGMA_TYPE_DOUBLE_ACTION,
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
ligma_double_action_activate (GtkAction *action)
{
  LigmaDoubleAction *double_action = LIGMA_DOUBLE_ACTION (action);

  ligma_action_emit_activate (LIGMA_ACTION (action),
                             g_variant_new_double (double_action->value));

  ligma_action_history_action_activated (LIGMA_ACTION (action));
}
