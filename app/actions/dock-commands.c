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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpdockwindow.h"
#include "widgets/gimpdockwindow.h"

#include "actions.h"
#include "dock-commands.h"


static GimpDockWindow *
dock_commands_get_dock_window_from_widget (GtkWidget *widget)
{
  GtkWidget      *toplevel    = gtk_widget_get_toplevel (widget);
  GimpDockWindow *dock_window = NULL;

  if (GIMP_IS_DOCK_WINDOW (toplevel))
    dock_window = GIMP_DOCK_WINDOW (toplevel);

  return dock_window;
}


/*  public functions  */

void
dock_toggle_image_menu_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GtkWidget      *widget      = NULL;
  GimpDockWindow *dock_window = NULL;
  return_if_no_widget (widget, data);

  dock_window = dock_commands_get_dock_window_from_widget (widget);

  if (dock_window)
    {
      gboolean active = g_variant_get_boolean (value);

      gimp_dock_window_set_show_image_menu (dock_window, active);
    }
}

void
dock_toggle_auto_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GtkWidget      *widget      = NULL;
  GimpDockWindow *dock_window = NULL;
  return_if_no_widget (widget, data);

  dock_window = dock_commands_get_dock_window_from_widget (widget);

  if (dock_window)
    {
      gboolean active = g_variant_get_boolean (value);

      gimp_dock_window_set_auto_follow_active (dock_window, active);
    }
}
