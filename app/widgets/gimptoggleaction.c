/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoggleaction.c
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
 * Copyright (C) 2008 Sven Neumann <sven@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "ligmaaction.h"
#include "ligmatoggleaction.h"


static void   ligma_toggle_action_connect_proxy (GtkAction       *action,
                                                GtkWidget       *proxy);

static void   ligma_toggle_action_toggled       (GtkToggleAction *action);


G_DEFINE_TYPE_WITH_CODE (LigmaToggleAction, ligma_toggle_action,
                         GTK_TYPE_TOGGLE_ACTION,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_ACTION, NULL))

#define parent_class ligma_toggle_action_parent_class


static void
ligma_toggle_action_class_init (LigmaToggleActionClass *klass)
{
  GtkActionClass       *action_class = GTK_ACTION_CLASS (klass);
  GtkToggleActionClass *toggle_class = GTK_TOGGLE_ACTION_CLASS (klass);

  action_class->connect_proxy = ligma_toggle_action_connect_proxy;

  toggle_class->toggled       = ligma_toggle_action_toggled;
}

static void
ligma_toggle_action_init (LigmaToggleAction *action)
{
  ligma_action_init (LIGMA_ACTION (action));
}

static void
ligma_toggle_action_connect_proxy (GtkAction *action,
                                  GtkWidget *proxy)
{
  GTK_ACTION_CLASS (parent_class)->connect_proxy (action, proxy);

  ligma_action_set_proxy (LIGMA_ACTION (action), proxy);
}

static void
ligma_toggle_action_toggled (GtkToggleAction *action)
{
  gboolean value = ligma_toggle_action_get_active (LIGMA_TOGGLE_ACTION (action));

  ligma_action_emit_change_state (LIGMA_ACTION (action),
                                 g_variant_new_boolean (value));
}


/*  public functions  */

GtkToggleAction *
ligma_toggle_action_new (const gchar *name,
                        const gchar *label,
                        const gchar *tooltip,
                        const gchar *icon_name,
                        const gchar *help_id)
{
  GtkToggleAction *action;

  action = g_object_new (LIGMA_TYPE_TOGGLE_ACTION,
                         "name",      name,
                         "label",     label,
                         "tooltip",   tooltip,
                         "icon-name", icon_name,
                         NULL);

  ligma_action_set_help_id (LIGMA_ACTION (action), help_id);

  return action;
}

void
ligma_toggle_action_set_active (LigmaToggleAction *action,
                               gboolean          active)
{
  return gtk_toggle_action_set_active ((GtkToggleAction *) action, active);
}

gboolean
ligma_toggle_action_get_active (LigmaToggleAction *action)
{
  return gtk_toggle_action_get_active ((GtkToggleAction *) action);
}
