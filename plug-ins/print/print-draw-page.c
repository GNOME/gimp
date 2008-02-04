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

#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))
#define INT_BLEND(a,b,alpha,tmp)  (INT_MULT((a) - (b), alpha, tmp) + (b))


static void  convert_from_rgb  (guchar *pixels,
                                gint    width);
static void  convert_from_rgba (guchar *pixels,
                                gint    width);


gboolean
draw_page_cairo (GtkPrintContext *context,
                 PrintData       *data)
{
  GimpDrawable    *drawable = gimp_drawable_get (data->drawable_id);
  GimpPixelRgn     region;
  cairo_t         *cr;
  cairo_surface_t *surface;
  guchar          *pixels;
  gdouble          cr_width;
  gdouble          cr_height;
  gdouble          cr_dpi_x;
  gdouble          cr_dpi_y;
  gint             width;
  gint             height;
  gint             stride;
  gint             y;
  gdouble          scale_x;
  gdouble          scale_y;

  width  = drawable->width;
  height = drawable->height;

  gimp_tile_cache_ntiles (width / gimp_tile_width () + 1);

  cr = gtk_print_context_get_cairo_context (context);

  cr_width  = gtk_print_context_get_width  (context);
  cr_height = gtk_print_context_get_height (context);
  cr_dpi_x  = gtk_print_context_get_dpi_x  (context);
  cr_dpi_y  = gtk_print_context_get_dpi_y  (context);

  scale_x = cr_dpi_x / data->xres;
  scale_y = cr_dpi_y / data->yres;

  cairo_translate (cr,
                   data->offset_x / cr_dpi_x * 72.0,
                   data->offset_y / cr_dpi_y * 72.0);
  cairo_scale (cr, scale_x, scale_y);

  gimp_pixel_rgn_init (&region, drawable, 0, 0, width, height, FALSE, FALSE);

  surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height);

  pixels = cairo_image_surface_get_data (surface);
  stride = cairo_image_surface_get_stride (surface);

  for (y = 0; y < height; y++, pixels += stride)
    {
      gimp_pixel_rgn_get_row (&region, pixels, 0, y, width);

      switch (drawable->bpp)
        {
        case 3:
          convert_from_rgb (pixels, width);
          break;
        case 4:
          convert_from_rgba (pixels, width);
          break;
        }

      if (y % 16 == 0)
        gimp_progress_update ((gdouble) y / (gdouble) height);
    }

  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);
  cairo_surface_destroy (surface);

  gimp_progress_update (1.0);

  gimp_drawable_detach (drawable);

  return TRUE;
}

static void
convert_from_rgb (guchar *pixels,
                  gint    width)
{
  guint32 *cairo_data = (guint32 *) pixels;
  guchar  *p;
  gint     i;

  for (i = width - 1, p = pixels + 3 * width - 1; i >= 0; i--)
    {
      guint32 b = *p--;
      guint32 g = *p--;
      guint32 r = *p--;

      cairo_data[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}

static void
convert_from_rgba (guchar *pixels,
                   gint    width)
{
  guint32 *cairo_data = (guint32 *) pixels;
  guchar  *p;
  gint     i;

  for (i = 0, p = pixels; i < width; i++)
    {
      guint32 r = *p++;
      guint32 g = *p++;
      guint32 b = *p++;
      guint32 a = *p++;

      if (a != 255)
        {
          guint32 tmp;

          /* composite on a white background */

          r = INT_BLEND (r, 255, a, tmp);
          g = INT_BLEND (g, 255, a, tmp);
          b = INT_BLEND (b, 255, a, tmp);
        }

      cairo_data[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}
