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


#define DEFAULT_WIDTH 300

static GtkWidget *splash   = NULL;
static GtkWidget *label1   = NULL;
static GtkWidget *label2   = NULL;
static GtkWidget *progress = NULL;


static void  splash_map (void);


/*  public functions  */

void
splash_create (gboolean show_image)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GdkPixbuf *pixbuf = NULL;

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

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  if (show_image)
    {
      gchar *filename;

      filename = g_build_filename (gimp_data_directory (),
                                   "images", "gimp_splash.png", NULL);
      pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
      g_free (filename);

      if (pixbuf)
        {
          GtkWidget *align;
          GtkWidget *image;

          image = gtk_image_new_from_pixbuf (pixbuf);
          g_object_unref (pixbuf);

          align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
          gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, TRUE, 0);
          gtk_widget_show (align);

          gtk_container_add (GTK_CONTAINER (align), image);
          gtk_widget_show (image);
        }
    }

  if (! pixbuf)
    {
      GtkWidget *line;

      label1 = gtk_label_new (_("The GIMP"));
      gtk_box_pack_start_defaults (GTK_BOX (vbox), label1);
      gtk_widget_show (label1);

      label2 = gtk_label_new (GIMP_VERSION);
      gtk_box_pack_start_defaults (GTK_BOX (vbox), label2);
      gtk_widget_show (label2);

      line = gtk_hseparator_new ();
      gtk_box_pack_start_defaults (GTK_BOX (vbox), line);
      gtk_widget_show (line);

      gtk_widget_set_size_request (splash, DEFAULT_WIDTH, -1);
    }

  label1 = gtk_label_new ("");
  gtk_box_pack_start_defaults (GTK_BOX (vbox), label1);
  gtk_widget_show (label1);

  label2 = gtk_label_new ("");
  gtk_box_pack_start_defaults (GTK_BOX (vbox), label2);
  gtk_widget_show (label2);

  progress = gtk_progress_bar_new ();
  gtk_box_pack_start_defaults (GTK_BOX (vbox), progress);
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
    gtk_label_set_text (GTK_LABEL (label1), text1);

  if (text2)
    gtk_label_set_text (GTK_LABEL (label2), text2);

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
                                 CLAMP (percentage, 0.0, 1.0));

  while (gtk_events_pending ())
    gtk_main_iteration ();
}


/*  private functions  */

static void
splash_map (void)
{
  /*  Reenable startup notification after the splash has been shown
   *  so that the next window that is mapped sends the notification.
   */
   gtk_window_set_auto_startup_notification (TRUE);
}

