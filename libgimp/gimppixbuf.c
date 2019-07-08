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


/**
 * SECTION: gimppixbuf
 * @title: gimppixbuf
 * @short_description: Get a thumbnail pixbuf for a drawable or image.
 *
 * Get a thumbnail pixbuf for a drawable or image.
 **/


static GdkPixbuf * gimp_pixbuf_from_data (guchar                 *data,
                                          gint                    width,
                                          gint                    height,
                                          gint                    bpp,
                                          GimpPixbufTransparency  alpha);


/**
 * gimp_image_get_thumbnail:
 * @image_ID: the image ID
 * @width:    the requested thumbnail width  (<= 1024 pixels)
 * @height:   the requested thumbnail height (<= 1024 pixels)
 * @alpha:    how to handle an alpha channel
 *
 * Retrieves a thumbnail pixbuf for the image identified by @image_ID.
 * The thumbnail will be not larger than the requested size.
 *
 * Return value: a new #GdkPixbuf
 *
 * Since: 2.2
 **/
GdkPixbuf *
gimp_image_get_thumbnail (gint32                  image_ID,
                          gint                    width,
                          gint                    height,
                          GimpPixbufTransparency  alpha)
{
  gint    thumb_width  = width;
  gint    thumb_height = height;
  gint    thumb_bpp;
  guchar *data;

  g_return_val_if_fail (width  > 0 && width  <= 1024, NULL);
  g_return_val_if_fail (height > 0 && height <= 1024, NULL);

  data = gimp_image_get_thumbnail_data (image_ID,
                                        &thumb_width,
                                        &thumb_height,
                                        &thumb_bpp);
  if (data)
    return gimp_pixbuf_from_data (data,
                                  thumb_width, thumb_height, thumb_bpp,
                                  alpha);
  else
    return NULL;
}

/**
 * gimp_drawable_get_thumbnail:
 * @drawable_ID: the drawable ID
 * @width:       the requested thumbnail width  (<= 1024 pixels)
 * @height:      the requested thumbnail height (<= 1024 pixels)
 * @alpha:       how to handle an alpha channel
 *
 * Retrieves a thumbnail pixbuf for the drawable identified by
 * @drawable_ID. The thumbnail will be not larger than the requested
 * size.
 *
 * Return value: a new #GdkPixbuf
 *
 * Since: 2.2
 **/
GdkPixbuf *
gimp_drawable_get_thumbnail (gint32                  drawable_ID,
                             gint                    width,
                             gint                    height,
                             GimpPixbufTransparency  alpha)
{
  gint    thumb_width  = width;
  gint    thumb_height = height;
  gint    thumb_bpp;
  guchar *data;

  g_return_val_if_fail (width  > 0 && width  <= 1024, NULL);
  g_return_val_if_fail (height > 0 && height <= 1024, NULL);

  data = gimp_drawable_get_thumbnail_data (drawable_ID,
                                           &thumb_width,
                                           &thumb_height,
                                           &thumb_bpp);

  if (data)
    return gimp_pixbuf_from_data (data,
                                  thumb_width, thumb_height, thumb_bpp,
                                  alpha);

  return NULL;
}

/**
 * gimp_drawable_get_sub_thumbnail:
 * @drawable_ID: the drawable ID
 * @src_x:       the x coordinate of the area
 * @src_y:       the y coordinate of the area
 * @src_width:   the width of the area
 * @src_height:  the height of the area
 * @dest_width:  the requested thumbnail width  (<= 1024 pixels)
 * @dest_height: the requested thumbnail height (<= 1024 pixels)
 * @alpha:       how to handle an alpha channel
 *
 * Retrieves a thumbnail pixbuf for the drawable identified by
 * @drawable_ID. The thumbnail will be not larger than the requested
 * size.
 *
 * Return value: a new #GdkPixbuf
 *
 * Since: 2.2
 **/
GdkPixbuf *
gimp_drawable_get_sub_thumbnail (gint32                  drawable_ID,
                                 gint                    src_x,
                                 gint                    src_y,
                                 gint                    src_width,
                                 gint                    src_height,
                                 gint                    dest_width,
                                 gint                    dest_height,
                                 GimpPixbufTransparency  alpha)
{
  gint    thumb_width  = dest_width;
  gint    thumb_height = dest_height;
  gint    thumb_bpp;
  guchar *data;

  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0 && dest_width  <= 1024, NULL);
  g_return_val_if_fail (dest_height > 0 && dest_height <= 1024, NULL);

  data = gimp_drawable_get_sub_thumbnail_data (drawable_ID,
                                               src_x, src_y,
                                               src_width, src_height,
                                               &thumb_width,
                                               &thumb_height,
                                               &thumb_bpp);

  if (data)
    return gimp_pixbuf_from_data (data,
                                  thumb_width, thumb_height, thumb_bpp,
                                  alpha);

  return NULL;
}


/*
 * The data that is passed to this function is either freed here or
 * owned by the returned pixbuf.
 */
static GdkPixbuf *
gimp_pixbuf_from_data (guchar                 *data,
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
