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
 
#include "libgimp/gimpenv.h"
#include "libgimp/gimpfeatures.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "appenv.h"
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
#define AUTHORS "Spencer Kimball & Peter Mattis"


static gboolean  splash_logo_load_size (GtkWidget *window);
static void      splash_logo_draw      (GtkWidget *widget);
static void      splash_text_draw      (GtkWidget *widget);
static void      splash_logo_expose    (GtkWidget *widget);


static gint       splash_show_logo  = SPLASH_SHOW_LOGO_NEVER;

static GtkWidget *logo_area         = NULL;
static GdkPixmap *logo_pixmap       = NULL;
static gint       logo_width        = 0;
static gint       logo_height       = 0;
static gint       logo_area_width   = 0;
static gint       logo_area_height  = 0;
static gint       max_label_length  = 1024;

static GtkWidget *win_initstatus = NULL;
static GtkWidget *label1 = NULL;
static GtkWidget *label2 = NULL;
static GtkWidget *pbar   = NULL;


/*  public functions  */

void
splash_create (void)
{
  GtkWidget *vbox;
  GtkWidget *logo_hbox;
  GtkStyle  *style;

  win_initstatus = gtk_window_new (GTK_WINDOW_DIALOG);

  gtk_window_set_title (GTK_WINDOW (win_initstatus), _("GIMP Startup"));
  gtk_window_set_wmclass (GTK_WINDOW (win_initstatus), "gimp_startup", "Gimp");
  gtk_window_set_position (GTK_WINDOW (win_initstatus), GTK_WIN_POS_CENTER);
  gtk_window_set_policy (GTK_WINDOW (win_initstatus), FALSE, FALSE, FALSE);

  gimp_dialog_set_icon (GTK_WINDOW (win_initstatus));

  if (no_splash_image == FALSE &&
      splash_logo_load_size (win_initstatus))
    {
      splash_show_logo = SPLASH_SHOW_LOGO_LATER;
    }

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (win_initstatus), vbox);

  logo_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), logo_hbox, FALSE, TRUE, 0);

  logo_area = gtk_drawing_area_new ();

  gtk_signal_connect (GTK_OBJECT (logo_area), "expose_event",
		      GTK_SIGNAL_FUNC (splash_logo_expose),
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

  pbar = gtk_progress_bar_new ();
  gtk_box_pack_start_defaults (GTK_BOX (vbox), pbar);

  gtk_widget_show (vbox);
  gtk_widget_show (logo_hbox);
  gtk_widget_show (logo_area);
  gtk_widget_show (label1);
  gtk_widget_show (label2);
  gtk_widget_show (pbar);

  gtk_widget_show (win_initstatus);

  /*  This is a hack: we try to compute a good guess for the maximum 
   *  number of charcters that will fit into the splash-screen using 
   *  the default_font
   */
  style = gtk_widget_get_style (win_initstatus);
  max_label_length = (0.8 * (gfloat) strlen (AUTHORS) *
		      ((gfloat) logo_area_width /
		       (gfloat) gdk_string_width (style->font, AUTHORS)));

  splash_text_draw (logo_area);
}

void
splash_destroy (void)
{
  if (win_initstatus)
    {
      gtk_widget_destroy (win_initstatus);
      if (logo_pixmap != NULL)
	gdk_pixmap_unref (logo_pixmap);

      win_initstatus = label1 = label2 = pbar = logo_area = NULL;
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

      gtk_progress_bar_update (GTK_PROGRESS_BAR (pbar), percentage);

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
  FILE  *fp;

  if (logo_pixmap)
    return TRUE;

  g_snprintf (buf, sizeof(buf), "%s" G_DIR_SEPARATOR_S "gimp_splash.ppm",
	      gimp_data_directory ());

  fp = fopen (buf, "rb");
  if (!fp)
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
  guchar    *pixelrow;
  FILE      *fp;
  gint       count;
  gint       i;

  if (! win_initstatus || logo_pixmap || ! splash_show_logo)
    return;

  g_snprintf (buf, sizeof (buf), "%s" G_DIR_SEPARATOR_S "gimp_splash.ppm",
	      gimp_data_directory ());

  fp = fopen (buf, "rb");
  if (!fp)
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
      count = fread (pixelrow, sizeof (unsigned char), logo_width * 3, fp);
      if (count != (logo_width * 3))
	{
	  gtk_widget_destroy (preview);
	  g_free (pixelrow);
	  fclose (fp);
	  return;
	}
      gtk_preview_draw_row (GTK_PREVIEW (preview), pixelrow, 0, i, logo_width);
    }

  gtk_widget_realize (win_initstatus);
  logo_pixmap = gdk_pixmap_new (win_initstatus->window, logo_width, logo_height,
				gtk_preview_get_visual ()->depth);
  gc = gdk_gc_new (logo_pixmap);
  gtk_preview_put (GTK_PREVIEW (preview),
		   logo_pixmap, gc,
		   0, 0, 0, 0, logo_width, logo_height);
  gdk_gc_destroy (gc);

  gtk_widget_unref (preview);
  g_free (pixelrow);

  fclose (fp);

  splash_show_logo = SPLASH_SHOW_LOGO_NOW;

  splash_logo_draw (logo_area);
}

static void
splash_text_draw (GtkWidget *widget)
{
  GdkFont *font;

  /* this is a font, provide only one single font definition */
  font = gdk_font_load (_("-*-helvetica-bold-r-normal--*-140-*-*-*-*-*-*"));
  if (!font)
    {
      GtkStyle *style = gtk_widget_get_style (widget);
      font = style->font;
      gdk_font_ref (font);
    }

  gdk_draw_string (widget->window,
		   font,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   ((logo_area_width - gdk_string_width (font,  _("The GIMP"))) / 2),
		   (0.25 * logo_area_height),
		   _("The GIMP"));

  gdk_font_unref (font);

  /* this is a fontset, e.g. multiple comma-separated font definitions */
  font = gdk_fontset_load (_("-*-helvetica-bold-r-normal--*-120-*-*-*-*-*-*,*"));
  if (!font)
    {
      GtkStyle *style = gtk_widget_get_style (widget);
      font = style->font;
      gdk_font_ref (font);
    }

  gdk_draw_string (widget->window,
		   font,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   ((logo_area_width - gdk_string_width (font, GIMP_VERSION)) / 2),
		   (0.45 * logo_area_height),
		   GIMP_VERSION);
  gdk_draw_string (widget->window,
		   font,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   ((logo_area_width - gdk_string_width (font, _("brought to you by"))) / 2),
		   (0.65 * logo_area_height),
		   _("brought to you by"));
  gdk_draw_string (widget->window,
		   font,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   ((logo_area_width - gdk_string_width (font, AUTHORS)) / 2),
		   (0.80 * logo_area_height),
		   AUTHORS);

  gdk_font_unref (font);
}

static void
splash_logo_draw (GtkWidget *widget)
{
  gdk_draw_pixmap (widget->window,
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
