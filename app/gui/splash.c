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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gui-types.h"

#include "splash.h"

#include "gimp-intl.h"


static GtkWidget   *splash   = NULL;
static PangoLayout *upper    = NULL;
static PangoLayout *lower    = NULL;
static GtkWidget   *progress = NULL;
static gint         width    = 0;
static gint         height   = 0;


static void      splash_map         (void);
static gboolean  splash_area_expose (GtkWidget      *widget,
                                     GdkEventExpose *event,
                                     GdkPixmap      *pixmap);


/*  public functions  */

void
splash_create (void)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *area;
  GdkPixbuf *pixbuf;
  GdkPixmap *pixmap;
  gchar     *filename;

  filename = g_build_filename (gimp_data_directory (),
                               "images", "gimp_splash.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
  g_free (filename);

  if (! pixbuf)
    return;

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  splash = g_object_new (GTK_TYPE_WINDOW,
                         "type",            GTK_WINDOW_TOPLEVEL,
                         "type_hint",       GDK_WINDOW_TYPE_HINT_SPLASHSCREEN,
                         "title",           _("GIMP Startup"),
                         "window_position", GTK_WIN_POS_CENTER,
                         "resizable",       FALSE,
                         NULL);

  gtk_window_set_role (GTK_WINDOW (splash), "gimp-startup");

  g_signal_connect_swapped (splash, "delete_event",
                            G_CALLBACK (exit),
                            GINT_TO_POINTER (0));

  /* we don't want the splash screen to send the startup notification */
  gtk_window_set_auto_startup_notification (FALSE);
  g_signal_connect (splash, "map",
                    G_CALLBACK (splash_map),
                    NULL);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (splash), frame);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  area = gtk_drawing_area_new ();
  gtk_box_pack_start_defaults (GTK_BOX (vbox), area);
  gtk_widget_show (area);

  gtk_widget_set_size_request (area, width, height);

  gtk_widget_realize (area);
  pixmap = gdk_pixmap_new (area->window, width, height, -1);

  gdk_draw_pixbuf (pixmap, area->style->black_gc,
                   pixbuf, 0, 0, 0, 0, width, height,
                   GDK_RGB_DITHER_NORMAL, 0, 0);
  g_object_unref (pixbuf);

  g_signal_connect (area, "expose_event",
                    G_CALLBACK (splash_area_expose),
                    pixmap);
  g_signal_connect_swapped (area, "destroy",
                            G_CALLBACK (g_object_unref),
                            pixmap);

  upper = gtk_widget_create_pango_layout (splash, "");
  lower = gtk_widget_create_pango_layout (splash, "");

  progress = gtk_progress_bar_new ();
  gtk_box_pack_end (GTK_BOX (vbox), progress, FALSE, FALSE, 0);
  gtk_widget_show (progress);

  gtk_widget_show (splash);
}

void
splash_destroy (void)
{
  if (splash)
    {
      gtk_widget_destroy (splash);
      splash = NULL;

      g_object_unref (upper);
      upper = NULL;

      g_object_unref (lower);
      lower = NULL;
    }
}

void
splash_update (const gchar *text1,
	       const gchar *text2,
	       gdouble      percentage)
{
  if (! splash)
    return;

  if (text1)
    pango_layout_set_text (upper, text1, -1);

  if (text2)
    pango_layout_set_text (lower, text2, -2);

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
                                 CLAMP (percentage, 0.0, 1.0));

  /*  FIXME: It would be more effective to expose the text area only.  */
  gtk_widget_queue_draw (splash);

  while (gtk_events_pending ())
    gtk_main_iteration ();
}


/*  private functions  */

static gboolean
splash_area_expose (GtkWidget      *widget,
                    GdkEventExpose *event,
                    GdkPixmap      *pixmap)
{
  GdkRectangle  rect;
  GdkRectangle  draw;
  gint          x, y;

  rect.x      = (widget->allocation.width  - width)  / 2;
  rect.y      = (widget->allocation.height - height) / 2;
  rect.width  = width;
  rect.height = height;

  if (gdk_rectangle_intersect (&rect, &event->area, &draw))
    {
      gdk_draw_drawable (widget->window,
                         widget->style->black_gc,
                         pixmap,
                         draw.x - rect.x, draw.y - rect.y,
                         draw.x, draw.y, draw.width, draw.height);
    }

  pango_layout_get_pixel_size (upper, &x, &y);

  x = (widget->allocation.width - x) / 2;
  y = widget->allocation.height - 2 * y - 16;
  gdk_draw_layout (widget->window, widget->style->black_gc, x, y, upper);

  pango_layout_get_pixel_size (lower, &x, &y);

  x = (widget->allocation.width - x) / 2;
  y = widget->allocation.height - y - 8;
  gdk_draw_layout (widget->window, widget->style->black_gc, x, y, lower);

  return FALSE;
}

static void
splash_map (void)
{
  /*  Reenable startup notification after the splash has been shown
   *  so that the next window that is mapped sends the notification.
   */
   gtk_window_set_auto_startup_notification (TRUE);
}
