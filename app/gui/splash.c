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
 
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "splash.h"

#include "libgimp/gimpintl.h"


enum
{
  SPLASH_SHOW_LOGO_NEVER,
  SPLASH_SHOW_LOGO_LATER,
  SPLASH_SHOW_LOGO_NOW
};


#define LOGO_WIDTH_MIN  300
#define LOGO_HEIGHT_MIN 110


static gboolean  splash_logo_load_size (GtkWidget *window);
static void      splash_logo_draw      (GtkWidget *widget);
static void      splash_text_draw      (GtkWidget *widget);
static void      splash_logo_expose    (GtkWidget *widget);


static gint         splash_show_logo  = SPLASH_SHOW_LOGO_NEVER;

static GtkWidget   *logo_area         = NULL;
static GdkPixmap   *logo_pixmap       = NULL;
static gint         logo_width        = 0;
static gint         logo_height       = 0;
static gint         logo_area_width   = 0;
static gint         logo_area_height  = 0;
static PangoLayout *logo_layout       = NULL;

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
  GtkWidget        *logo_hbox;
  PangoFontMetrics *metrics;
  PangoContext     *context;
  gint              char_width;

  win_initstatus = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint (GTK_WINDOW (win_initstatus),
                            GDK_WINDOW_TYPE_HINT_DIALOG);

  gtk_window_set_title (GTK_WINDOW (win_initstatus), _("GIMP Startup"));
  gtk_window_set_wmclass (GTK_WINDOW (win_initstatus), "gimp_startup", "Gimp");
  gtk_window_set_position (GTK_WINDOW (win_initstatus), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (win_initstatus), FALSE, FALSE, FALSE);

  gimp_dialog_set_icon (GTK_WINDOW (win_initstatus));

  if (show_image && splash_logo_load_size (win_initstatus))
    {
      splash_show_logo = SPLASH_SHOW_LOGO_LATER;
    }

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (win_initstatus), vbox);

  logo_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), logo_hbox, FALSE, TRUE, 0);

  logo_area = gtk_drawing_area_new ();

  g_signal_connect (G_OBJECT (logo_area), "expose_event",
		    G_CALLBACK (splash_logo_expose),
		    NULL);

  logo_area_width  = MAX (logo_width, LOGO_WIDTH_MIN);
  logo_area_height = MAX (logo_height, LOGO_HEIGHT_MIN);

  gtk_drawing_area_size (GTK_DRAWING_AREA (logo_area),
			 logo_area_width, logo_area_height);
  gtk_box_pack_start (GTK_BOX(logo_hbox), logo_area, TRUE, FALSE, 0);

  label1 = gtk_label_new ("");
  gtk_box_pack_start_defaults (GTK_BOX (vbox), label1);
  label2 = gtk_label_new ("");
  gtk_box_pack_start_defaults (GTK_BOX (vbox), label2);

  progress = gtk_progress_bar_new ();
  gtk_box_pack_start_defaults (GTK_BOX (vbox), progress);

  gtk_widget_show (vbox);
  gtk_widget_show (logo_hbox);
  gtk_widget_show (logo_area);
  gtk_widget_show (label1);
  gtk_widget_show (label2);
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

  max_label_length = (0.9 * (gdouble) logo_area_width /
                      (gdouble) PANGO_PIXELS (char_width));

  logo_layout = gtk_widget_create_pango_layout (logo_area, NULL);
  g_object_weak_ref (G_OBJECT (win_initstatus), 
                     (GWeakNotify) g_object_unref, logo_layout);

  gtk_widget_show (win_initstatus);
}

void
splash_destroy (void)
{
  if (win_initstatus)
    {
      gtk_widget_destroy (win_initstatus);
      if (logo_pixmap != NULL)
	gdk_drawable_unref (logo_pixmap);

      win_initstatus = label1 = label2 = progress = logo_area = NULL;
      logo_pixmap = NULL;
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

      /* We sync here to make sure things get drawn before continuing,
       * is the improved look worth the time? I'm not sure...
       */
      gdk_flush ();
    }
}


/*  private functions  */

static gboolean
splash_logo_load_size (GtkWidget *window)
{
  gchar  buf[1024];
  gchar *filename;
  FILE  *fp;

  if (logo_pixmap)
    return TRUE;

  filename = g_build_filename (gimp_data_directory (), "images",
                               "gimp_splash.ppm", NULL);

  fp = fopen (filename, "rb");

  g_free (filename);

  if (! fp)
    return FALSE;

  fgets (buf, sizeof (buf), fp);
  if (strcmp (buf, "P6\n") != 0)
    {
      fclose (fp);
      return FALSE;
    }

  fgets (buf, sizeof (buf), fp);
  fgets (buf, sizeof (buf), fp);
  sscanf (buf, "%d %d", &logo_width, &logo_height);

  fclose (fp);

  return TRUE;
}

void
splash_logo_load (void)
{
  GtkWidget *preview;
  GdkGC     *gc;
  gchar      buf[1024];
  gchar     *filename;
  guchar    *pixelrow;
  FILE      *fp;
  gint       count;
  gint       i;

  if (! win_initstatus || logo_pixmap || ! splash_show_logo)
    return;

  filename = g_build_filename (gimp_data_directory (), "images",
                               "gimp_splash.ppm", NULL);

  fp = fopen (filename, "rb");

  g_free (filename);

  if (! fp)
    return;

  fgets (buf, sizeof (buf), fp);
  if (strcmp (buf, "P6\n") != 0)
    {
      fclose (fp);
      return;
    }

  fgets (buf, sizeof (buf), fp);
  fgets (buf, sizeof (buf), fp);
  sscanf (buf, "%d %d", &logo_width, &logo_height);

  fgets (buf, sizeof (buf), fp);
  if (strcmp (buf, "255\n") != 0)
    {
      fclose (fp);
      return;
    }

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), logo_width, logo_height);
  pixelrow = g_new (guchar, logo_width * 3);

  for (i = 0; i < logo_height; i++)
    {
      count = fread (pixelrow, sizeof (guchar), logo_width * 3, fp);
      if (count != (logo_width * 3))
	{
	  gtk_object_sink (GTK_OBJECT (preview));
	  g_free (pixelrow);
	  fclose (fp);
	  return;
	}
      gtk_preview_draw_row (GTK_PREVIEW (preview), pixelrow, 0, i, logo_width);
    }

  gtk_widget_realize (win_initstatus);
  logo_pixmap = gdk_pixmap_new (win_initstatus->window, 
                                logo_width, logo_height,
				gtk_preview_get_visual ()->depth);
  gc = gdk_gc_new (logo_pixmap);
  gtk_preview_put (GTK_PREVIEW (preview),
		   logo_pixmap, gc,
		   0, 0, 0, 0, logo_width, logo_height);
  gdk_gc_unref (gc);

  gtk_object_sink (GTK_OBJECT (preview));

  g_free (pixelrow);

  fclose (fp);

  splash_show_logo = SPLASH_SHOW_LOGO_NOW;

  splash_logo_draw (logo_area);
}

static void
splash_text_draw (GtkWidget *widget)
{
  gint width;

  pango_layout_set_text (logo_layout, _("The GIMP"), -1);
  pango_layout_get_pixel_size (logo_layout, &width, NULL);

  gdk_draw_layout (widget->window,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   (logo_area_width - width) / 2,
		   0.25 * logo_area_height,
                   logo_layout);

  pango_layout_set_text (logo_layout, GIMP_VERSION, -1);
  pango_layout_get_pixel_size (logo_layout, &width, NULL);

  gdk_draw_layout (widget->window,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   (logo_area_width - width) / 2,
		   0.45 * logo_area_height,
		   logo_layout);
}

static void
splash_logo_draw (GtkWidget *widget)
{
  gdk_draw_drawable (widget->window,
		     widget->style->black_gc,
		     logo_pixmap,
		     0, 0,
		     ((logo_area_width - logo_width) / 2),
		     ((logo_area_height - logo_height) / 2),
		     logo_width, logo_height);
}

static void
splash_logo_expose (GtkWidget *widget)
{
  switch (splash_show_logo)
    {
    case SPLASH_SHOW_LOGO_NEVER:
    case SPLASH_SHOW_LOGO_LATER:
      splash_text_draw (widget);
      break;
    case SPLASH_SHOW_LOGO_NOW:
      splash_logo_draw (widget);
      break;
    }
}
