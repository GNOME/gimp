/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrush-scale.c
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

#include <glib-object.h>

#include "core-types.h"

#include "gimpbrush.h"
#include "gimpbrush-scale.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/scale-funcs.h"


/*  local function prototypes  */

static TempBuf * gimp_brush_scale_buf_up      (TempBuf *brush_buf,
                                               gint     dest_width,
                                               gint     dest_height);
static TempBuf * gimp_brush_scale_mask_down   (TempBuf *brush_mask,
                                               gint     dest_width,
                                               gint     dest_height);
static TempBuf * gimp_brush_scale_pixmap_down (TempBuf *pixmap,
                                               gint     dest_width,
                                               gint     dest_height);


/*  public functions  */

void
gimp_brush_real_scale_size (GimpBrush *brush,
                            gdouble    scale,
                            gint      *width,
                            gint      *height)
{
  *width  = (gint) (brush->mask->width  * scale + 0.5);
  *height = (gint) (brush->mask->height * scale + 0.5);
}

TempBuf *
gimp_brush_real_scale_mask (GimpBrush *brush,
                            gdouble    scale)
{
  gint dest_width;
  gint dest_height;

  gimp_brush_scale_size (brush, scale, &dest_width, &dest_height);

  if (dest_width <= 0 || dest_height <= 0)
    return NULL;

  if (scale <= 1.0)
    {
      /*  Downscaling with brush_scale_mask is much faster than with
       *  gimp_brush_scale_buf.
       */
      return gimp_brush_scale_mask_down (brush->mask,
                                         dest_width, dest_height);
    }

  return gimp_brush_scale_buf_up (brush->mask, dest_width, dest_height);
}

TempBuf *
gimp_brush_real_scale_pixmap (GimpBrush *brush,
                              gdouble    scale)
{
  gint dest_width;
  gint dest_height;

  gimp_brush_scale_size (brush, scale, &dest_width, &dest_height);

  if (dest_width <= 0 || dest_height <= 0)
    return NULL;

  if (scale <= 1.0)
    {
      /*  Downscaling with brush_scale_pixmap is much faster than with
       *  gimp_brush_scale_buf.
       */
      return gimp_brush_scale_pixmap_down (brush->pixmap,
                                           dest_width, dest_height);
    }

  return gimp_brush_scale_buf_up (brush->pixmap, dest_width, dest_height);
}


/*  private functions  */

static TempBuf *
gimp_brush_scale_buf_up (TempBuf *brush_buf,
                         gint     dest_width,
                         gint     dest_height)
{
  PixelRegion  source_region;
  PixelRegion  dest_region;
  TempBuf     *dest_brush_buf;

  pixel_region_init_temp_buf (&source_region, brush_buf,
                              0, 0,
                              brush_buf->width,
                              brush_buf->height);

  dest_brush_buf = temp_buf_new (dest_width, dest_height, brush_buf->bytes,
                                 0, 0, NULL);

  pixel_region_init_temp_buf (&dest_region, dest_brush_buf,
                              0, 0,
                              dest_width,
                              dest_height);

  scale_region (&source_region, &dest_region, GIMP_INTERPOLATION_LINEAR,
                NULL, NULL);

  return dest_brush_buf;
}

static TempBuf *
gimp_brush_scale_mask_down (TempBuf *brush_mask,
                            gint     dest_width,
                            gint     dest_height)
{
  TempBuf *scale_brush;
  gint     src_width;
  gint     src_height;
  gint     value;
  gint     area;
  gint     i, j;
  gint     x, x0, y, y0;
  gint     dx, dx0, dy, dy0;
  gint     fx, fx0, fy, fy0;
  guchar  *src, *dest;

  g_return_val_if_fail (brush_mask != NULL &&
                        dest_width != 0 && dest_height != 0, NULL);

  src_width  = brush_mask->width;
  src_height = brush_mask->height;

  scale_brush = temp_buf_new (dest_width, dest_height, 1, 0, 0, NULL);
  g_return_val_if_fail (scale_brush != NULL, NULL);

  /*  get the data  */
  dest = temp_buf_data (scale_brush);
  src  = temp_buf_data (brush_mask);

  fx = fx0 = (src_width << 8) / dest_width;
  fy = fy0 = (src_height << 8) / dest_height;

  area = (fx0 * fy0) >> 8;

  x = x0 = 0;
  y = y0 = 0;
  dx = dx0 = 0;
  dy = dy0 = 0;

  for (i=0; i<dest_height; i++)
    {
      for (j=0; j<dest_width; j++)
        {
          value  = 0;

          fy = fy0;
          y  = y0;
          dy = dy0;

          if (dy)
            {
              fx = fx0;
              x  = x0;
              dx = dx0;

              if (dx)
                {
                  value += (dx * dy * src[x + src_width * y]) >> 8;
                  x++;
                  fx -= dx;
                  dx = 0;
                }
              while (fx >= 256)
                {
                  value += dy * src[x + src_width * y];
                  x++;
                  fx -= 256;
                }
              if (fx)
                {
                  value += fx * dy * src[x + src_width * y] >> 8;
                  dx = 256 - fx;
                }

              y++;
              fy -= dy;
              dy = 0;
            }

          while (fy >= 256)
            {
              fx = fx0;
              x  = x0;
              dx = dx0;

              if (dx)
                {
                  value += dx * src[x + src_width * y];
                  x++;
                  fx -= dx;
                  dx = 0;
                }
              while (fx >= 256)
                {
                  value += 256 * src[x + src_width * y];
                  x++;
                  fx -= 256;
                }
              if (fx)
                {
                  value += fx * src[x + src_width * y];
                  dx = 256 - fx;
                }

              y++;
              fy -= 256;
            }

          if (fy)
            {
              fx = fx0;
              x  = x0;
              dx = dx0;

              if (dx)
                {
                  value += (dx * fy * src[x + src_width * y]) >> 8;
                  x++;
                  fx -= dx;
                  dx = 0;
                }
              while (fx >= 256)
                {
                  value += fy * src[x + src_width * y];
                  x++;
                  fx -= 256;
                }
              if (fx)
                {
                  value += (fx * fy * src[x + src_width * y]) >> 8;
                  dx = 256 - fx;
                }

              dy = 256 - fy;
            }

          value /= area;
          *dest++ = MIN (value, 255);

          x0  = x;
          dx0 = dx;
        }

      x0  = 0;
      dx0 = 0;
      y0  = y;
      dy0 = dy;
    }

  return scale_brush;
}


#define ADD_RGB(dest, factor, src) \
  dest[0] += factor * src[0]; \
  dest[1] += factor * src[1]; \
  dest[2] += factor * src[2];

static TempBuf *
gimp_brush_scale_pixmap_down (TempBuf *pixmap,
                              gint     dest_width,
                              gint     dest_height)
{
  TempBuf *scale_brush;
  gint     src_width;
  gint     src_height;
  gint     value[3];
  gint     factor;
  gint     area;
  gint     i, j;
  gint     x, x0, y, y0;
  gint     dx, dx0, dy, dy0;
  gint     fx, fx0, fy, fy0;
  guchar  *src, *src_ptr, *dest;

  g_return_val_if_fail (pixmap != NULL && pixmap->bytes == 3 &&
                        dest_width != 0 && dest_height != 0, NULL);

  src_width  = pixmap->width;
  src_height = pixmap->height;

  scale_brush = temp_buf_new (dest_width, dest_height, 3, 0, 0, NULL);
  g_return_val_if_fail (scale_brush != NULL, NULL);

  /*  get the data  */
  dest = temp_buf_data (scale_brush);
  src  = temp_buf_data (pixmap);

  fx = fx0 = (src_width << 8) / dest_width;
  fy = fy0 = (src_height << 8) / dest_height;
  area = (fx0 * fy0) >> 8;

  x = x0 = 0;
  y = y0 = 0;
  dx = dx0 = 0;
  dy = dy0 = 0;

  for (i=0; i<dest_height; i++)
    {
      for (j=0; j<dest_width; j++)
        {
          value[0] = 0;
          value[1] = 0;
          value[2] = 0;

          fy = fy0;
          y  = y0;
          dy = dy0;

          if (dy)
            {
              fx = fx0;
              x  = x0;
              dx = dx0;

              if (dx)
                {
                  factor = (dx * dy) >> 8;
                  src_ptr = src + 3 * (x + y * src_width);
                  ADD_RGB (value, factor, src_ptr);
                  x++;
                  fx -= dx;
                  dx = 0;
                }
              while (fx >= 256)
                {
                  factor = dy;
                  src_ptr = src + 3 * (x + y * src_width);
                  ADD_RGB (value, factor, src_ptr);
                  x++;
                  fx -= 256;
                }
              if (fx)
                {
                  factor = (fx * dy) >> 8;
                  src_ptr = src + 3 * (x + y * src_width);
                  ADD_RGB (value, factor, src_ptr);
                  dx = 256 - fx;
                }

              y++;
              fy -= dy;
              dy = 0;
            }

          while (fy >= 256)
            {
              fx = fx0;
              x  = x0;
              dx = dx0;

              if (dx)
                {
                  factor = dx;
                  src_ptr = src + 3 * (x + y * src_width);
                  ADD_RGB (value, factor, src_ptr);
                  x++;
                  fx -= dx;
                  dx = 0;
                }
              while (fx >= 256)
                {
                  factor = 256;
                  src_ptr = src + 3 * (x + y * src_width);
                  ADD_RGB (value, factor, src_ptr);
                  x++;
                  fx -= 256;
                }
              if (fx)
                {
                  factor = fx;
                  src_ptr = src + 3 * (x + y * src_width);
                  ADD_RGB (value, factor, src_ptr);
                  dx = 256 - fx;
                }

              y++;
              fy -= 256;
            }

          if (fy)
            {
              fx = fx0;
              x  = x0;
              dx = dx0;

              if (dx)
                {
                  factor = (dx * fy) >> 8;
                  src_ptr = src + 3 * (x + y * src_width);
                  ADD_RGB (value, factor, src_ptr);
                  x++;
                  fx -= dx;
                  dx = 0;
                }
              while (fx >= 256)
                {
                  factor = fy;
                  src_ptr = src + 3 * (x + y * src_width);
                  ADD_RGB (value, factor, src_ptr);
                  x++;
                  fx -= 256;
                }
              if (fx)
                {
                  factor = (fx * fy) >> 8;
                  src_ptr = src + 3 * (x + y * src_width);
                  ADD_RGB (value, factor, src_ptr);
                  dx = 256 - fx;
                }

              dy = 256 - fy;
            }

          value[0] /= area;
          value[1] /= area;
          value[2] /= area;

          *dest++ = MIN (value[0], 255);
          *dest++ = MIN (value[1], 255);
          *dest++ = MIN (value[2], 255);

          x0  = x;
          dx0 = dx;
        }

      x0  = 0;
      dx0 = 0;
      y0  = y;
      dy0 = dy;
    }

  return scale_brush;
}

#undef ADD_RGB
