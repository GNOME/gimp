/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-loops.c
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "gimp-gegl-loops.h"


void
gimp_gegl_convolve (GeglBuffer          *src_buffer,
                    const GeglRectangle *src_rect,
                    GeglBuffer          *dest_buffer,
                    const GeglRectangle *dest_rect,
                    const gfloat        *kernel,
                    gint                 kernel_size,
                    gdouble              divisor,
                    GimpConvolutionType  mode,
                    gboolean             alpha_weighting)
{
  GeglBufferIterator *iter;
  GeglRectangle      *src_roi;
  GeglRectangle      *dest_roi;
  const Babl         *src_format;
  const Babl         *dest_format;
  gint                src_bpp;
  gint                dest_bpp;

  src_format  = gegl_buffer_get_format (src_buffer);
  dest_format = gegl_buffer_get_format (dest_buffer);

  src_bpp  = babl_format_get_bytes_per_pixel (src_format);
  dest_bpp = babl_format_get_bytes_per_pixel (dest_format);

  iter = gegl_buffer_iterator_new (src_buffer, src_rect, 0, NULL,
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);
  src_roi = &iter->roi[0];

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0, NULL,
                            GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
  dest_roi = &iter->roi[1];

  while (gegl_buffer_iterator_next (iter))
    {
      /*  Convolve the src image using the convolution kernel, writing
       *  to dest Convolve is not tile-enabled--use accordingly
       */
      const guchar *src       = iter->data[0];
      guchar       *dest      = iter->data[1];
      const gint    bytes     = src_bpp;
      const gint    a_byte    = bytes - 1;
      const gint    rowstride = src_bpp * src_roi->width;
      const gint    margin    = kernel_size / 2;
      const gint    x1        = src_roi->x;
      const gint    y1        = src_roi->y;
      const gint    x2        = src_roi->x + src_roi->width  - 1;
      const gint    y2        = src_roi->y + src_roi->height - 1;
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

      for (y = 0; y < dest_roi->height; y++)
        {
          guchar *d = dest;

          if (alpha_weighting)
            {
              for (x = 0; x < dest_roi->width; x++)
                {
                  const gfloat *m                = kernel;
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
              for (x = 0; x < dest_roi->width; x++)
                {
                  const gfloat *m        = kernel;
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

          dest += dest_roi->width * dest_bpp;
        }
    }
}

void
gimp_gegl_dodgeburn (GeglBuffer          *src_buffer,
                     const GeglRectangle *src_rect,
                     GeglBuffer          *dest_buffer,
                     const GeglRectangle *dest_rect,
                     gdouble              exposure,
                     GimpDodgeBurnType    type,
                     GimpTransferMode     mode)
{
  GeglBufferIterator *iter;

  if (type == GIMP_BURN)
    exposure = -exposure;

  iter = gegl_buffer_iterator_new (src_buffer, src_rect, 0,
                                   babl_format ("R'G'B'A float"),
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            babl_format ("R'G'B'A float"),
                            GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  switch (mode)
    {
      gfloat factor;

    case GIMP_HIGHLIGHTS:
      factor = 1.0 + exposure * (0.333333);

      while (gegl_buffer_iterator_next (iter))
        {
          gfloat *src  = iter->data[0];
          gfloat *dest = iter->data[1];

          while (iter->length--)
            {
              *dest++ = *src++ * factor;
              *dest++ = *src++ * factor;
              *dest++ = *src++ * factor;

              *dest++ = *src++;
            }
        }
      break;

    case GIMP_MIDTONES:
      if (exposure < 0)
        factor = 1.0 - exposure * (0.333333);
      else
        factor = 1.0 / (1.0 + exposure);

      while (gegl_buffer_iterator_next (iter))
        {
          gfloat *src  = iter->data[0];
          gfloat *dest = iter->data[1];

          while (iter->length--)
            {
              *dest++ = pow (*src++, factor);
              *dest++ = pow (*src++, factor);
              *dest++ = pow (*src++, factor);

              *dest++ = *src++;
            }
        }
      break;

    case GIMP_SHADOWS:
      if (exposure >= 0)
        factor = 0.333333 * exposure;
      else
        factor = -0.333333 * exposure;

      while (gegl_buffer_iterator_next (iter))
        {
          gfloat *src  = iter->data[0];
          gfloat *dest = iter->data[1];

          while (iter->length--)
            {
              if (exposure >= 0)
                {
                  gfloat s;

                  s = *src++; *dest++ = factor + s - factor * s;
                  s = *src++; *dest++ = factor + s - factor * s;
                  s = *src++; *dest++ = factor + s - factor * s;
                }
              else
                {
                  gfloat s;

                  s = *src++;
                  if (s < factor)
                    *dest++ = 0;
                  else /* factor <= value <=1 */
                    *dest++ = (s - factor) / (1.0 - factor);

                  s = *src++;
                  if (s < factor)
                    *dest++ = 0;
                  else /* factor <= value <=1 */
                    *dest++ = (s - factor) / (1.0 - factor);

                  s = *src++;
                  if (s < factor)
                    *dest++ = 0;
                  else /* factor <= value <=1 */
                    *dest++ = (s - factor) / (1.0 - factor);
                }

              *dest++ = *src++;
           }
        }
      break;
    }
}
