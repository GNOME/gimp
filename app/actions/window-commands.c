/* The GIMP -- an image manipulation program
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

#include "actions.h"
#include "window-commands.h"


/*  public functions  */

void
window_close_cmd_callback (GtkAction *action,
                           gpointer   data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  if (! GTK_WIDGET_TOPLEVEL (widget))
    widget = gtk_widget_get_toplevel (widget);

  if (widget && widget->window)
    {
      GdkEvent *event = gdk_event_new (GDK_DELETE);

      event->any.window     = g_object_ref (widget->window);
      event->any.send_event = TRUE;

      gtk_main_do_event (event);
      gdk_event_free (event);
    }
}

void
window_move_to_screen_cmd_callback (GtkAction *action,
                                    GtkAction *current,
                                    gpointer   data)
{
  GtkWidget *widget;
  GdkScreen *screen;
  return_if_no_widget (widget, data);

  if (! GTK_WIDGET_TOPLEVEL (widget))
    widget = gtk_widget_get_toplevel (widget);

  screen = g_object_get_data (G_OBJECT (current), "screen");

  if (GDK_IS_SCREEN (screen) && screen != gtk_widget_get_screen (widget))
    {
      gtk_window_set_screen (GTK_WINDOW (widget), screen);
    }
}
