/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcairo-utils.c
 * Copyright (C) 2007 Sven Neumann <sven@gimp.org>
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpcairo-utils.h"


/**
 * gimp_cairo_set_source_color:
 * @cr:    Cairo context
 * @color: GimpRGB color
 *
 * Sets the source pattern within @cr to the color described by @color.
 *
 * This function calls cairo_set_source_rgba() for you.
 **/
void
gimp_cairo_set_source_color (cairo_t *cr,
                             GimpRGB *color)
{
  cairo_set_source_rgba (cr, color->r, color->g, color->b, color->a);
}

/**
 * gimp_cairo_create_surface_from_pixbuf:
 * @pixbuf: a GdkPixbuf
 *
 * Create a Cairo image surface from a GdkPixbuf.
 *
 * You should avoid calling this function as there are probably more
 * efficient ways of achieving the result you are looking for.
 **/
cairo_surface_t *
gimp_cairo_create_surface_from_pixbuf (GdkPixbuf *pixbuf)
{
  cairo_surface_t *surface;
  cairo_format_t   format;
  guchar          *dest;
  const guchar    *src;
  gint             width;
  gint             height;
  gint             y;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  switch (gdk_pixbuf_get_n_channels (pixbuf))
    {
    case 3:
      format = CAIRO_FORMAT_RGB24;
      break;
    case 4:
      format = CAIRO_FORMAT_ARGB32;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  surface = cairo_image_surface_create (format, width, height);

  src  = gdk_pixbuf_get_pixels (pixbuf);
  dest = cairo_image_surface_get_data (surface);

  switch (format)
    {
    case CAIRO_FORMAT_RGB24:
      for (y = 0; y < height; y++)
        {
          const guchar *s = src;
          guchar       *d = dest;
          gint          w = width;

          while (w--)
            {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	      d[0] = s[2];
	      d[1] = s[1];
	      d[2] = s[0];
#else
	      d[1] = s[0];
	      d[2] = s[1];
	      d[3] = s[2];
#endif

              s += 3;
              d += 4;
            }

          src  += gdk_pixbuf_get_rowstride (pixbuf);
          dest += cairo_image_surface_get_stride (surface);
        }
      break;

    case CAIRO_FORMAT_ARGB32:
      for (y = 0; y < height; y++)
        {
          const guchar *s = src;
          guchar       *d = dest;
          gint          w = width;
	  guint         t1, t2, t3;

#define MULT(d,c,a,t) \
   G_STMT_START { t = c * a + 0x7f; d = ((t >> 8) + t) >> 8; } G_STMT_END

          while (w--)
            {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	      MULT (d[0], s[2], s[3], t1);
	      MULT (d[1], s[1], s[3], t2);
	      MULT (d[2], s[0], s[3], t3);
	      d[3] = s[3];
#else
	      d[0] = s[3];
	      MULT (d[1], s[0], s[3], t1);
	      MULT (d[2], s[1], s[3], t2);
	      MULT (d[3], s[2], s[3], t3);
#endif
              s += 4;
              d += 4;
            }

#undef MULT

          src  += gdk_pixbuf_get_rowstride (pixbuf);
          dest += cairo_image_surface_get_stride (surface);
        }
      break;

    default:
      break;
    }

  return surface;
}
