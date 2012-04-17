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

/*
 * blend_pixels patched 8-24-05 to fix bug #163721.  Note that this change
 * causes the function to treat src1 and src2 asymmetrically.  This gives the
 * right behavior for the smudge tool, which is the only user of this function
 * at the time of patching.  If you want to use the function for something
 * else, caveat emptor.
 */
void
gimp_gegl_smudge_blend (GeglBuffer          *top_buffer,
                        const GeglRectangle *top_rect,
                        GeglBuffer          *bottom_buffer,
                        const GeglRectangle *bottom_rect,
                        GeglBuffer          *dest_buffer,
                        const GeglRectangle *dest_rect,
                        guchar               blend)
{
  GeglBufferIterator *iter;
  const Babl         *top_format;
  const Babl         *bottom_format;
  const Babl         *dest_format;
  gint                top_bpp;
  gint                bottom_bpp;
  gint                dest_bpp;

  top_format    = gegl_buffer_get_format (top_buffer);
  bottom_format = gegl_buffer_get_format (bottom_buffer);
  dest_format   = gegl_buffer_get_format (dest_buffer);

  top_bpp    = babl_format_get_bytes_per_pixel (top_format);
  bottom_bpp = babl_format_get_bytes_per_pixel (bottom_format);
  dest_bpp   = babl_format_get_bytes_per_pixel (dest_format);

  iter = gegl_buffer_iterator_new (top_buffer, top_rect, 0, NULL,
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, bottom_buffer, bottom_rect, 0, NULL,
                            GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0, NULL,
                            GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *top    = iter->data[0];
      const guchar *bottom = iter->data[1];
      guchar       *dest   = iter->data[2];

      if (babl_format_has_alpha (top_format))
        {
          const guint blend1 = 255 - blend;
          const guint blend2 = blend + 1;
          const guint c      = top_bpp - 1;

          while (iter->length--)
            {
              const gint  a1 = blend1 * bottom[c];
              const gint  a2 = blend2 * top[c];
              const gint  a  = a1 + a2;
              guint       b;

              if (!a)
                {
                  for (b = 0; b < top_bpp; b++)
                    dest[b] = 0;
                }
              else
                {
                  for (b = 0; b < c; b++)
                    dest[b] =
                      bottom[b] + (bottom[b] * a1 + top[b] * a2 - a * bottom[b]) / a;

                  dest[c] = a >> 8;
                }

              top    += top_bpp;
              bottom += bottom_bpp;
              dest   += dest_bpp;
            }
        }
      else
        {
          const guchar blend1 = 255 - blend;

          while (iter->length--)
            {
              guint b;

              for (b = 0; b < top_bpp; b++)
                dest[b] =
                  bottom[b] + (bottom[b] * blend1 + top[b] * blend - bottom[b] * 255) / 255;

              top    += top_bpp;
              bottom += bottom_bpp;
              dest   += dest_bpp;
            }
        }
    }
}
