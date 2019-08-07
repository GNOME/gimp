/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppixbuf.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

#include "gimppixbuf.h"


/*
 * The data that is passed to this function is either freed here or
 * owned by the returned pixbuf.
 */
GdkPixbuf *
_gimp_pixbuf_from_data (guchar                 *data,
                        gint                    width,
                        gint                    height,
                        gint                    bpp,
                        GimpPixbufTransparency  alpha)
{
  GdkPixbuf *pixbuf;

  switch (bpp)
    {
    case 1:
      pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);
      {
        guchar *src       = data;
        guchar *pixels    = gdk_pixbuf_get_pixels (pixbuf);
        gint    rowstride = gdk_pixbuf_get_rowstride (pixbuf);
        gint    y;

        for (y = 0; y < height; y++)
          {
            guchar *dest = pixels;
            gint    x;

            for (x = 0; x < width; x++, src += 1, dest += 3)
              {
                dest[0] = dest[1] = dest[2] = src[0];
              }

            pixels += rowstride;
          }

        g_free (data);
      }
      bpp = 3;
      break;

    case 2:
      pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
      {
        guchar *src       = data;
        guchar *pixels    = gdk_pixbuf_get_pixels (pixbuf);
        gint    rowstride = gdk_pixbuf_get_rowstride (pixbuf);
        gint    y;

        for (y = 0; y < height; y++)
          {
            guchar *dest = pixels;
            gint    x;

            for (x = 0; x < width; x++, src += 2, dest += 4)
              {
                dest[0] = dest[1] = dest[2] = src[0];
                dest[3] = src[1];
              }

            pixels += rowstride;
          }

        g_free (data);
      }
      bpp = 4;
      break;

    case 3:
      pixbuf = gdk_pixbuf_new_from_data (data,
                                         GDK_COLORSPACE_RGB, FALSE, 8,
                                         width, height, width * bpp,
                                         (GdkPixbufDestroyNotify) g_free, NULL);
      break;

    case 4:
      pixbuf = gdk_pixbuf_new_from_data (data,
                                         GDK_COLORSPACE_RGB, TRUE, 8,
                                         width, height, width * bpp,
                                         (GdkPixbufDestroyNotify) g_free, NULL);
      break;

    default:
      g_return_val_if_reached (NULL);
      return NULL;
    }

  if (bpp == 4)
    {
      GdkPixbuf *tmp;
      gint       check_size  = 0;

      switch (alpha)
        {
        case GIMP_PIXBUF_KEEP_ALPHA:
          return pixbuf;

        case GIMP_PIXBUF_SMALL_CHECKS:
          check_size = GIMP_CHECK_SIZE_SM;
          break;

        case GIMP_PIXBUF_LARGE_CHECKS:
          check_size = GIMP_CHECK_SIZE;
          break;
        }

      tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);

      gdk_pixbuf_composite_color (pixbuf, tmp,
                                  0, 0, width, height, 0, 0, 1.0, 1.0,
                                  GDK_INTERP_NEAREST, 255,
                                  0, 0, check_size, 0x66666666, 0x99999999);

      g_object_unref (pixbuf);
      pixbuf = tmp;
    }

  return pixbuf;
}
