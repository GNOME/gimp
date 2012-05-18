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
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core/core-types.h" /* eek, but this file will die anyway */

#include "base/pixel-region.h"

#include "paint-funcs.h"


/*  Local function prototypes  */

static inline void rotate_pointers       (guchar        **p,
                                          guint32         n);


/**************************************************/
/*    REGION FUNCTIONS                            */
/**************************************************/

void
convolve_region (PixelRegion         *srcR,
                 PixelRegion         *destR,
                 const gfloat        *matrix,
                 gint                 size,
                 gdouble              divisor,
                 GimpConvolutionType  mode,
                 gboolean             alpha_weighting)
{
  /*  Convolve the src image using the convolution matrix, writing to dest  */
  /*  Convolve is not tile-enabled--use accordingly  */
  const guchar *src       = srcR->data;
  guchar       *dest      = destR->data;
  const gint    bytes     = srcR->bytes;
  const gint    a_byte    = bytes - 1;
  const gint    rowstride = srcR->rowstride;
  const gint    margin    = size / 2;
  const gint    x1        = srcR->x;
  const gint    y1        = srcR->y;
  const gint    x2        = srcR->x + srcR->w - 1;
  const gint    y2        = srcR->y + srcR->h - 1;
  gint          x, y;
  gint          offset;

  /*  If the mode is NEGATIVE_CONVOL, the offset should be 128  */
  if (mode == GIMP_NEGATIVE_CONVOL)
    {
      offset = 128;
      mode = GIMP_NORMAL_CONVOL;
    }
  else
    {
      offset = 0;
    }

  for (y = 0; y < destR->h; y++)
    {
      guchar *d = dest;

      if (alpha_weighting)
        {
          for (x = 0; x < destR->w; x++)
            {
              const gfloat *m                = matrix;
              gdouble       total[4]         = { 0.0, 0.0, 0.0, 0.0 };
              gdouble       weighted_divisor = 0.0;
              gint          i, j, b;

              for (j = y - margin; j <= y + margin; j++)
                {
                  for (i = x - margin; i <= x + margin; i++, m++)
                    {
                      gint          xx = CLAMP (i, x1, x2);
                      gint          yy = CLAMP (j, y1, y2);
                      const guchar *s  = src + yy * rowstride + xx * bytes;
                      const guchar  a  = s[a_byte];

                      if (a)
                        {
                          gdouble mult_alpha = *m * a;

                          weighted_divisor += mult_alpha;

                          for (b = 0; b < a_byte; b++)
                            total[b] += mult_alpha * s[b];

                          total[a_byte] += mult_alpha;
                        }
                    }
                }

              if (weighted_divisor == 0.0)
                weighted_divisor = divisor;

              for (b = 0; b < a_byte; b++)
                total[b] /= weighted_divisor;

              total[a_byte] /= divisor;

              for (b = 0; b < bytes; b++)
                {
                  total[b] += offset;

                  if (mode != GIMP_NORMAL_CONVOL && total[b] < 0.0)
                    total[b] = - total[b];

                  if (total[b] < 0.0)
                    *d++ = 0;
                  else
                    *d++ = (total[b] > 255.0) ? 255 : (guchar) ROUND (total[b]);
                }
            }
        }
      else
        {
          for (x = 0; x < destR->w; x++)
            {
              const gfloat *m        = matrix;
              gdouble       total[4] = { 0.0, 0.0, 0.0, 0.0 };
              gint          i, j, b;

              for (j = y - margin; j <= y + margin; j++)
                {
                  for (i = x - margin; i <= x + margin; i++, m++)
                    {
                      gint          xx = CLAMP (i, x1, x2);
                      gint          yy = CLAMP (j, y1, y2);
                      const guchar *s  = src + yy * rowstride + xx * bytes;

                      for (b = 0; b < bytes; b++)
                        total[b] += *m * s[b];
                    }
                }

              for (b = 0; b < bytes; b++)
                {
                  total[b] = total[b] / divisor + offset;

                  if (mode != GIMP_NORMAL_CONVOL && total[b] < 0.0)
                    total[b] = - total[b];

                  if (total[b] < 0.0)
                    *d++ = 0.0;
                  else
                    *d++ = (total[b] > 255.0) ? 255 : (guchar) ROUND (total[b]);
                }
            }
        }

      dest += destR->rowstride;
    }
}


static inline void
rotate_pointers (guchar  **p,
                 guint32   n)
{
  guint32  i;
  guchar  *tmp;

  tmp = p[0];

  for (i = 0; i < n - 1; i++)
    p[i] = p[i + 1];

  p[i] = tmp;
}


/*  Simple convolution filter to smooth a mask (1bpp).  */
void
smooth_region (PixelRegion *region)
{
  gint      x, y;
  gint      width;
  gint      i;
  guchar   *buf[3];
  guchar   *out;

  width = region->w;

  for (i = 0; i < 3; i++)
    buf[i] = g_new (guchar, width + 2);

  out = g_new (guchar, width);

  /* load top of image */
  pixel_region_get_row (region, region->x, region->y, width, buf[0] + 1, 1);

  buf[0][0]         = buf[0][1];
  buf[0][width + 1] = buf[0][width];

  memcpy (buf[1], buf[0], width + 2);

  for (y = 0; y < region->h; y++)
    {
      if (y + 1 < region->h)
        {
          pixel_region_get_row (region, region->x, region->y + y + 1, width,
                                buf[2] + 1, 1);

          buf[2][0]         = buf[2][1];
          buf[2][width + 1] = buf[2][width];
        }
      else
        {
          memcpy (buf[2], buf[1], width + 2);
        }

      for (x = 0 ; x < width; x++)
        {
          gint value = (buf[0][x] + buf[0][x+1] + buf[0][x+2] +
                        buf[1][x] + buf[2][x+1] + buf[1][x+2] +
                        buf[2][x] + buf[1][x+1] + buf[2][x+2]);

          out[x] = value / 9;
        }

      pixel_region_set_row (region, region->x, region->y + y, width, out);

      rotate_pointers (buf, 3);
    }

  for (i = 0; i < 3; i++)
    g_free (buf[i]);

  g_free (out);
}

/*  Erode (radius 1 pixel) a mask (1bpp).  */
void
erode_region (PixelRegion *region)
{
  gint      x, y;
  gint      width;
  gint      i;
  guchar   *buf[3];
  guchar   *out;

  width = region->w;

  for (i = 0; i < 3; i++)
    buf[i] = g_new (guchar, width + 2);

  out = g_new (guchar, width);

  /* load top of image */
  pixel_region_get_row (region, region->x, region->y, width, buf[0] + 1, 1);

  buf[0][0]         = buf[0][1];
  buf[0][width + 1] = buf[0][width];

  memcpy (buf[1], buf[0], width + 2);

  for (y = 0; y < region->h; y++)
    {
      if (y + 1 < region->h)
        {
          pixel_region_get_row (region, region->x, region->y + y + 1, width,
                                buf[2] + 1, 1);

          buf[2][0]         = buf[2][1];
          buf[2][width + 1] = buf[2][width];
        }
      else
        {
          memcpy (buf[2], buf[1], width + 2);
        }

      for (x = 0 ; x < width; x++)
        {
          gint min = 255;

          if (buf[0][x+1] < min) min = buf[0][x+1];
          if (buf[1][x]   < min) min = buf[1][x];
          if (buf[1][x+1] < min) min = buf[1][x+1];
          if (buf[1][x+2] < min) min = buf[1][x+2];
          if (buf[2][x+1] < min) min = buf[2][x+1];

          out[x] = min;
        }

      pixel_region_set_row (region, region->x, region->y + y, width, out);

      rotate_pointers (buf, 3);
    }

  for (i = 0; i < 3; i++)
    g_free (buf[i]);

  g_free (out);
}

/*  Dilate (radius 1 pixel) a mask (1bpp).  */
void
dilate_region (PixelRegion *region)
{
  gint      x, y;
  gint      width;
  gint      i;
  guchar   *buf[3];
  guchar   *out;

  width = region->w;

  for (i = 0; i < 3; i++)
    buf[i] = g_new (guchar, width + 2);

  out = g_new (guchar, width);

  /* load top of image */
  pixel_region_get_row (region, region->x, region->y, width, buf[0] + 1, 1);

  buf[0][0]         = buf[0][1];
  buf[0][width + 1] = buf[0][width];

  memcpy (buf[1], buf[0], width + 2);

  for (y = 0; y < region->h; y++)
    {
      if (y + 1 < region->h)
        {
          pixel_region_get_row (region, region->x, region->y + y + 1, width,
                                buf[2] + 1, 1);

          buf[2][0]         = buf[2][1];
          buf[2][width + 1] = buf[2][width];
        }
      else
        {
          memcpy (buf[2], buf[1], width + 2);
        }

      for (x = 0 ; x < width; x++)
        {
          gint max = 0;

          if (buf[0][x+1] > max) max = buf[0][x+1];
          if (buf[1][x]   > max) max = buf[1][x];
          if (buf[1][x+1] > max) max = buf[1][x+1];
          if (buf[1][x+2] > max) max = buf[1][x+2];
          if (buf[2][x+1] > max) max = buf[2][x+1];

          out[x] = max;
        }

      pixel_region_set_row (region, region->x, region->y + y, width, out);

      rotate_pointers (buf, 3);
    }

  for (i = 0; i < 3; i++)
    g_free (buf[i]);

  g_free (out);
}
