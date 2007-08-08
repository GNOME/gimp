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

#include "libgimpmath/gimpmath.h"

#include "paint-funcs-types.h"

#include "base/pixel-region.h"

#include "paint-funcs.h"
#include "reduce-region.h"


static void  reduce_pixel_region      (PixelRegion  *srcPR,
                                       guchar       *dst,
                                       gint          bytes);
static void  reduce_buffer            (const guchar *src,
                                       guchar       *dst,
                                       gint          bytes,
                                       gint          width,
                                       gint          height,
                                       gint          alpha);
static void  reduce_row               (const guchar *src,
                                       guchar       *dst,
                                       gint          width,
                                       gint          bytes,
                                       gint          alpha);
static void  reduce_bilinear_pr       (PixelRegion  *dstPR,
                                       PixelRegion  *srcPR);
static void  reduce_bilinear          (PixelRegion  *dstPR,
                                       guchar       *src,
                                       gint          source_w,
                                       gint          source_h,
                                       gint          bytes);

void
reduce_region (PixelRegion      *srcPR,
               PixelRegion      *dstPR,
               GimpProgressFunc  progress_callback,
               gpointer          progress_data)
{
  const gint bytes = dstPR->bytes;
  gint       alpha = 0;
  gint       level = 0;
  gdouble    scale = (gdouble) dstPR->h / (gdouble) srcPR->h;

  if (pixel_region_has_alpha (srcPR))
    alpha = bytes - 1;

  while (scale <= 0.5)
    {
      scale  *= 2;
      level++;
    }

  if (level > 0)
    {
      gint    width  = srcPR->w / 2;
      gint    height = srcPR->h / 2;
      guchar *dst    = g_new (guchar, width * height * bytes);

      reduce_pixel_region (srcPR, dst, bytes);
      level--;

      if (level > 0)
        {
          while (level)
            {
              guchar *src = dst;

              width  = width  / 2;
              height = height / 2;

              dst = g_new (guchar, width * height * bytes);
              reduce_buffer (src, dst, bytes, width * 2, height * 2, alpha);
              g_free(src);

              level--;
            }
        }

      reduce_bilinear (dstPR, dst, width, height, bytes);
      g_free (dst);
    }
  else
    {
      reduce_bilinear_pr (dstPR, srcPR);
    }
}

static void
reduce_pixel_region (PixelRegion *srcPR,
                     guchar      *dst,
                     gint         bytes)
{
  guchar *src_buf, *row0, *row1, *row2;
  gint    y;
  gint    src_width  = srcPR->w;
  gint    dst_width  = srcPR->w/2;
  guchar *dst_ptr;
  gint    alpha = 0;

  if (pixel_region_has_alpha (srcPR))
    alpha = bytes - 1;

  src_buf  = g_new (guchar, src_width * bytes * 3);

  row0 = src_buf;
  row1 = row0 + src_width * bytes;
  row2 = row1 + src_width * bytes;

  dst_ptr = dst;
  for (y = 0 ; y < srcPR->h ; y+=2)
    {
      pixel_region_get_row (srcPR, 0, ABS(y-1), src_width, row0, 1);
      pixel_region_get_row (srcPR, 0, y       , src_width, row1, 1);
      pixel_region_get_row (srcPR, 0, y+1     , src_width, row2, 1);

      reduce_row(src_buf, dst_ptr, src_width, bytes, alpha);

      dst_ptr += dst_width * bytes;
  }

  g_free (src_buf);
}

static void
reduce_buffer (const guchar *src,
               guchar       *dst,
               gint          bytes,
               gint          width,
               gint          height,
               gint          alpha)
{
  guchar *src_buf;
  guchar *row0, *row1, *row2;
  gint    y, pos;
  gint    dst_width = width/2;
  guchar *dstptr;
  gint    rowstride = width * bytes;

  src_buf  = g_new (guchar, rowstride * 3);

  row0 = src_buf;
  row1 = row0 + rowstride;
  row2 = row1 + rowstride;

  dstptr = dst;

  for (y = 0 ; y < height ; y+=2)
    {
      pos = ABS (y-1) * rowstride;
      memcpy (row0, src+pos, rowstride);

      pos = y * rowstride;
      memcpy (row1, src+pos, rowstride);

      pos= (y+1) * rowstride;
      memcpy (row2, src+pos, rowstride);

      reduce_row (src_buf, dstptr, width, bytes, alpha);

      dstptr += dst_width * bytes;
  }

  g_free (src_buf);
}



static void
reduce_row (const guchar *src,
            guchar       *dst,
            gint          width,
            gint          bytes,
            gint          alpha)
{

  gint          x, b, start;
  gdouble       alpha_sum;
  gdouble       sum;
  const guchar *src0;
  const guchar *src1;
  const guchar *src2;
  guchar       *dstptr;

  /*  Pre calculated gausian matrix */
  /*  Standard deviation = 0.71

      12 32 12
      32 86 32
      12 32 12

      Matrix sum is = 262
      Normalize by dividing with 273

  */

  dstptr = dst;

  src0 = src;
  src1 = src + width * bytes;
  src2 = src1 + width * bytes;

  start = bytes -1;
  sum = 0.0;
  alpha_sum = 0.0;

  for (x=0; x < width ; x+=2)
    {
      for (b = start ; b >= 0 ; b--)
        {
          sum += src0[ABS(x-1) * bytes + b] * 12;
          sum += src0[ABS(x)   * bytes + b] * 32;
          sum += src0[ABS(x+1) * bytes + b] * 12;

          sum += src1[ABS(x-1) * bytes + b] * 32;
          sum += src1[ABS(x)   * bytes + b] * 86;
          sum += src1[ABS(x+1) * bytes + b] * 32;

          sum += src2[ABS(x-1) * bytes + b] * 12;
          sum += src2[ABS(x)   * bytes + b] * 32;
          sum += src2[ABS(x+1) * bytes + b] * 12;

          sum /= 273.0;

          if ( alpha && (b == alpha))
            {
              alpha_sum = sum;
              dstptr[b] = (guchar)alpha_sum;
            }
          else
            {
              if (alpha)
                {
                  if (alpha_sum != 0.0)
                    sum = sum * ( 262.0 / alpha_sum );
                  else
                    sum = 0.0;
                }

              dstptr[b] = (guchar) sum;
            }
        }

      dstptr += bytes;
    }
}


static void
reduce_bilinear (PixelRegion *dstPR,
                 guchar *src,
                 gint    source_w,
                 gint    source_h,
                 gint    bytes)
{
  gint     x, y, b;
  gint     x0, x1, y0, y1;

  gdouble  scale;
  gdouble  xfrac, yfrac;
  gdouble  s00, s01, s10, s11;

  guchar   value;
  guchar  *dst;

  scale =  (gdouble)dstPR->h / (gdouble)source_h;
  dst = g_new (guchar, dstPR->w * dstPR->h * bytes);

  for (y = 0; y < dstPR->h; y++)
    {
      yfrac = ( y / scale );
      y0 = (gint)floor(yfrac);
      y1 = (gint)ceil(yfrac);
      yfrac =  yfrac - y0;

      for (x = 0; x < dstPR->w; x++)
        {
           xfrac = (x / scale);
           x0 = (gint)floor(xfrac);
           x1 = (gint)ceil(xfrac);
           xfrac =  xfrac - x0;

           for (b = 0 ; b < bytes ; b++) {
             s00 = src[(x0 + y0 * source_w) * bytes + b];
             s10 = src[(x1 + y0 * source_w) * bytes + b];
             s01 = src[(x0 + y1 * source_w) * bytes + b];
             s11 = src[(x1 + y1 * source_w) * bytes + b];
             value =  (gint)((1 - yfrac) * ((1 - xfrac) * s00 + xfrac * s01 )
                               +  yfrac  * ((1 - xfrac) * s10 + xfrac * s11 ));
             dst[x*bytes+b] = value;
           }
        }
        pixel_region_set_row (dstPR, 0, y, dstPR->w, dst);
    }
}

static void
reduce_bilinear_pr (PixelRegion *dstPR,
                    PixelRegion *srcPR)
{
  const gint bytes = dstPR->bytes;
  gint       x, y, b;
  gint       x0, x1, y0, y1;

  gdouble    scale;
  gdouble    xfrac, yfrac;
  gdouble    s00, s01, s10, s11;

  guchar     value;
  guchar    *dst;
  guchar    *src0;
  guchar    *src1;

  /* Copy and return if scale = 1.0 */
  if (srcPR->h == dstPR->h)
    {
      copy_region (srcPR, dstPR);
      return;
    }

  scale = (gdouble) dstPR->h / (gdouble) srcPR->h;

  /* Interpolate */
  src0 = g_new0 (guchar, srcPR->w * bytes);
  src1 = g_new0 (guchar, srcPR->w * bytes);
  dst  = g_new0 (guchar, dstPR->w * bytes);

  for (y = 0; y < dstPR->h; y++)
    {
      yfrac = y / scale;
      y0 = (gint) floor (yfrac);
      y1 = (gint) ceil (yfrac);
      yfrac =  yfrac - y0;

      pixel_region_get_row (srcPR, 0, y0, srcPR->w, src0, 1);
      pixel_region_get_row (srcPR, 0, y1, srcPR->w, src1, 1);

      for (x = 0; x < dstPR->w; x++)
        {
          xfrac = (x / scale);
          x0 = (gint)floor(xfrac);
          x1 = (gint)ceil(xfrac);
          xfrac =  xfrac - x0;

          for (b = 0 ; b < bytes ; b++)
            {
              s00 = src0[x0 * bytes + b];
              s01 = src0[x1 * bytes + b];
              s10 = src1[x0 * bytes + b];
              s11 = src1[x1 * bytes + b];
              value =  (1 - yfrac) * ( (1 - xfrac) * s00 + xfrac * s01 )
                         +  yfrac  * ( (1 - xfrac) * s10 + xfrac * s11 );

              dst[x * bytes + b] = (guchar) value;
           }
        }

      pixel_region_set_row (dstPR, 0, y, dstPR->w, dst);
    }

  g_free(src0);
  g_free(src1);
  g_free(dst);
}
