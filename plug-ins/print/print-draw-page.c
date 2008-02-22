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


static inline void  convert_from_rgb  (const guchar *src,
                                       guchar       *dest,
                                       gint          pixels);
static inline void  convert_from_rgba (const guchar *src,
                                       guchar       *dest,
                                       gint          pixels);


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
  const gint       width    = drawable->width;
  const gint       height   = drawable->height;
  gint             stride;
  gint             y;
  gpointer         pr;
  gdouble          scale_x;
  gdouble          scale_y;
  guint            count    = 0;
  guint            done     = 0;

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

  surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height);

  pixels = cairo_image_surface_get_data (surface);
  stride = cairo_image_surface_get_stride (surface);

  gimp_pixel_rgn_init (&region, drawable, 0, 0, width, height, FALSE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &region);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      const guchar *src  = region.data;
      guchar       *dest = pixels + region.y * stride + region.x * 4;

      for (y = 0; y < region.h; y++)
        {
          switch (drawable->bpp)
            {
            case 3:
              convert_from_rgb (src, dest, region.w);
              break;

            case 4:
              convert_from_rgba (src, dest, region.w);
              break;
            }

          src  += region.rowstride;
          dest += stride;
        }

      done += region.h * region.w;

      if (count++ % 16 == 0)
        gimp_progress_update ((gdouble) done / (width * height));
    }

  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);
  cairo_surface_destroy (surface);

  gimp_progress_update (1.0);

  gimp_drawable_detach (drawable);

  return TRUE;
}


#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))
#define INT_BLEND(a,b,alpha,tmp)  (INT_MULT((a) - (b), alpha, tmp) + (b))

static inline void
convert_from_rgb (const guchar *src,
                  guchar       *dest,
                  gint          pixels)
{
  while (pixels--)
    {
      GIMP_CAIRO_RGB24_SET_PIXEL (dest, src[0], src[1], src[2]);

      src  += 3;
      dest += 4;
    }
}

static inline void
convert_from_rgba (const guchar *src,
                   guchar       *dest,
                   gint          pixels)
{
  while (pixels--)
    {
      guint32 r = src[0];
      guint32 g = src[1];
      guint32 b = src[2];
      guint32 a = src[3];

      if (a != 255)
        {
          guint32 tmp;

          /* composite on a white background */

          r = INT_BLEND (r, 255, a, tmp);
          g = INT_BLEND (g, 255, a, tmp);
          b = INT_BLEND (b, 255, a, tmp);
        }

      GIMP_CAIRO_RGB24_SET_PIXEL (dest, r, g, b);

      src  += 4;
      dest += 4;
    }
}
