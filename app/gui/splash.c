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


#define MEASURE_UPPER "1235678901234567890"
#define MEASURE_LOWER "12356789012345678901234567890"


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


static void        splash_position_layouts     (GimpSplash     *splash,
                                                const gchar    *text1,
                                                const gchar    *text2,
                                                GdkRectangle   *area);
static gboolean    splash_area_draw            (GtkWidget      *widget,
                                                cairo_t        *cr,
                                                GimpSplash     *splash);
static void        splash_rectangle_union      (GdkRectangle   *dest,
                                                PangoRectangle *pango_rect,
                                                gint            offset_x,
                                                gint            offset_y);
static void        splash_average_text_area    (GimpSplash     *splash,
                                                GdkPixbuf      *pixbuf,
                                                GdkRGBA        *rgba);

static GdkPixbufAnimation *
                   splash_image_load           (gint            max_width,
                                                gint            max_height,
                                                gboolean        be_verbose);
static GdkPixbufAnimation *
                   splash_image_load_from_file (const gchar    *filename,
                                                gint            max_width,
                                                gint            max_height,
                                                gboolean        be_verbose);
static GdkPixbufAnimation *
                   splash_image_pick_from_dir  (const gchar    *dirname,
                                                gint            max_width,
                                                gint            max_height,
                                                gboolean        be_verbose);

static void        splash_timer_elapsed        (void);


/*  public functions  */

void
splash_create (gboolean    be_verbose,
               GdkMonitor *monitor)
{
  GtkWidget          *frame;
  GtkWidget          *vbox;
  GdkPixbufAnimation *pixbuf;
  PangoRectangle      ink;
  GdkRectangle        workarea;
  gint                max_width;
  gint                max_height;

  g_return_if_fail (splash == NULL);
  g_return_if_fail (GDK_IS_MONITOR (monitor));

  gdk_monitor_get_workarea (monitor, &workarea);

  max_width  = workarea.width  / 2;
  max_height = workarea.height / 2;
  pixbuf = splash_image_load (max_width, max_height, be_verbose);

  if (! pixbuf)
    return;

  splash = g_slice_new0 (GimpSplash);

  splash->window =
    g_object_new (GTK_TYPE_WINDOW,
                  "type",            GTK_WINDOW_TOPLEVEL,
                  "type-hint",       GDK_WINDOW_TYPE_HINT_SPLASHSCREEN,
                  "title",           _("GIMP Startup"),
                  "role",            "gimp-startup",
                  "window-position", GTK_WIN_POS_CENTER,
                  "resizable",       FALSE,
                  NULL);

  g_signal_connect_swapped (splash->window, "delete-event",
                            G_CALLBACK (exit),
                            GINT_TO_POINTER (0));

  splash->width  = MIN (gdk_pixbuf_animation_get_width (pixbuf),
                        workarea.width);
  splash->height = MIN (gdk_pixbuf_animation_get_height (pixbuf),
                        workarea.height);

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
  splash->upper = gtk_widget_create_pango_layout (splash->area,
                                                  MEASURE_UPPER);
  pango_layout_get_pixel_extents (splash->upper, &ink, NULL);

  if (splash->width > 4 * ink.width)
    gimp_pango_layout_set_scale (splash->upper, PANGO_SCALE_X_LARGE);
  else if (splash->width > 3 * ink.width)
    gimp_pango_layout_set_scale (splash->upper, PANGO_SCALE_LARGE);
  else if (splash->width > 2 * ink.width)
    gimp_pango_layout_set_scale (splash->upper, PANGO_SCALE_MEDIUM);
  else
    gimp_pango_layout_set_scale (splash->upper, PANGO_SCALE_MEDIUM);

  splash->lower = gtk_widget_create_pango_layout (splash->area,
                                                  MEASURE_LOWER);
  pango_layout_get_pixel_extents (splash->lower, &ink, NULL);

  if (splash->width > 4 * ink.width)
    gimp_pango_layout_set_scale (splash->lower, PANGO_SCALE_LARGE);
  else if (splash->width > 3 * ink.width)
    gimp_pango_layout_set_scale (splash->lower, PANGO_SCALE_MEDIUM);
  else if (splash->width > 2 * ink.width)
    gimp_pango_layout_set_scale (splash->lower, PANGO_SCALE_SMALL);
  else
    gimp_pango_layout_set_scale (splash->lower, PANGO_SCALE_X_SMALL);

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
  PangoRectangle  upper_ink;
  PangoRectangle  upper_logical;
  PangoRectangle  lower_ink;
  PangoRectangle  lower_logical;
  gint            text_height = 0;

  if (text1)
    {
      pango_layout_get_pixel_extents (splash->upper, &upper_ink, NULL);

      if (area)
        splash_rectangle_union (area, &upper_ink,
                                splash->upper_x, splash->upper_y);

      pango_layout_set_text (splash->upper, text1, -1);
      pango_layout_get_pixel_extents (splash->upper,
                                      &upper_ink, &upper_logical);

      splash->upper_x = (splash->width - upper_logical.width) / 2;
      text_height += upper_logical.height;
    }

  if (text2)
    {
      pango_layout_get_pixel_extents (splash->lower, &lower_ink, NULL);

      if (area)
        splash_rectangle_union (area, &lower_ink,
                                splash->lower_x, splash->lower_y);

      pango_layout_set_text (splash->lower, text2, -1);
      pango_layout_get_pixel_extents (splash->lower,
                                      &lower_ink, &lower_logical);

      splash->lower_x = (splash->width - lower_logical.width) / 2;
      text_height += lower_logical.height;
    }

  /* For pretty printing, let's say we want at least double space. */
  text_height *= 2;

  /* The ordinates are computed in 2 steps, because we are first
   * checking the minimal height needed for text (text_height).
   *
   * Ideally we are printing in the bottom quarter of the splash image,
   * with well centered positions. But if this zone appears to be too
   * small, we will end up using this previously computed text_height
   * instead. Since splash images are designed to have text in the lower
   * quarter, this may end up a bit uglier, but at least top and bottom
   * texts won't overlay each other.
   */
  if (text1)
    {
      splash->upper_y = MIN (splash->height - text_height,
                             splash->height * 13 / 16 -
                             upper_logical.height / 2);

      if (area)
        splash_rectangle_union (area, &upper_ink,
                                splash->upper_x, splash->upper_y);
    }

  if (text2)
    {
      splash->lower_y = ((splash->height + splash->upper_y) / 2 -
                         lower_logical.height / 2);

      if (area)
        splash_rectangle_union (area, &lower_ink,
                                splash->lower_x, splash->lower_y);
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

  splash_position_layouts (splash, MEASURE_UPPER, MEASURE_LOWER, &area);
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

  color->red = color->green = color->blue = luminance / 255.0;
  color->alpha = 1.0;
}

static GdkPixbufAnimation *
splash_image_load (gint     max_width,
                   gint     max_height,
                   gboolean be_verbose)
{
  GdkPixbufAnimation *animation = NULL;
  gchar              *filename;

  /* File "gimp-splash.png" in personal configuration directory. */
  filename = gimp_personal_rc_file ("gimp-splash.png");
  animation = splash_image_load_from_file (filename,
                                           max_width, max_height,
                                           be_verbose);
  g_free (filename);
  if (animation)
    return animation;

  /* Random image under splashes/ directory in personal config dir. */
  filename = gimp_personal_rc_file ("splashes");
  animation = splash_image_pick_from_dir (filename,
                                          max_width, max_height,
                                          be_verbose);
  g_free (filename);
  if (animation)
    return animation;

  /* Release splash image. */
  filename = g_build_filename (gimp_data_directory (),
                               "images", "gimp-splash.png", NULL);
  animation = splash_image_load_from_file (filename,
                                           max_width, max_height,
                                           be_verbose);
  g_free (filename);
  if (animation)
    return animation;

  /* Random release image in installed splashes/ directory. */
  filename = g_build_filename (gimp_data_directory (), "splashes", NULL);
  animation = splash_image_pick_from_dir (filename,
                                          max_width, max_height,
                                          be_verbose);
  g_free (filename);

  return animation;
}

static GdkPixbufAnimation *
splash_image_load_from_file (const gchar *filename,
                             gint         max_width,
                             gint         max_height,
                             gboolean     be_verbose)
{
  GdkPixbufAnimation *animation;
  GFile              *file;
  GFileInfo          *info;
  gboolean            is_svg = FALSE;

  if (be_verbose)
    g_printerr ("Trying splash '%s' ... ", filename);

  file = g_file_new_for_path (filename);
  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                            G_FILE_QUERY_INFO_NONE, NULL, NULL);
  if (info)
    {
      const gchar *content_type;

      content_type = g_file_info_get_content_type (info);
      if (content_type)
        {
          gchar *mime_type;

          mime_type = g_content_type_get_mime_type (content_type);
          if (mime_type)
            {
              if (g_strcmp0 (mime_type, "image/svg+xml") == 0)
                {
                  /* We want to treat vector images differently than
                   * pixel images. We only scale down bitmaps, but we
                   * don't try to scale them up.
                   * On the other hand, we can always scale down and up
                   * vector images so that they end up in an ideal size
                   * in all cases.
                   */
                  is_svg = TRUE;
                }
              g_free (mime_type);
            }
        }
      g_object_unref (info);
    }
  g_object_unref (file);

  animation = gdk_pixbuf_animation_new_from_file (filename, NULL);

  /* FIXME Right now, we only try to scale static images.
   * Animated images may end up bigger than the expected max dimensions.
   */
  if (animation && gdk_pixbuf_animation_is_static_image (animation) &&
      (gdk_pixbuf_animation_get_width (animation) > max_width        ||
       gdk_pixbuf_animation_get_height (animation) > max_height      ||
       is_svg))
    {
      GdkPixbuf *pixbuf;

      pixbuf = gdk_pixbuf_new_from_file_at_size (filename,
                                                 max_width, max_height,
                                                 NULL);
      if (pixbuf)
        {
          GdkPixbufSimpleAnim *simple_anim = NULL;

          simple_anim = gdk_pixbuf_simple_anim_new (gdk_pixbuf_get_width (pixbuf),
                                                    gdk_pixbuf_get_height (pixbuf),
                                                    1.0);
          if (simple_anim)
            {
              gdk_pixbuf_simple_anim_add_frame (simple_anim, pixbuf);

              g_object_unref (animation);
              animation = GDK_PIXBUF_ANIMATION (simple_anim);
            }
          g_object_unref (pixbuf);
        }
    }

  if (be_verbose)
    g_printerr (animation ? "OK\n" : "failed\n");

  return animation;
}

static GdkPixbufAnimation *
splash_image_pick_from_dir (const gchar *dirname,
                            gint         max_width,
                            gint         max_height,
                            gboolean     be_verbose)
{
  GdkPixbufAnimation *animation = NULL;
  GDir               *dir       = g_dir_open (dirname, 0, NULL);

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

          animation = splash_image_load_from_file (filename,
                                                   max_width, max_height,
                                                   be_verbose);
          g_free (filename);
          g_list_free_full (splashes, (GDestroyNotify) g_free);
        }
    }

  return animation;
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
