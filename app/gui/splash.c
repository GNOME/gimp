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

#include <string.h>

#include <gtk/gtk.h>
 
#include "gui-types.h"

#include "splash.h"

#include "libgimpbase/gimpbase.h"

#include "libgimp/gimpintl.h"


#define DEFAULT_WIDTH 300

static GtkWidget   *win_initstatus    = NULL;
static GtkWidget   *label1            = NULL;
static GtkWidget   *label2            = NULL;
static GtkWidget   *progress          = NULL;


/*  public functions  */

void
splash_create (gboolean show_image)
{
  GtkWidget *vbox;
  GdkPixbufAnimation *animation = NULL;

  win_initstatus = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint (GTK_WINDOW (win_initstatus),
                            GDK_WINDOW_TYPE_HINT_DIALOG);

  gtk_window_set_title (GTK_WINDOW (win_initstatus), _("GIMP Startup"));
  gtk_window_set_wmclass (GTK_WINDOW (win_initstatus), "gimp_startup", "Gimp");
  gtk_window_set_position (GTK_WINDOW (win_initstatus), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable (GTK_WINDOW (win_initstatus), FALSE);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (win_initstatus), vbox);
  gtk_widget_show (vbox);

  if (show_image)
    {
      gchar *filename;

      filename = g_build_filename (gimp_data_directory (), 
                                   "images", "gimp_splash.gif", NULL);
      animation = gdk_pixbuf_animation_new_from_file (filename, NULL);
      g_free (filename);
      
      if (animation)
        {
          GtkWidget *align;
          GtkWidget *image;

          image = gtk_image_new_from_animation (animation);
          g_object_unref (animation);

          align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
          gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, TRUE, 0);
          gtk_widget_show (align);
 
          gtk_container_add (GTK_CONTAINER (align), image);
          gtk_widget_show (image);
        }
    }

  if (!animation)
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

  if (!animation)
    gtk_widget_set_size_request (win_initstatus, DEFAULT_WIDTH, -1);

  gtk_widget_show (win_initstatus);
}

void
splash_destroy (void)
{
  if (win_initstatus)
    {
      gtk_widget_destroy (win_initstatus);
      win_initstatus = NULL;
    }
}

void
splash_update (const gchar *text1,
	       const gchar *text2,
	       gdouble      percentage)
{
  if (!win_initstatus)
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
