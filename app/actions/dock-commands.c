/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpmenudock.h"

#include "actions.h"
#include "dock-commands.h"


/*  public functions  */

void
dock_toggle_image_menu_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GtkWidget *widget;
  gboolean   active;
  return_if_no_widget (widget, data);

  if (! GTK_WIDGET_TOPLEVEL (widget))
    widget = gtk_widget_get_toplevel (widget);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (GIMP_IS_MENU_DOCK (widget))
    gimp_menu_dock_set_show_image_menu (GIMP_MENU_DOCK (widget), active);
}

void
dock_toggle_auto_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GtkWidget *widget;
  gboolean   active;
  return_if_no_widget (widget, data);

  if (! GTK_WIDGET_TOPLEVEL (widget))
    widget = gtk_widget_get_toplevel (widget);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (GIMP_IS_MENU_DOCK (widget))
    gimp_menu_dock_set_auto_follow_active (GIMP_MENU_DOCK (widget), active);
}
