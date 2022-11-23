/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gegl-utils.h>
#include <glib/gstdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "libligmacolor/ligmacolor.h"

#include "ligmatempbuf.h"


#define LOCK_DATA_ALIGNMENT 16


struct _LigmaTempBuf
{
  gint        ref_count;
  gint        width;
  gint        height;
  const Babl *format;
  guchar     *data;
};

typedef struct
{
  const Babl     *format;
  GeglAccessMode  access_mode;
} LockData;

G_STATIC_ASSERT (sizeof (LockData) <= LOCK_DATA_ALIGNMENT);


/*  local variables  */

static guintptr ligma_temp_buf_total_memsize = 0;


/*  public functions  */

LigmaTempBuf *
ligma_temp_buf_new (gint        width,
                   gint        height,
                   const Babl *format)
{
  LigmaTempBuf *temp;
  gint         bpp;

  g_return_val_if_fail (format != NULL, NULL);

  bpp = babl_format_get_bytes_per_pixel (format);

  g_return_val_if_fail (width > 0 && height > 0 && bpp > 0, NULL);
  g_return_val_if_fail (G_MAXSIZE / width / height / bpp > 0, NULL);

  temp = g_slice_new (LigmaTempBuf);

  temp->ref_count = 1;
  temp->width     = width;
  temp->height    = height;
  temp->format    = format;
  temp->data      = gegl_malloc ((gsize) width * height * bpp);

  g_atomic_pointer_add (&ligma_temp_buf_total_memsize,
                        +ligma_temp_buf_get_memsize (temp));

  return temp;
}

LigmaTempBuf *
ligma_temp_buf_new_from_pixbuf (GdkPixbuf  *pixbuf,
                               const Babl *f_or_null)
{
  const Babl   *format = f_or_null;
  const Babl   *fish   = NULL;
  LigmaTempBuf  *temp_buf;
  const guchar *pixels;
  gint          width;
  gint          height;
  gint          rowstride;
  gint          bpp;
  guchar       *data;
  gint          i;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  if (! format)
    format = ligma_pixbuf_get_format (pixbuf);

  pixels    = gdk_pixbuf_get_pixels (pixbuf);
  width     = gdk_pixbuf_get_width (pixbuf);
  height    = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  temp_buf  = ligma_temp_buf_new (width, height, format);
  data      = ligma_temp_buf_get_data (temp_buf);

  bpp       = babl_format_get_bytes_per_pixel (format);

  if (ligma_pixbuf_get_format (pixbuf) != format)
    fish = babl_fish (ligma_pixbuf_get_format (pixbuf), format);

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

LigmaTempBuf *
ligma_temp_buf_copy (const LigmaTempBuf *src)
{
  LigmaTempBuf *dest;

  g_return_val_if_fail (src != NULL, NULL);

  dest = ligma_temp_buf_new (src->width, src->height, src->format);

  memcpy (ligma_temp_buf_get_data (dest),
          ligma_temp_buf_get_data (src),
          ligma_temp_buf_get_data_size (src));

  return dest;
}

LigmaTempBuf *
ligma_temp_buf_ref (const LigmaTempBuf *buf)
{
  g_return_val_if_fail (buf != NULL, NULL);

  g_atomic_int_inc ((gint *) &buf->ref_count);

  return (LigmaTempBuf *) buf;
}

void
ligma_temp_buf_unref (const LigmaTempBuf *buf)
{
  g_return_if_fail (buf != NULL);
  g_return_if_fail (buf->ref_count > 0);

  if (g_atomic_int_dec_and_test ((gint *) &buf->ref_count))
    {
      g_atomic_pointer_add (&ligma_temp_buf_total_memsize,
                            -ligma_temp_buf_get_memsize (buf));


      if (buf->data)
        gegl_free (buf->data);

      g_slice_free (LigmaTempBuf, (LigmaTempBuf *) buf);
    }
}

LigmaTempBuf *
ligma_temp_buf_scale (const LigmaTempBuf *src,
                     gint               new_width,
                     gint               new_height)
{
  LigmaTempBuf  *dest;
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
    return ligma_temp_buf_copy (src);

  dest = ligma_temp_buf_new (new_width,
                            new_height,
                            src->format);

  src_data  = ligma_temp_buf_get_data (src);
  dest_data = ligma_temp_buf_get_data (dest);

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
ligma_temp_buf_get_width (const LigmaTempBuf *buf)
{
  return buf->width;
}

gint
ligma_temp_buf_get_height (const LigmaTempBuf *buf)
{
  return buf->height;
}

const Babl *
ligma_temp_buf_get_format (const LigmaTempBuf *buf)
{
  return buf->format;
}

void
ligma_temp_buf_set_format (LigmaTempBuf *buf,
                          const Babl  *format)
{
  g_return_if_fail (babl_format_get_bytes_per_pixel (buf->format) ==
                    babl_format_get_bytes_per_pixel (format));

  buf->format = format;
}

guchar *
ligma_temp_buf_get_data (const LigmaTempBuf *buf)
{
  return buf->data;
}

gsize
ligma_temp_buf_get_data_size (const LigmaTempBuf *buf)
{
  return (gsize) babl_format_get_bytes_per_pixel (buf->format) *
                 buf->width * buf->height;
}

guchar *
ligma_temp_buf_data_clear (LigmaTempBuf *buf)
{
  memset (buf->data, 0, ligma_temp_buf_get_data_size (buf));

  return buf->data;
}

gpointer
ligma_temp_buf_lock (const LigmaTempBuf *buf,
                    const Babl        *format,
                    GeglAccessMode     access_mode)
{
  guchar   *data;
  LockData *lock_data;
  gint      n_pixels;
  gint      bpp;

  g_return_val_if_fail (buf != NULL, NULL);

  if (! format || format == buf->format)
    return ligma_temp_buf_get_data (buf);

  n_pixels = buf->width * buf->height;
  bpp      = babl_format_get_bytes_per_pixel (format);

  data = gegl_scratch_alloc (LOCK_DATA_ALIGNMENT + n_pixels * bpp);

  if ((guintptr) data % LOCK_DATA_ALIGNMENT)
    {
      g_free (data);

      g_return_val_if_reached (NULL);
    }

  lock_data              = (LockData *) data;
  lock_data->format      = format;
  lock_data->access_mode = access_mode;

  data += LOCK_DATA_ALIGNMENT;

  if (access_mode & GEGL_ACCESS_READ)
    {
      babl_process (babl_fish (buf->format, format),
                    ligma_temp_buf_get_data (buf),
                    data,
                    n_pixels);
    }

  return data;
}

void
ligma_temp_buf_unlock (const LigmaTempBuf *buf,
                      gconstpointer      data)
{
  LockData *lock_data;

  g_return_if_fail (buf != NULL);
  g_return_if_fail (data != NULL);

  if (data == buf->data)
    return;

  lock_data = (LockData *) ((const guint8 *) data - LOCK_DATA_ALIGNMENT);

  if (lock_data->access_mode & GEGL_ACCESS_WRITE)
    {
      babl_process (babl_fish (lock_data->format, buf->format),
                    data,
                    ligma_temp_buf_get_data (buf),
                    buf->width * buf->height);
    }

  gegl_scratch_free (lock_data);
}

gsize
ligma_temp_buf_get_memsize (const LigmaTempBuf *buf)
{
  if (buf)
    return (sizeof (LigmaTempBuf) + ligma_temp_buf_get_data_size (buf));

  return 0;
}

GeglBuffer *
ligma_temp_buf_create_buffer (const LigmaTempBuf *temp_buf)
{
  GeglBuffer *buffer;

  g_return_val_if_fail (temp_buf != NULL, NULL);

  buffer =
    gegl_buffer_linear_new_from_data (ligma_temp_buf_get_data (temp_buf),
                                      temp_buf->format,
                                      GEGL_RECTANGLE (0, 0,
                                                      temp_buf->width,
                                                      temp_buf->height),
                                      GEGL_AUTO_ROWSTRIDE,
                                      (GDestroyNotify) ligma_temp_buf_unref,
                                      ligma_temp_buf_ref (temp_buf));

  g_object_set_data (G_OBJECT (buffer),
                     "ligma-temp-buf", (LigmaTempBuf *) temp_buf);

  return buffer;
}

GdkPixbuf *
ligma_temp_buf_create_pixbuf (const LigmaTempBuf *temp_buf)
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

  data      = ligma_temp_buf_get_data (temp_buf);
  format    = ligma_temp_buf_get_format (temp_buf);
  width     = ligma_temp_buf_get_width (temp_buf);
  height    = ligma_temp_buf_get_height (temp_buf);
  bpp       = babl_format_get_bytes_per_pixel (format);

  pixbuf    = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                              babl_format_has_alpha (format),
                              8, width, height);

  pixels    = gdk_pixbuf_get_pixels (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  if (format != ligma_pixbuf_get_format (pixbuf))
    fish = babl_fish (format, ligma_pixbuf_get_format (pixbuf));

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

LigmaTempBuf *
ligma_gegl_buffer_get_temp_buf (GeglBuffer *buffer)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  return g_object_get_data (G_OBJECT (buffer), "ligma-temp-buf");
}


/*  public functions (stats)  */

guint64
ligma_temp_buf_get_total_memsize (void)
{
  return ligma_temp_buf_total_memsize;
}
