/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppixbuf.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gimp.h"

#include "gimppixbuf.h"


static GdkPixbuf * gimp_pixbuf_from_data (guchar                 *data,
                                          gint                    width,
                                          gint                    height,
                                          gint                    bpp,
                                          GimpPixbufTransparency  alpha);


/**
 * gimp_image_get_thumbnail:
 * @image_ID: the image ID
 * @width:    the requested thumbnail width  (<= 512 pixels)
 * @height:   the requested thumbnail height (<= 512 pixels)
 * @alpha:    how to handle an alpha channel
 *
 * Retrieves a thumbnail pixbuf for the image identified by @image_ID.
 * The thumbnail will be not larger than the requested size.
 *
 * Return value: a new #GdkPixbuf
 *
 * Since: GIMP 2.2
 **/
GdkPixbuf *
gimp_image_get_thumbnail (gint32                  image_ID,
                          gint                    width,
                          gint                    height,
                          GimpPixbufTransparency  alpha)
{
  gint    thumb_width;
  gint    thumb_height;
  gint    thumb_bpp;
  gint    data_size;
  guchar *data;

  g_return_val_if_fail (width  > 0 && width  <= 512, NULL);
  g_return_val_if_fail (height > 0 && height <= 512, NULL);

  if (! _gimp_image_thumbnail (image_ID,
                               width, height,
                               &thumb_width, &thumb_height, &thumb_bpp,
                               &data_size, &data))
    return NULL;

  g_return_val_if_fail (data_size == (thumb_width * thumb_height * thumb_bpp),
                        NULL);

  return gimp_pixbuf_from_data (data,
                                   thumb_width, thumb_height, thumb_bpp,
                                   alpha);
}

/**
 * gimp_drawable_get_thumbnail:
 * @drawable_ID: the drawable ID
 * @width:       the requested thumbnail width  (<= 512 pixels)
 * @height:      the requested thumbnail height (<= 512 pixels)
 * @alpha:       how to handle an alpha channel
 *
 * Retrieves a thumbnail pixbuf for the drawable identified by
 * @drawable_ID. The thumbnail will be not larger than the requested
 * size.
 *
 * Return value: a new #GdkPixbuf
 *
 * Since: GIMP 2.2
 **/
GdkPixbuf *
gimp_drawable_get_thumbnail (gint32                  drawable_ID,
                             gint                    width,
                             gint                    height,
                             GimpPixbufTransparency  alpha)
{
  gint    thumb_width;
  gint    thumb_height;
  gint    thumb_bpp;
  gint    data_size;
  guchar *data;

  g_return_val_if_fail (width  > 0 && width  <= 512, NULL);
  g_return_val_if_fail (height > 0 && height <= 512, NULL);

  if (! _gimp_drawable_thumbnail (drawable_ID,
                                  width, height,
                                  &thumb_width, &thumb_height, &thumb_bpp,
                                  &data_size, &data))
    return NULL;

  g_return_val_if_fail (data_size == (thumb_width * thumb_height * thumb_bpp),
                        NULL);

  return gimp_pixbuf_from_data (data,
                                thumb_width, thumb_height, thumb_bpp, alpha);
}

static GdkPixbuf *
gimp_pixbuf_from_data (guchar                 *data,
                       gint                    width,
                       gint                    height,
                       gint                    bpp,
                       GimpPixbufTransparency  alpha)
{
  GdkPixbuf *pixbuf;

  g_return_val_if_fail (bpp == 3 || bpp == 4, NULL);

  pixbuf = gdk_pixbuf_new_from_data (data,
                                     GDK_COLORSPACE_RGB, (bpp == 4), 8,
                                     width, height, width * bpp,
                                     (GdkPixbufDestroyNotify) g_free, NULL);

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
