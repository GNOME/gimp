/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpradioaction.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpaction.h"
#include "gimpradioaction.h"


static void   gimp_radio_action_connect_proxy (GtkAction      *action,
                                               GtkWidget      *proxy);

static void   gimp_radio_action_changed       (GtkRadioAction *action,
                                               GtkRadioAction *current);


G_DEFINE_TYPE_WITH_CODE (GimpRadioAction, gimp_radio_action,
                         GTK_TYPE_RADIO_ACTION,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_ACTION, NULL))

#define parent_class gimp_radio_action_parent_class


static void
gimp_radio_action_class_init (GimpRadioActionClass *klass)
{
  GtkActionClass      *action_class = GTK_ACTION_CLASS (klass);
  GtkRadioActionClass *radio_class  = GTK_RADIO_ACTION_CLASS (klass);

  action_class->connect_proxy = gimp_radio_action_connect_proxy;

  radio_class->changed        = gimp_radio_action_changed;
}

static void
gimp_radio_action_init (GimpRadioAction *action)
{
  gimp_action_init (GIMP_ACTION (action));
}

static void
gimp_radio_action_connect_proxy (GtkAction *action,
                                 GtkWidget *proxy)
{
  GTK_ACTION_CLASS (parent_class)->connect_proxy (action, proxy);

  gimp_action_set_proxy (GIMP_ACTION (action), proxy);
}

static void
gimp_radio_action_changed (GtkRadioAction *action,
                           GtkRadioAction *current)
{
  gint value = gtk_radio_action_get_current_value (action);

  gimp_action_emit_change_state (GIMP_ACTION (action),
                                 g_variant_new_int32 (value));
}


/*  public functions  */

GtkRadioAction *
gimp_radio_action_new (const gchar *name,
                       const gchar *label,
                       const gchar *tooltip,
                       const gchar *icon_name,
                       const gchar *help_id,
                       gint         value)
{
  GtkRadioAction *action;

  action = g_object_new (GIMP_TYPE_RADIO_ACTION,
                         "name",      name,
                         "label",     label,
                         "tooltip",   tooltip,
                         "icon-name", icon_name,
                         "value",     value,
                         NULL);

  gimp_action_set_help_id (GIMP_ACTION (action), help_id);

  return action;
}
