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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gegl-utils.h>
#include <glib/gstdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "libgimpcolor/gimpcolor.h"

#include "gimptempbuf.h"


struct _GimpTempBuf
{
  gint        ref_count;
  gint        width;
  gint        height;
  const Babl *format;
  guchar     *data;
};


GimpTempBuf *
gimp_temp_buf_new (gint        width,
                   gint        height,
                   const Babl *format)
{
  GimpTempBuf *temp;

  g_return_val_if_fail (width > 0 && height > 0, NULL);
  g_return_val_if_fail (format != NULL, NULL);

  temp = g_slice_new (GimpTempBuf);

  temp->ref_count = 1;
  temp->width     = width;
  temp->height    = height;
  temp->format    = format;
  temp->data      = gegl_malloc (width * height *
                                 babl_format_get_bytes_per_pixel (format));

  return temp;
}

GimpTempBuf *
gimp_temp_buf_new_from_pixbuf (GdkPixbuf  *pixbuf,
                               const Babl *f_or_null)
{
  const Babl   *format = f_or_null;
  const Babl   *fish   = NULL;
  GimpTempBuf  *temp_buf;
  const guchar *pixels;
  gint          width;
  gint          height;
  gint          rowstride;
  gint          bpp;
  guchar       *data;
  gint          i;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  if (! format)
    format = gimp_pixbuf_get_format (pixbuf);

  pixels    = gdk_pixbuf_get_pixels (pixbuf);
  width     = gdk_pixbuf_get_width (pixbuf);
  height    = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  temp_buf  = gimp_temp_buf_new (width, height, format);
  data      = gimp_temp_buf_get_data (temp_buf);

  bpp       = babl_format_get_bytes_per_pixel (format);

  if (gimp_pixbuf_get_format (pixbuf) != format)
    fish = babl_fish (gimp_pixbuf_get_format (pixbuf), format);

  for (i = 0; i < height; i++)
    {
      if (fish)
        babl_process (fish, pixels, data, width);
      else
        memcpy (data, pixels, width * bpp);

      data   += width * bpp;
      pixels += rowstride;
    }

  return temp_buf;
}

GimpTempBuf *
gimp_temp_buf_copy (const GimpTempBuf *src)
{
  GimpTempBuf *dest;

  g_return_val_if_fail (src != NULL, NULL);

  dest = gimp_temp_buf_new (src->width, src->height, src->format);

  memcpy (gimp_temp_buf_get_data (dest),
          gimp_temp_buf_get_data (src),
          gimp_temp_buf_get_data_size (src));

  return dest;
}

GimpTempBuf *
gimp_temp_buf_ref (GimpTempBuf *buf)
{
  g_return_val_if_fail (buf != NULL, NULL);

  buf->ref_count++;

  return buf;
}

void
gimp_temp_buf_unref (GimpTempBuf *buf)
{
  g_return_if_fail (buf != NULL);
  g_return_if_fail (buf->ref_count > 0);

  buf->ref_count--;

  if (buf->ref_count < 1)
    {
      if (buf->data)
        gegl_free (buf->data);

      g_slice_free (GimpTempBuf, buf);
    }
}

GimpTempBuf *
gimp_temp_buf_scale (const GimpTempBuf *src,
                     gint               new_width,
                     gint               new_height)
{
  GimpTempBuf  *dest;
  const guchar *src_data;
  guchar       *dest_data;
  gdouble       x_ratio;
  gdouble       y_ratio;
  gint          bytes;
  gint          loop1;
  gint          loop2;

  g_return_val_if_fail (src != NULL, NULL);
  g_return_val_if_fail (new_width > 0 && new_height > 0, NULL);

  if (new_width == src->width && new_height == src->height)
    return gimp_temp_buf_copy (src);

  dest = gimp_temp_buf_new (new_width,
                            new_height,
                            src->format);

  src_data  = gimp_temp_buf_get_data (src);
  dest_data = gimp_temp_buf_get_data (dest);

  x_ratio = (gdouble) src->width  / (gdouble) new_width;
  y_ratio = (gdouble) src->height / (gdouble) new_height;

  bytes = babl_format_get_bytes_per_pixel (src->format);

  for (loop1 = 0 ; loop1 < new_height ; loop1++)
    {
      for (loop2 = 0 ; loop2 < new_width ; loop2++)
        {
          const guchar *src_pixel;
          guchar       *dest_pixel;
          gint          i;

          src_pixel = src_data +
            (gint) (loop2 * x_ratio) * bytes +
            (gint) (loop1 * y_ratio) * bytes * src->width;

          dest_pixel = dest_data +
            (loop2 + loop1 * new_width) * bytes;

          for (i = 0 ; i < bytes; i++)
            *dest_pixel++ = *src_pixel++;
        }
    }

  return dest;
}

gint
gimp_temp_buf_get_width (const GimpTempBuf *buf)
{
  return buf->width;
}

gint
gimp_temp_buf_get_height (const GimpTempBuf *buf)
{
  return buf->height;
}

const Babl *
gimp_temp_buf_get_format (const GimpTempBuf *buf)
{
  return buf->format;
}

void
gimp_temp_buf_set_format (GimpTempBuf *buf,
                          const Babl  *format)
{
  g_return_if_fail (babl_format_get_bytes_per_pixel (buf->format) ==
                    babl_format_get_bytes_per_pixel (format));

  buf->format = format;
}

guchar *
gimp_temp_buf_get_data (const GimpTempBuf *buf)
{
  return buf->data;
}

gsize
gimp_temp_buf_get_data_size (const GimpTempBuf *buf)
{
  return babl_format_get_bytes_per_pixel (buf->format) * buf->width * buf->height;
}

guchar *
gimp_temp_buf_data_clear (GimpTempBuf *buf)
{
  memset (buf->data, 0, gimp_temp_buf_get_data_size (buf));

  return buf->data;
}

gsize
gimp_temp_buf_get_memsize (const GimpTempBuf *buf)
{
  if (buf)
    return (sizeof (GimpTempBuf) + gimp_temp_buf_get_data_size (buf));

  return 0;
}

GeglBuffer  *
gimp_temp_buf_create_buffer (GimpTempBuf *temp_buf)
{
  GeglBuffer *buffer;

  g_return_val_if_fail (temp_buf != NULL, NULL);

  buffer =
    gegl_buffer_linear_new_from_data (gimp_temp_buf_get_data (temp_buf),
                                      temp_buf->format,
                                      GEGL_RECTANGLE (0, 0,
                                                      temp_buf->width,
                                                      temp_buf->height),
                                      GEGL_AUTO_ROWSTRIDE,
                                      (GDestroyNotify) gimp_temp_buf_unref,
                                      gimp_temp_buf_ref (temp_buf));

  g_object_set_data (G_OBJECT (buffer), "gimp-temp-buf", temp_buf);

  return buffer;
}

GdkPixbuf *
gimp_temp_buf_create_pixbuf (GimpTempBuf *temp_buf)
{
  GdkPixbuf    *pixbuf;
  const Babl   *format;
  const Babl   *fish = NULL;
  const guchar *data;
  gint          width;
  gint          height;
  gint          bpp;
  guchar       *pixels;
  gint          rowstride;
  gint          i;

  g_return_val_if_fail (temp_buf != NULL, NULL);

  data      = gimp_temp_buf_get_data (temp_buf);
  format    = gimp_temp_buf_get_format (temp_buf);
  width     = gimp_temp_buf_get_width (temp_buf);
  height    = gimp_temp_buf_get_height (temp_buf);
  bpp       = babl_format_get_bytes_per_pixel (format);

  pixbuf    = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                              babl_format_has_alpha (format),
                              8, width, height);

  pixels    = gdk_pixbuf_get_pixels (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  if (format != gimp_pixbuf_get_format (pixbuf))
    fish = babl_fish (format, gimp_pixbuf_get_format (pixbuf));

  for (i = 0; i < height; i++)
    {
      if (fish)
        babl_process (fish, data, pixels, width);
      else
        memcpy (pixels, data, width * bpp);

      data   += width * bpp;
      pixels += rowstride;
    }

  return pixbuf;
}

GimpTempBuf *
gimp_gegl_buffer_get_temp_buf (GeglBuffer *buffer)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  return g_object_get_data (G_OBJECT (buffer), "gimp-temp-buf");
}
