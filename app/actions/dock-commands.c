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

#include "widgets/gimpimagedock.h"

#include "actions.h"
#include "dock-commands.h"


/*  local function prototypes  */

static void   dock_change_screen_confirm_callback (GtkWidget *dialog,
                                                   gint       value,
                                                   gpointer   data);
static void   dock_change_screen_destroy_callback (GtkWidget *dialog,
                                                   GtkWidget *dock);


/*  public functions  */

void
dock_close_cmd_callback (GtkAction *action,
                         gpointer   data)
{
  GtkWidget *widget;
  GtkWidget *toplevel;
  return_if_no_widget (widget, data);

  toplevel = gtk_widget_get_toplevel (widget);

  if (toplevel && toplevel->window)
    {
      GdkEvent *event = gdk_event_new (GDK_DELETE);

      event->any.window     = g_object_ref (toplevel->window);
      event->any.send_event = TRUE;

      gtk_main_do_event (event);
      gdk_event_free (event);
    }
}

void
dock_change_screen_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GtkWidget  *widget;
  GtkWidget  *toplevel;
  GdkScreen  *screen;
  GdkDisplay *display;
  gint        cur_screen;
  gint        num_screens;
  GtkWidget  *dialog;
  return_if_no_widget (widget, data);

  toplevel = gtk_widget_get_toplevel (widget);

  dialog = g_object_get_data (G_OBJECT (toplevel), "gimp-change-screen-dialog");

  if (dialog)
    {
      gtk_window_present (GTK_WINDOW (dialog));
      return;
    }

  screen  = gtk_widget_get_screen (toplevel);
  display = gtk_widget_get_display (toplevel);

  cur_screen  = gdk_screen_get_number (screen);
  num_screens = gdk_display_get_n_screens (display);

  dialog = gimp_query_int_box ("Move Dock to Screen",
                               toplevel,
                               NULL, NULL,
                               "Enter destination screen",
                               cur_screen, 0, num_screens - 1,
                               G_OBJECT (toplevel), "destroy",
                               dock_change_screen_confirm_callback,
                               toplevel);

  g_object_set_data (G_OBJECT (toplevel), "gimp-change-screen-dialog", dialog);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (dock_change_screen_destroy_callback),
                    toplevel);

  gtk_widget_show (dialog);
}

void
dock_toggle_image_menu_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GtkWidget *widget;
  GtkWidget *toplevel;
  gboolean   active;
  return_if_no_widget (widget, data);

  toplevel = gtk_widget_get_toplevel (widget);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (GIMP_IS_IMAGE_DOCK (toplevel))
    gimp_image_dock_set_show_image_menu (GIMP_IMAGE_DOCK (toplevel), active);
}

void
dock_toggle_auto_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GtkWidget *widget;
  GtkWidget *toplevel;
  gboolean   active;
  return_if_no_widget (widget, data);

  toplevel = gtk_widget_get_toplevel (widget);

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (GIMP_IS_IMAGE_DOCK (toplevel))
    gimp_image_dock_set_auto_follow_active (GIMP_IMAGE_DOCK (toplevel), active);
}


/*  private functions  */

static void
dock_change_screen_confirm_callback (GtkWidget *dialog,
                                     gint       value,
                                     gpointer   data)
{
  GdkScreen *screen;

  screen = gdk_display_get_screen (gtk_widget_get_display (GTK_WIDGET (data)),
                                   value);

  if (screen)
    gtk_window_set_screen (GTK_WINDOW (data), screen);
}

static void
dock_change_screen_destroy_callback (GtkWidget *dialog,
                                     GtkWidget *dock)
{
  g_object_set_data (G_OBJECT (dock), "gimp-change-screen-dialog", NULL);
}
