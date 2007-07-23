/* GIMP - The GNU Image Manipulation Program
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print.h"
#include "print-draw-page.h"

#include "libgimp/stdplugins-intl.h"


#define EPSILON 0.0001


static cairo_surface_t * create_surface_from_rgb  (guchar          *pixels,
                                                   gint             width,
                                                   gint             height);
static cairo_surface_t * create_surface_from_rgba (guchar          *pixels,
                                                   gint             width,
                                                   gint             height);

#if 0
static void              draw_info_header         (GtkPrintContext *context,
                                                   cairo_t         *cr,
                                                   PrintData       *data);
#endif


gboolean
draw_page_cairo (GtkPrintContext *context,
                 PrintData       *data)
{
  const gint        tile_height = gimp_tile_height ();
  GimpDrawable     *drawable;
  GimpPixelRgn      region;
  cairo_t          *cr;
  cairo_surface_t  *surface     = NULL;
  guchar           *pixels;
  gdouble           cr_width;
  gdouble           cr_height;
  gdouble           cr_dpi_x;
  gdouble           cr_dpi_y;
  gint              width;
  gint              height;
  gint              y;
  gdouble           scale_x;
  gdouble           scale_y;

  drawable = gimp_drawable_get (data->drawable_id);

  width     = drawable->width;
  height    = drawable->height;

  cr = gtk_print_context_get_cairo_context (context);

  cr_width  = gtk_print_context_get_width  (context);
  cr_height = gtk_print_context_get_height (context);
  cr_dpi_x  = gtk_print_context_get_dpi_x  (context);
  cr_dpi_y  = gtk_print_context_get_dpi_y  (context);

  scale_x = cr_dpi_x / data->xres;
  scale_y = cr_dpi_y / data->yres;

#if 0
  /* print header if it is requested */
  if (data->show_info_header)
    {
      draw_info_header (context, cr, data);

/* In points */
#define HEADER_HEIGHT (20 * 72.0 / 25.4)
      cairo_translate (cr, 0, HEADER_HEIGHT);
      cr_height -= HEADER_HEIGHT;
    }
#endif

  cairo_translate (cr,
                   data->offset_x / cr_dpi_x * 72.0,
                   data->offset_y / cr_dpi_y * 72.0);
  cairo_scale (cr, scale_x, scale_y);

  gimp_pixel_rgn_init (&region, drawable, 0, 0, width, height, FALSE, FALSE);

  pixels = g_new (guchar, MIN (height, tile_height) * 4 * width);

  for (y = 0; y < height; y += tile_height)
    {
      gint h = MIN (tile_height, height - y);

      gimp_pixel_rgn_get_rect (&region, pixels, 0, y, width, h);

      switch (drawable->bpp)
        {
        case 3:
          surface = create_surface_from_rgb (pixels, width, h);
          break;
        case 4:
          surface = create_surface_from_rgba (pixels, width, h);
          break;
        }

      cairo_set_source_surface (cr, surface, 0, y);

      cairo_rectangle (cr, 0, y, width, h);
      cairo_fill (cr);

      cairo_surface_destroy (surface);

      gimp_progress_update ((gdouble) (y + h) / (gdouble) height);
    }

  g_free (pixels);

  gimp_drawable_detach (drawable);

  return TRUE;
}

static cairo_surface_t *
create_surface_from_rgb (guchar *pixels,
                         gint    width,
                         gint    height)
{
  guint32      *cairo_data = (guint32 *) pixels;
  const guchar *p;
  gsize         len;
  gint          i;

  len = width * height;

  for (i = len - 1, p = pixels + 3 * len - 1; i >= 0; i--)
    {
      guint32 b = *p--;
      guint32 g = *p--;
      guint32 r = *p--;

      cairo_data[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }

  return cairo_image_surface_create_for_data (pixels, CAIRO_FORMAT_RGB24,
                                              width, height, 4 * width);
}

static cairo_surface_t *
create_surface_from_rgba (guchar *pixels,
                          gint    width,
                          gint    height)
{
  guint32      *cairo_data = (guint32 *) pixels;
  const guchar *p;
  gsize         len;
  gsize         i;

  len = width * height;

  for (i = 0, p = pixels; i < len; i++)
    {
      guint32 r = *p++;
      guint32 g = *p++;
      guint32 b = *p++;
      guint32 a = *p++;

      if (a != 255)
        {
          gdouble alpha = a / 255.0;

          /* composite on a white background */

          r = (guint) (alpha * r + 255 - a) & 0xFF;
          g = (guint) (alpha * g + 255 - a) & 0xFF;
          b = (guint) (alpha * b + 255 - a) & 0xFF;
        }

      cairo_data[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }

  return cairo_image_surface_create_for_data (pixels, CAIRO_FORMAT_RGB24,
                                              width, height, 4 * width);
}

#if 0
static void
draw_info_header (GtkPrintContext *context,
                  cairo_t         *cr,
                  PrintData       *data)
{
  PangoLayout          *layout;
  PangoFontDescription *desc;
  gdouble               text_height;
  gdouble               text_width;
  gdouble               fname_text_width;
  gint                  layout_height;
  gint                  layout_width;
  gchar                 date_buffer[100];
  GDate                *date;
  const gchar          *name_str;
  GimpParasite         *parasite;
  const gchar          *end_ptr;
  gchar                *filename;
  gdouble               cr_width;

  cairo_save (cr);

  cr_width  = gtk_print_context_get_width (context);
  cairo_rectangle (cr, 0, 0, cr_width, HEADER_HEIGHT);
  cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
  cairo_fill_preserve (cr);

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_set_line_width (cr, 1);
  cairo_stroke (cr);

  layout = gtk_print_context_create_pango_layout (context);

  desc = pango_font_description_from_string ("sans 14");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_layout_set_width (layout, -1);
  pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);

  /* image name */
  pango_layout_set_text (layout, gimp_image_get_name (data->image_id), -1);

  pango_layout_get_size (layout, &layout_width, &layout_height);
  text_height = (gdouble) layout_height / PANGO_SCALE;

  cairo_move_to (cr, 0.02 * cr_width,  (HEADER_HEIGHT - text_height) / 5);
  pango_cairo_show_layout (cr, layout);

  /* user name */
  name_str = g_get_real_name ();
  if (name_str && g_utf8_validate (name_str, -1, &end_ptr))
    {
      pango_layout_set_text (layout, name_str, -1);

      pango_layout_get_size (layout, &layout_width, &layout_height);
      text_height = (gdouble) layout_height / PANGO_SCALE;
      text_width = (gdouble) layout_width / PANGO_SCALE;

      cairo_move_to (cr, 0.5 * cr_width - 0.5 * text_width,
                     (HEADER_HEIGHT - text_height) / 5);
      pango_cairo_show_layout (cr, layout);
    }

  /* date */
  date = g_date_new ();
  g_date_set_time_t (date, time (NULL));
  g_date_strftime (date_buffer, 100, "%x", date);
  g_date_free (date);
  pango_layout_set_text (layout, date_buffer, -1);

  pango_layout_get_size (layout, &layout_width, &layout_height);
  text_height = (gdouble) layout_height / PANGO_SCALE;
  text_width = (gdouble) layout_width / PANGO_SCALE;

  cairo_move_to (cr,
                 0.98 * cr_width - text_width,
                 (HEADER_HEIGHT - text_height) / 5);
  pango_cairo_show_layout (cr, layout);

  /* file name if any */
  filename = gimp_image_get_filename (data->image_id);

  if (filename)
    {
      pango_layout_set_text (layout,
                             gimp_filename_to_utf8 (filename), -1);
      g_free (filename);

      pango_layout_get_size (layout, &layout_width, &layout_height);
      text_height = (gdouble) layout_height / PANGO_SCALE;
      fname_text_width = (gdouble) layout_width / PANGO_SCALE;

      cairo_move_to (cr,
                     0.02 * cr_width,  4 * (HEADER_HEIGHT - text_height) / 5);
      pango_cairo_show_layout (cr, layout);
    }
  else
    {
      fname_text_width = 0;
    }

  /* image comment if it is short */
  parasite = gimp_image_parasite_find (data->image_id, "gimp-comment");

  if (parasite)
    {
      pango_layout_set_text (layout, gimp_parasite_data (parasite), -1);

      pango_layout_get_size (layout, &layout_width, &layout_height);
      text_height = (gdouble) layout_height / PANGO_SCALE;
      text_width = (gdouble) layout_width / PANGO_SCALE;

      if (fname_text_width + text_width < 0.8 * cr_width &&
          text_height < 0.5 * HEADER_HEIGHT)
        {
          cairo_move_to (cr, 0.98 * cr_width - text_width,
                         4 * (HEADER_HEIGHT - text_height) / 5);
          pango_cairo_show_layout (cr, layout);
        }

      gimp_parasite_free (parasite);
    }

  g_object_unref (layout);

  cairo_restore (cr);
}
#endif
