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

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "base-types.h"

#include "paint-funcs/paint-funcs.h"

#include "pixel-region.h"
#include "temp-buf.h"


static void  temp_buf_to_color (TempBuf *src_buf,
                                TempBuf *dest_buf);
static void  temp_buf_to_gray  (TempBuf *src_buf,
                                TempBuf *dest_buf);


TempBuf *
temp_buf_new (gint          width,
              gint          height,
              gint          bytes,
              gint          x,
              gint          y,
              const guchar *color)
{
  TempBuf *temp;

  g_return_val_if_fail (width > 0 && height > 0, NULL);
  g_return_val_if_fail (bytes > 0, NULL);

  temp = g_slice_new (TempBuf);

  temp->width  = width;
  temp->height = height;
  temp->bytes  = bytes;
  temp->x      = x;
  temp->y      = y;

  temp->data = g_new (guchar, width * height * bytes);

  /*  initialize the data  */
  if (color)
    {
      glong i;

      /* First check if we can save a lot of work */
      for (i = 1; i < bytes; i++)
        {
          if (color[0] != color[i])
            break;
        }

      if (i == bytes)
        {
          memset (temp->data, *color, width * height * bytes);
        }
      else /* No, we cannot */
        {
          guchar *dptr = temp->data;

          /* Fill the first row */
          for (i = width - 1; i >= 0; --i)
            {
              const guchar *c = color;
              gint          j = bytes;

              while (j--)
                *dptr++ = *c++;
            }

          /* Now copy from it (we set bytes to bytes-per-row now) */
          bytes *= width;

          while (--height)
            {
              memcpy (dptr, temp->data, bytes);
              dptr += bytes;
            }
        }
    }

  return temp;
}

/* This function simply renders a checkerboard with the given
   parameters into a newly allocated RGB tempbuf */

TempBuf *
temp_buf_new_check (gint           width,
                    gint           height,
                    GimpCheckType  check_type,
                    GimpCheckSize  check_size)
{
  TempBuf *new;
  guchar  *data;
  guchar   check_shift = 0;
  guchar   check_mod   = 0;
  guchar   check_light = 0;
  guchar   check_dark  = 0;
  gint     x, y;

  g_return_val_if_fail (width > 0 && height > 0, NULL);

  switch (check_size)
    {
    case GIMP_CHECK_SIZE_SMALL_CHECKS:
      check_mod   = 0x3;
      check_shift = 2;
      break;
    case GIMP_CHECK_SIZE_MEDIUM_CHECKS:
      check_mod   = 0x7;
      check_shift = 3;
      break;
    case GIMP_CHECK_SIZE_LARGE_CHECKS:
      check_mod   = 0xf;
      check_shift = 4;
      break;
    }

  gimp_checks_get_shades (check_type, &check_light, &check_dark);

  new = temp_buf_new (width, height, 3, 0, 0, NULL);
  data = temp_buf_data (new);

  for (y = 0; y < height; y++)
    {
      guchar check_dark = y >> check_shift;
      guchar color      = (check_dark & 0x1) ? check_light : check_dark;

      for (x = 0; x < width; x++)
        {
          *data++ = color;
          *data++ = color;
          *data++ = color;

          if (((x + 1) & check_mod) == 0)
            {
              check_dark += 1;
              color = (check_dark & 0x1) ? check_light : check_dark;
            }
        }
    }

  return new;
}

TempBuf *
temp_buf_copy (TempBuf *src,
               TempBuf *dest)
{
  g_return_val_if_fail (src != NULL, NULL);
  g_return_val_if_fail (! dest || (dest->width  == src->width &&
                                   dest->height == src->height), NULL);

  if (! dest)
    dest = temp_buf_new (src->width, src->height, src->bytes, 0, 0, NULL);

  if (src->bytes != dest->bytes)
    {
      if (src->bytes == 4 && dest->bytes == 2)       /* RGBA  -> GRAYA */
        temp_buf_to_gray (src, dest);
      else if (src->bytes == 3 && dest->bytes == 1)  /* RGB   -> GRAY  */
        temp_buf_to_gray (src, dest);
      else if (src->bytes == 2 && dest->bytes == 4)  /* GRAYA -> RGBA  */
        temp_buf_to_color (src, dest);
      else if (src->bytes == 1 && dest->bytes == 3)  /* GRAY  -> RGB   */
        temp_buf_to_color (src, dest);
      else
        g_warning ("temp_buf_copy(): unimplemented color conversion");
    }
  else
    {
      memcpy (temp_buf_data (dest),
              temp_buf_data (src),
              src->width * src->height * src->bytes);
    }

  return dest;
}

TempBuf *
temp_buf_resize (TempBuf *buf,
                 gint     bytes,
                 gint     x,
                 gint     y,
                 gint     width,
                 gint     height)
{
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  if (! buf)
    {
      buf = temp_buf_new (width, height, bytes, x, y, NULL);
    }
  else
    {
      gsize size = width * height * bytes;

      if (size != (buf->width * buf->height * buf->bytes))
        {
          buf->data = g_renew (guchar, buf->data, size);
        }

      buf->x      = x;
      buf->y      = y;
      buf->width  = width;
      buf->height = height;
      buf->bytes  = bytes;
    }

  return buf;
}

TempBuf *
temp_buf_scale (TempBuf *src,
                gint     new_width,
                gint     new_height)
{
  TempBuf *dest;
  guchar  *src_data;
  guchar  *dest_data;
  gdouble  x_ratio;
  gdouble  y_ratio;
  gint     loop1;
  gint     loop2;

  g_return_val_if_fail (src != NULL, NULL);
  g_return_val_if_fail (new_width > 0 && new_height > 0, NULL);

  dest = temp_buf_new (new_width,
                       new_height,
                       src->bytes,
                       0, 0, NULL);

  src_data  = temp_buf_data (src);
  dest_data = temp_buf_data (dest);

  x_ratio = (gdouble) src->width  / (gdouble) new_width;
  y_ratio = (gdouble) src->height / (gdouble) new_height;

  for (loop1 = 0 ; loop1 < new_height ; loop1++)
    {
      for (loop2 = 0 ; loop2 < new_width ; loop2++)
        {
          guchar *src_pixel;
          guchar *dest_pixel;
          gint    i;

          src_pixel = src_data +
            (gint) (loop2 * x_ratio) * src->bytes +
            (gint) (loop1 * y_ratio) * src->bytes * src->width;

          dest_pixel = dest_data +
            (loop2 + loop1 * new_width) * src->bytes;

          for (i = 0 ; i < src->bytes; i++)
            *dest_pixel++ = *src_pixel++;
        }
    }

  return dest;
}

TempBuf *
temp_buf_copy_area (TempBuf *src,
                    TempBuf *dest,
                    gint     x,
                    gint     y,
                    gint     width,
                    gint     height,
                    gint     dest_x,
                    gint     dest_y)
{
  TempBuf     *new;
  PixelRegion  srcPR  = { 0, };
  PixelRegion  destPR = { 0, };
  guchar       empty[MAX_CHANNELS] = { 0, 0, 0, 0 };
  gint         x1, y1, x2, y2;

  g_return_val_if_fail (src != NULL, dest);
  g_return_val_if_fail (!dest || dest->bytes == src->bytes, dest);

  g_return_val_if_fail (width  + dest_x > 0, dest);
  g_return_val_if_fail (height + dest_y > 0, dest);

  g_return_val_if_fail (!dest || dest->width  >= width  + dest_x, dest);
  g_return_val_if_fail (!dest || dest->height >= height + dest_y, dest);

  /*  some bounds checking  */
  x1 = CLAMP (x, 0, src->width  - 1);
  y1 = CLAMP (y, 0, src->height - 1);
  x2 = CLAMP (x + width  - 1, 0, src->width  - 1);
  y2 = CLAMP (y + height - 1, 0, src->height - 1);

  if (!(x2 - x1) || !(y2 - y1))
    return dest;

  width  = x2 - x1 + 1;
  height = y2 - y1 + 1;

  if (! dest)
    {
      new = temp_buf_new (width  + dest_x,
                          height + dest_y,
                          src->bytes,
                          0, 0,
                          empty);
    }
  else
    {
      new = dest;
    }

  /*  Copy the region  */
  pixel_region_init_temp_buf (&srcPR,  src, x1, y1, width, height);
  pixel_region_init_temp_buf (&destPR, new, dest_x, dest_y, width, height);

  copy_region (&srcPR, &destPR);

  return new;
}

void
temp_buf_free (TempBuf *temp_buf)
{
  g_return_if_fail (temp_buf != NULL);

  if (temp_buf->data)
    g_free (temp_buf->data);

  g_slice_free (TempBuf, temp_buf);
}

guchar *
temp_buf_data (TempBuf *temp_buf)
{
  return temp_buf->data;
}

guchar *
temp_buf_data_clear (TempBuf *temp_buf)
{
  memset (temp_buf->data, 0,
          temp_buf->height * temp_buf->width * temp_buf->bytes);

  return temp_buf->data;
}

gsize
temp_buf_get_memsize (TempBuf *temp_buf)
{
  gsize memsize = 0;

  g_return_val_if_fail (temp_buf != NULL, 0);

  memsize += (sizeof (TempBuf)
              + (gsize) temp_buf->bytes * temp_buf->width * temp_buf->height);

  return memsize;
}


/*  The conversion routines  */

static void
temp_buf_to_color (TempBuf *src_buf,
                   TempBuf *dest_buf)
{
  guchar *src;
  guchar *dest;
  glong   num_pixels;

  src  = temp_buf_data (src_buf);
  dest = temp_buf_data (dest_buf);

  num_pixels = src_buf->width * src_buf->height;

  switch (dest_buf->bytes)
    {
    case 3:
      g_return_if_fail (src_buf->bytes == 1);
      while (num_pixels--)
        {
          guchar tmp;

          *dest++ = tmp = *src++;
          *dest++ = tmp;
          *dest++ = tmp;
        }
      break;

    case 4:
      g_return_if_fail (src_buf->bytes == 2);
      while (num_pixels--)
        {
          guchar tmp;

          *dest++ = tmp = *src++;
          *dest++ = tmp;
          *dest++ = tmp;

          *dest++ = *src++;  /* alpha channel */
        }
      break;

    default:
      g_return_if_reached ();
      break;
    }
}

static void
temp_buf_to_gray (TempBuf *src_buf,
                  TempBuf *dest_buf)
{
  const guchar *src;
  guchar       *dest;
  glong         num_pixels;

  src  = temp_buf_data (src_buf);
  dest = temp_buf_data (dest_buf);

  num_pixels = src_buf->width * src_buf->height;

  switch (dest_buf->bytes)
    {
    case 1:
      g_return_if_fail (src_buf->bytes == 3);
      while (num_pixels--)
        {
          gint lum = GIMP_RGB_LUMINANCE (src[0], src[1], src[2]) + 0.5;

          *dest++ = (guchar) lum;

          src += 3;
        }
      break;

    case 2:
      g_return_if_fail (src_buf->bytes == 4);
      while (num_pixels--)
        {
          gint lum = GIMP_RGB_LUMINANCE (src[0], src[1], src[2]) + 0.5;

          *dest++ = (guchar) lum;
          *dest++ = src[3];  /* alpha channel */

          src += 4;
        }
      break;

    default:
      g_return_if_reached ();
      break;
    }
}
