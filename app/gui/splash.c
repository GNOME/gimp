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

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
 
#include "gui-types.h"

#include "splash.h"

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/gimpintl.h"


#define DEFAULT_WIDTH 300

static gint         max_label_length  = 1024;
static GtkWidget   *win_initstatus    = NULL;
static GtkWidget   *label1            = NULL;
static GtkWidget   *label2            = NULL;
static GtkWidget   *progress          = NULL;


/*  public functions  */

void
splash_create (gboolean show_image)
{
  GtkWidget        *vbox;
  GdkPixbuf        *pixbuf = NULL;
  PangoFontMetrics *metrics;
  PangoContext     *context;
  gint              width;
  gint              char_width;

  win_initstatus = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint (GTK_WINDOW (win_initstatus),
                            GDK_WINDOW_TYPE_HINT_DIALOG);

  gtk_window_set_title (GTK_WINDOW (win_initstatus), _("GIMP Startup"));
  gtk_window_set_wmclass (GTK_WINDOW (win_initstatus), "gimp_startup", "Gimp");
  gtk_window_set_position (GTK_WINDOW (win_initstatus), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (win_initstatus), FALSE, FALSE, FALSE);

  gimp_dialog_set_icon (GTK_WINDOW (win_initstatus));

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (win_initstatus), vbox);
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

  if (!pixbuf)
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

  /*  This is a hack: we try to compute a good guess for the maximum 
   *  number of charcters that will fit into the splash-screen using 
   *  the default_font
   */
  context = gtk_widget_get_pango_context (label2);
  metrics = pango_context_get_metrics (context,
                                       label2->style->font_desc,
                                       pango_context_get_language (context));
  char_width = pango_font_metrics_get_approximate_char_width (metrics);
  pango_font_metrics_unref (metrics);

  if (pixbuf)
    {
      width = gdk_pixbuf_get_width (pixbuf);
    }
  else
    {
      width = DEFAULT_WIDTH;
      gtk_widget_set_size_request (win_initstatus, width, -1);
    }

  max_label_length = 
    0.9 * (gdouble) width / (gdouble) PANGO_PIXELS (char_width);

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
  gchar *temp;

  if (win_initstatus)
    {
      if (text1)
	gtk_label_set_text (GTK_LABEL (label1), text1);

      if (text2)
	{
	  while (strlen (text2) > max_label_length)
	    {
	      temp = strchr (text2, G_DIR_SEPARATOR);
	      if (temp == NULL)  /* for sanity */
		break;
	      temp++;
	      text2 = temp;
	    }

	  gtk_label_set_text (GTK_LABEL (label2), text2);
	}

      percentage = CLAMP (percentage, 0.0, 1.0);

      gtk_progress_bar_update (GTK_PROGRESS_BAR (progress), percentage);

      while (gtk_events_pending ())
	gtk_main_iteration ();
    }
}
