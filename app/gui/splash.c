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

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "widgets/gimpwidgets-utils.h"

#include "splash.h"

#include "gimp-intl.h"


typedef struct
{
  GtkWidget      *window;
  GtkWidget      *area;
  gint            width;
  gint            height;
  GtkWidget      *progress;
  GdkRGBA         color;
  PangoLayout    *upper;
  gint            upper_x;
  gint            upper_y;
  PangoLayout    *lower;
  gint            lower_x;
  gint            lower_y;

  gdouble         percentage;
  gchar          *text1;
  gchar          *text2;

  /* debug timer */
  GTimer         *timer;
  gdouble         last_time;
} GimpSplash;

static GimpSplash *splash = NULL;


static void        splash_position_layouts    (GimpSplash     *splash,
                                               const gchar    *text1,
                                               const gchar    *text2,
                                               GdkRectangle   *area);
static gboolean    splash_area_draw           (GtkWidget      *widget,
                                               cairo_t        *cr,
                                               GimpSplash     *splash);
static void        splash_rectangle_union     (GdkRectangle   *dest,
                                               PangoRectangle *pango_rect,
                                               gint            offset_x,
                                               gint            offset_y);
static void        splash_average_text_area   (GimpSplash     *splash,
                                               GdkPixbuf      *pixbuf,
                                               GdkRGBA        *rgba);

static GdkPixbufAnimation *
                   splash_image_load          (gboolean        be_verbose);
static GdkPixbufAnimation *
                   splash_image_pick_from_dir (const gchar    *dirname,
                                               gboolean        be_verbose);

static void        splash_timer_elapsed       (void);


/*  public functions  */

void
splash_create (gboolean   be_verbose,
               GdkScreen *screen,
               gint       monitor)
{
  GtkWidget          *frame;
  GtkWidget          *vbox;
  GdkPixbufAnimation *pixbuf;

  g_return_if_fail (splash == NULL);
  g_return_if_fail (GDK_IS_SCREEN (screen));

  pixbuf = splash_image_load (be_verbose);

  if (! pixbuf)
    return;

  splash = g_slice_new0 (GimpSplash);

  splash->window =
    g_object_new (GTK_TYPE_WINDOW,
                  "type",            GTK_WINDOW_TOPLEVEL,
                  "type-hint",       GDK_WINDOW_TYPE_HINT_SPLASHSCREEN,
                  "title",           _("GIMP Startup"),
                  "role",            "gimp-startup",
                  "screen",          screen,
                  "window-position", GTK_WIN_POS_CENTER,
                  "resizable",       FALSE,
                  NULL);

  g_signal_connect_swapped (splash->window, "delete-event",
                            G_CALLBACK (exit),
                            GINT_TO_POINTER (0));

  splash->width  = MIN (gdk_pixbuf_animation_get_width (pixbuf),
                        gdk_screen_get_width (screen));
  splash->height = MIN (gdk_pixbuf_animation_get_height (pixbuf),
                        gdk_screen_get_height (screen));

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (splash->window), frame);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  splash->area = gtk_image_new_from_animation (pixbuf);
  gtk_box_pack_start (GTK_BOX (vbox), splash->area, TRUE, TRUE, 0);
  gtk_widget_show (splash->area);

  gtk_widget_set_size_request (splash->area, splash->width, splash->height);

  /*  create the pango layouts  */
  splash->upper = gtk_widget_create_pango_layout (splash->area, "");
  splash->lower = gtk_widget_create_pango_layout (splash->area, "");
  gimp_pango_layout_set_scale (splash->lower, PANGO_SCALE_SMALL);

  /*  this sets the initial layout positions  */
  splash_position_layouts (splash, "", "", NULL);

  splash_average_text_area (splash,
                            gdk_pixbuf_animation_get_static_image (pixbuf),
                            &splash->color);

  g_object_unref (pixbuf);

  g_signal_connect_after (splash->area, "draw",
			  G_CALLBACK (splash_area_draw),
			  splash);

  /*  add a progress bar  */
  splash->progress = gtk_progress_bar_new ();
  gtk_box_pack_end (GTK_BOX (vbox), splash->progress, FALSE, FALSE, 0);
  gtk_widget_show (splash->progress);

  gtk_widget_show (splash->window);

  if (FALSE)
    splash->timer = g_timer_new ();
}

void
splash_destroy (void)
{
  if (! splash)
    return;

  gtk_widget_destroy (splash->window);

  g_object_unref (splash->upper);
  g_object_unref (splash->lower);

  g_free (splash->text1);
  g_free (splash->text2);

  if (splash->timer)
    g_timer_destroy (splash->timer);

  g_slice_free (GimpSplash, splash);
  splash = NULL;
}

void
splash_update (const gchar *text1,
               const gchar *text2,
               gdouble      percentage)
{
  GdkRectangle expose = { 0, 0, 0, 0 };

  g_return_if_fail (percentage >= 0.0 && percentage <= 1.0);

  if (! splash)
    return;

  splash_position_layouts (splash, text1, text2, &expose);

  if (expose.width > 0 && expose.height > 0)
    gtk_widget_queue_draw_area (splash->area,
                                expose.x, expose.y,
                                expose.width, expose.height);

  if ((text1 == NULL || ! g_strcmp0 (text1, splash->text1)) &&
      (text2 == NULL || ! g_strcmp0 (text2, splash->text2)) &&
      percentage == splash->percentage)
    {
      if (text1)
        {
          g_free (splash->text1);
          splash->text1 = g_strdup (text1);
        }

      if (text2)
        {
          g_free (splash->text2);
          splash->text2 = g_strdup (text2);
        }

      gtk_progress_bar_pulse (GTK_PROGRESS_BAR (splash->progress));
    }
  else
    {
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (splash->progress),
                                     percentage);
    }

  splash->percentage = percentage;

  if (splash->timer)
    splash_timer_elapsed ();

  if (gtk_events_pending ())
    gtk_main_iteration ();
}


/*  private functions  */

static gboolean
splash_area_draw (GtkWidget  *widget,
                  cairo_t    *cr,
                  GimpSplash *splash)
{
  gdk_cairo_set_source_rgba (cr, &splash->color);

  cairo_move_to (cr, splash->upper_x, splash->upper_y);
  pango_cairo_show_layout (cr, splash->upper);

  cairo_move_to (cr, splash->lower_x, splash->lower_y);
  pango_cairo_show_layout (cr, splash->lower);

  return FALSE;
}

/* area returns the union of the previous and new ink rectangles */
static void
splash_position_layouts (GimpSplash   *splash,
                         const gchar  *text1,
                         const gchar  *text2,
                         GdkRectangle *area)
{
  PangoRectangle  ink;
  PangoRectangle  logical;

  if (text1)
    {
      pango_layout_get_pixel_extents (splash->upper, &ink, NULL);

      if (area)
        splash_rectangle_union (area, &ink, splash->upper_x, splash->upper_y);

      pango_layout_set_text (splash->upper, text1, -1);
      pango_layout_get_pixel_extents (splash->upper, &ink, &logical);

      splash->upper_x = (splash->width - logical.width) / 2;
      splash->upper_y = splash->height - (2 * logical.height + 6);

      if (area)
        splash_rectangle_union (area, &ink, splash->upper_x, splash->upper_y);
    }

  if (text2)
    {
      pango_layout_get_pixel_extents (splash->lower, &ink, NULL);

      if (area)
        splash_rectangle_union (area, &ink, splash->lower_x, splash->lower_y);

      pango_layout_set_text (splash->lower, text2, -1);
      pango_layout_get_pixel_extents (splash->lower, &ink, &logical);

      splash->lower_x = (splash->width - logical.width) / 2;
      splash->lower_y = splash->height - (logical.height + 6);

      if (area)
        splash_rectangle_union (area, &ink, splash->lower_x, splash->lower_y);
    }
}

static void
splash_rectangle_union (GdkRectangle   *dest,
                        PangoRectangle *pango_rect,
                        gint            offset_x,
                        gint            offset_y)
{
  GdkRectangle  rect;

  rect.x      = pango_rect->x + offset_x;
  rect.y      = pango_rect->y + offset_y;
  rect.width  = pango_rect->width;
  rect.height = pango_rect->height;

  if (dest->width > 0 && dest->height > 0)
    gdk_rectangle_union (dest, &rect, dest);
  else
    *dest = rect;
}

/* This function chooses a gray value for the text color, based on
 * the average luminance of the text area of the splash image.
 */
static void
splash_average_text_area (GimpSplash *splash,
                          GdkPixbuf  *pixbuf,
                          GdkRGBA    *color)
{
  const guchar *pixels;
  gint          rowstride;
  gint          channels;
  gint          luminance = 0;
  guint         sum[3]    = { 0, 0, 0 };
  GdkRectangle  image     = { 0, 0, 0, 0 };
  GdkRectangle  area      = { 0, 0, 0, 0 };

  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
  g_return_if_fail (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);

  image.width  = gdk_pixbuf_get_width (pixbuf);
  image.height = gdk_pixbuf_get_height (pixbuf);
  rowstride    = gdk_pixbuf_get_rowstride (pixbuf);
  channels     = gdk_pixbuf_get_n_channels (pixbuf);
  pixels       = gdk_pixbuf_get_pixels (pixbuf);

  splash_position_layouts (splash, "Short text", "Somewhat longer text", &area);
  splash_position_layouts (splash, "", "", NULL);

  if (gdk_rectangle_intersect (&image, &area, &area))
    {
      const gint count = area.width * area.height;
      gint       x, y;

      pixels += area.x * channels;
      pixels += area.y * rowstride;

      for (y = 0; y < area.height; y++)
        {
          const guchar *src = pixels;

          for (x = 0; x < area.width; x++)
            {
              sum[0] += src[0];
              sum[1] += src[1];
              sum[2] += src[2];

              src += channels;
            }

          pixels += rowstride;
        }

      luminance = GIMP_RGB_LUMINANCE (sum[0] / count,
                                      sum[1] / count,
                                      sum[2] / count);

      luminance = CLAMP0255 (luminance > 127 ?
                             luminance - 223 : luminance + 223);

    }

  color->red = color->green = color->blue = (luminance << 8 | luminance) / 255.0;
  color->alpha = 1.0;
}

static GdkPixbufAnimation *
splash_image_load (gboolean be_verbose)
{
  GdkPixbufAnimation *pixbuf;
  gchar              *filename;

  filename = gimp_personal_rc_file ("gimp-splash.png");

  if (be_verbose)
    g_printerr ("Trying splash '%s' ... ", filename);

  pixbuf = gdk_pixbuf_animation_new_from_file (filename, NULL);
  g_free (filename);

  if (be_verbose)
    g_printerr (pixbuf ? "OK\n" : "failed\n");

  if (pixbuf)
    return pixbuf;

  filename = gimp_personal_rc_file ("splashes");
  pixbuf = splash_image_pick_from_dir (filename, be_verbose);
  g_free (filename);

  if (pixbuf)
    return pixbuf;

  filename = g_build_filename (gimp_data_directory (),
                               "images", "gimp-splash.png", NULL);

  if (be_verbose)
    g_printerr ("Trying splash '%s' ... ", filename);

  pixbuf = gdk_pixbuf_animation_new_from_file (filename, NULL);
  g_free (filename);

  if (be_verbose)
    g_printerr (pixbuf ? "OK\n" : "failed\n");

  if (pixbuf)
    return pixbuf;

  filename = g_build_filename (gimp_data_directory (), "splashes", NULL);
  pixbuf = splash_image_pick_from_dir (filename, be_verbose);
  g_free (filename);

  return pixbuf;
}

static GdkPixbufAnimation *
splash_image_pick_from_dir (const gchar *dirname,
                            gboolean     be_verbose)
{
  GdkPixbufAnimation *pixbuf = NULL;
  GDir               *dir    = g_dir_open (dirname, 0, NULL);

  if (dir)
    {
      const gchar *entry;
      GList       *splashes = NULL;

      while ((entry = g_dir_read_name (dir)))
        splashes = g_list_prepend (splashes, g_strdup (entry));

      g_dir_close (dir);

      if (splashes)
        {
          gint32  i        = g_random_int_range (0, g_list_length (splashes));
          gchar  *filename = g_build_filename (dirname,
                                               g_list_nth_data (splashes, i),
                                               NULL);

          if (be_verbose)
            g_printerr ("Trying splash '%s' ... ", filename);

          pixbuf = gdk_pixbuf_animation_new_from_file (filename, NULL);
          g_free (filename);

          if (be_verbose)
            g_printerr (pixbuf ? "OK\n" : "failed\n");

          g_list_free_full (splashes, (GDestroyNotify) g_free);
        }
    }

  return pixbuf;
}

static void
splash_timer_elapsed (void)
{
  gdouble elapsed = g_timer_elapsed (splash->timer, NULL);

  g_printerr ("%8g  %8g  -  %s %g%%  -  %s\n",
              elapsed,
              elapsed - splash->last_time,
              splash->text1 ? splash->text1 : "",
              splash->percentage * 100.0,
              splash->text2 ? splash->text2 : "");

  splash->last_time = elapsed;
}
