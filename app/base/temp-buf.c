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

#include <babl/babl.h>
#include <cairo.h>
#include <glib-object.h>
#include <glib/gstdio.h>

#include "base-types.h"

#include "temp-buf.h"


TempBuf *
temp_buf_new (gint        width,
              gint        height,
              const Babl *format)
{
  TempBuf *temp;

  g_return_val_if_fail (width > 0 && height > 0, NULL);
  g_return_val_if_fail (format != NULL, NULL);

  temp = g_slice_new (TempBuf);

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

TempBuf *
temp_buf_copy (TempBuf *src)
{
  TempBuf *dest;

  g_return_val_if_fail (src != NULL, NULL);

  dest = temp_buf_new (src->width, src->height, src->format);

  if (! dest)
    return NULL;

  memcpy (temp_buf_get_data (dest),
          temp_buf_get_data (src),
          temp_buf_get_data_size (src));

  return dest;
}

TempBuf *
temp_buf_scale (TempBuf *src,
                gint     new_width,
                gint     new_height)
{
  TempBuf      *dest;
  const guchar *src_data;
  guchar       *dest_data;
  gdouble       x_ratio;
  gdouble       y_ratio;
  gint          bytes;
  gint          loop1;
  gint          loop2;

  g_return_val_if_fail (src != NULL, NULL);
  g_return_val_if_fail (new_width > 0 && new_height > 0, NULL);

  dest = temp_buf_new (new_width,
                       new_height,
                       src->format);

  src_data  = temp_buf_get_data (src);
  dest_data = temp_buf_get_data (dest);

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
 * temp_buf_demultiply:
 * @buf:
 *
 * Converts a TempBuf with pre-multiplied alpha to a 'normal' TempBuf.
 */
void
temp_buf_demultiply (TempBuf *buf)
{
  guchar *data;
  gint    pixels;

  g_return_if_fail (buf != NULL);

  switch (babl_format_get_bytes_per_pixel (buf->format))
    {
    case 1:
      break;

    case 2:
      data = temp_buf_get_data (buf);
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
      data = temp_buf_get_data (buf);
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

void
temp_buf_free (TempBuf *buf)
{
  g_return_if_fail (buf != NULL);

  if (buf->data)
    g_free (buf->data);

  g_slice_free (TempBuf, buf);
}

guchar *
temp_buf_get_data (const TempBuf *buf)
{
  return buf->data;
}

gsize
temp_buf_get_data_size (TempBuf *buf)
{
  return babl_format_get_bytes_per_pixel (buf->format) * buf->width * buf->height;
}

guchar *
temp_buf_data_clear (TempBuf *buf)
{
  memset (buf->data, 0, temp_buf_get_data_size (buf));

  return buf->data;
}

gsize
temp_buf_get_memsize (TempBuf *buf)
{
  if (buf)
    return (sizeof (TempBuf) + temp_buf_get_data_size (buf));

  return 0;
}


/**
 * temp_buf_dump:
 * @buf:
 * @file:
 *
 * Dumps a TempBuf to a raw RGB image that is easy to analyze, for
 * example with GIMP.
 **/
void
temp_buf_dump (TempBuf     *buf,
               const gchar *filename)
{
  gint fd = g_open (filename, O_CREAT | O_TRUNC | O_WRONLY, 0666);

  g_return_if_fail (fd != -1);
  g_return_if_fail (buf != NULL);
  g_return_if_fail (temp_buf_get_data (buf) != NULL);

  write (fd, temp_buf_get_data (buf), temp_buf_get_data_size (buf));

  close (fd);
}
