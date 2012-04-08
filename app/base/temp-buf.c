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

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cairo.h>
#include <gegl.h>
#include <glib/gstdio.h>

#include "base-types.h"

#include "temp-buf.h"


GimpTempBuf *
gimp_temp_buf_new (gint        width,
                   gint        height,
                   const Babl *format)
{
  GimpTempBuf *temp;

  g_return_val_if_fail (width > 0 && height > 0, NULL);
  g_return_val_if_fail (format != NULL, NULL);

  temp = g_slice_new (GimpTempBuf);

  temp->format  = format;
  temp->width   = width;
  temp->height  = height;
  temp->x       = 0;
  temp->y       = 0;

  temp->data = g_new (guchar,
                      width * height *
                      babl_format_get_bytes_per_pixel (format));

  return temp;
}

GimpTempBuf *
gimp_temp_buf_copy (GimpTempBuf *src)
{
  GimpTempBuf *dest;

  g_return_val_if_fail (src != NULL, NULL);

  dest = gimp_temp_buf_new (src->width, src->height, src->format);

  memcpy (gimp_temp_buf_get_data (dest),
          gimp_temp_buf_get_data (src),
          gimp_temp_buf_get_data_size (src));

  return dest;
}

void
gimp_temp_buf_free (GimpTempBuf *buf)
{
  g_return_if_fail (buf != NULL);

  if (buf->data)
    g_free (buf->data);

  g_slice_free (GimpTempBuf, buf);
}

GimpTempBuf *
gimp_temp_buf_scale (GimpTempBuf *src,
                     gint         new_width,
                     gint         new_height)
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

/**
 * gimp_temp_buf_demultiply:
 * @buf:
 *
 * Converts a GimpTempBuf with pre-multiplied alpha to a 'normal' GimpTempBuf.
 */
void
gimp_temp_buf_demultiply (GimpTempBuf *buf)
{
  guchar *data;
  gint    pixels;

  g_return_if_fail (buf != NULL);

  switch (babl_format_get_bytes_per_pixel (buf->format))
    {
    case 1:
      break;

    case 2:
      data = gimp_temp_buf_get_data (buf);
      pixels = buf->width * buf->height;
      while (pixels--)
        {
          data[0] = (data[0] << 8) / (data[1] + 1);

          data += 2;
        }
      break;

    case 3:
      break;

    case 4:
      data = gimp_temp_buf_get_data (buf);
      pixels = buf->width * buf->height;
      while (pixels--)
        {
          data[0] = (data[0] << 8) / (data[3] + 1);
          data[1] = (data[1] << 8) / (data[3] + 1);
          data[2] = (data[2] << 8) / (data[3] + 1);

          data += 4;
        }
      break;

    default:
      g_return_if_reached ();
      break;
    }
}

guchar *
gimp_temp_buf_get_data (const GimpTempBuf *buf)
{
  return buf->data;
}

gsize
gimp_temp_buf_get_data_size (GimpTempBuf *buf)
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
gimp_temp_buf_get_memsize (GimpTempBuf *buf)
{
  if (buf)
    return (sizeof (GimpTempBuf) + gimp_temp_buf_get_data_size (buf));

  return 0;
}

GeglBuffer  *
gimp_temp_buf_create_buffer (GimpTempBuf *temp_buf,
                             gboolean     take_ownership)
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
                                      take_ownership ?
                                      (GDestroyNotify) gimp_temp_buf_free : NULL,
                                      take_ownership ?
                                      temp_buf : NULL);

  g_object_set_data (G_OBJECT (buffer), "gimp-temp-buf", temp_buf);

  return buffer;
}
