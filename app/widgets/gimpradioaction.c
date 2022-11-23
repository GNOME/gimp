/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaradioaction.c
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
#include "ligmaradioaction.h"


static void   ligma_radio_action_connect_proxy (GtkAction      *action,
                                               GtkWidget      *proxy);

static void   ligma_radio_action_changed       (GtkRadioAction *action,
                                               GtkRadioAction *current);


G_DEFINE_TYPE_WITH_CODE (LigmaRadioAction, ligma_radio_action,
                         GTK_TYPE_RADIO_ACTION,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_ACTION, NULL))

#define parent_class ligma_radio_action_parent_class


static void
ligma_radio_action_class_init (LigmaRadioActionClass *klass)
{
  GtkActionClass      *action_class = GTK_ACTION_CLASS (klass);
  GtkRadioActionClass *radio_class  = GTK_RADIO_ACTION_CLASS (klass);

  action_class->connect_proxy = ligma_radio_action_connect_proxy;

  radio_class->changed        = ligma_radio_action_changed;
}

static void
ligma_radio_action_init (LigmaRadioAction *action)
{
  ligma_action_init (LIGMA_ACTION (action));
}

static void
ligma_radio_action_connect_proxy (GtkAction *action,
                                 GtkWidget *proxy)
{
  GTK_ACTION_CLASS (parent_class)->connect_proxy (action, proxy);

  ligma_action_set_proxy (LIGMA_ACTION (action), proxy);
}

static void
ligma_radio_action_changed (GtkRadioAction *action,
                           GtkRadioAction *current)
{
  gint value = gtk_radio_action_get_current_value (action);

  ligma_action_emit_change_state (LIGMA_ACTION (action),
                                 g_variant_new_int32 (value));
}


/*  public functions  */

GtkRadioAction *
ligma_radio_action_new (const gchar *name,
                       const gchar *label,
                       const gchar *tooltip,
                       const gchar *icon_name,
                       const gchar *help_id,
                       gint         value)
{
  GtkRadioAction *action;

  action = g_object_new (LIGMA_TYPE_RADIO_ACTION,
                         "name",      name,
                         "label",     label,
                         "tooltip",   tooltip,
                         "icon-name", icon_name,
                         "value",     value,
                         NULL);

  ligma_action_set_help_id (LIGMA_ACTION (action), help_id);

  return action;
}
