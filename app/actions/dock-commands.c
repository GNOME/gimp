/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpdockwindow.h"
#include "widgets/gimpmenudock.h"

#include "actions.h"
#include "dock-commands.h"


static GimpMenuDock *
dock_commands_get_menudock_from_widget (GtkWidget *widget)
{
  GtkWidget    *toplevel  = gtk_widget_get_toplevel (widget);
  GimpMenuDock *menu_dock = NULL;

  if (GIMP_IS_DOCK_WINDOW (toplevel))
    {
      GimpDock *dock = gimp_dock_window_get_dock (GIMP_DOCK_WINDOW (toplevel));

      if (GIMP_IS_MENU_DOCK (dock))
        menu_dock = GIMP_MENU_DOCK (dock);
    }

  return menu_dock;
}


/*  public functions  */

void
dock_toggle_image_menu_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GtkWidget    *widget    = NULL;
  GimpMenuDock *menu_dock = NULL;
  return_if_no_widget (widget, data);

  menu_dock = dock_commands_get_menudock_from_widget (widget);

  if (menu_dock)
    {
      gboolean active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
      gimp_menu_dock_set_show_image_menu (menu_dock, active);
    }
}

void
dock_toggle_auto_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GtkWidget    *widget    = NULL;
  GimpMenuDock *menu_dock = NULL;
  return_if_no_widget (widget, data);

  menu_dock = dock_commands_get_menudock_from_widget (widget);

  if (menu_dock)
    {
      gboolean active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
      gimp_menu_dock_set_auto_follow_active (menu_dock, active);
    }
}
