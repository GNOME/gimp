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

#include "gimp-babl.h"
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
  gint                src_components;
  gint                dest_components;

  src_format = gegl_buffer_get_format (src_buffer);

  if (babl_format_is_palette (src_format))
    src_format = gimp_babl_format (GIMP_RGB,
                                   GIMP_PRECISION_FLOAT_LINEAR,
                                   babl_format_has_alpha (src_format));
  else
    src_format = gimp_babl_format (gimp_babl_format_get_base_type (src_format),
                                   GIMP_PRECISION_FLOAT_LINEAR,
                                   babl_format_has_alpha (src_format));

  dest_format = gegl_buffer_get_format (dest_buffer);

  if (babl_format_is_palette (dest_format))
    dest_format = gimp_babl_format (GIMP_RGB,
                                    GIMP_PRECISION_FLOAT_LINEAR,
                                    babl_format_has_alpha (dest_format));
  else
    dest_format = gimp_babl_format (gimp_babl_format_get_base_type (dest_format),
                                    GIMP_PRECISION_FLOAT_LINEAR,
                                    babl_format_has_alpha (dest_format));

  src_components  = babl_format_get_n_components (src_format);
  dest_components = babl_format_get_n_components (dest_format);

  iter = gegl_buffer_iterator_new (src_buffer, src_rect, 0, src_format,
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);
  src_roi = &iter->roi[0];

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0, dest_format,
                            GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
  dest_roi = &iter->roi[1];

  while (gegl_buffer_iterator_next (iter))
    {
      /*  Convolve the src image using the convolution kernel, writing
       *  to dest Convolve is not tile-enabled--use accordingly
       */
      const gfloat *src         = iter->data[0];
      gfloat       *dest        = iter->data[1];
      const gint    components  = src_components;
      const gint    a_component = components - 1;
      const gint    rowstride   = src_components * src_roi->width;
      const gint    margin      = kernel_size / 2;
      const gint    x1          = src_roi->x;
      const gint    y1          = src_roi->y;
      const gint    x2          = src_roi->x + src_roi->width  - 1;
      const gint    y2          = src_roi->y + src_roi->height - 1;
      gint          x, y;
      gfloat        offset;

      /*  If the mode is NEGATIVE_CONVOL, the offset should be 128  */
      if (mode == GIMP_NEGATIVE_CONVOL)
        {
          offset = 0.5;
          mode = GIMP_NORMAL_CONVOL;
        }
      else
        {
          offset = 0.0;
        }

      for (y = 0; y < dest_roi->height; y++)
        {
          gfloat *d = dest;

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
                          const gfloat *s  = src + yy * rowstride + xx * components;
                          const gfloat  a  = s[a_component];

                          if (a)
                            {
                              gdouble mult_alpha = *m * a;

                              weighted_divisor += mult_alpha;

                              for (b = 0; b < a_component; b++)
                                total[b] += mult_alpha * s[b];

                              total[a_component] += mult_alpha;
                            }
                        }
                    }

                  if (weighted_divisor == 0.0)
                    weighted_divisor = divisor;

                  for (b = 0; b < a_component; b++)
                    total[b] /= weighted_divisor;

                  total[a_component] /= divisor;

                  for (b = 0; b < components; b++)
                    {
                      total[b] += offset;

                      if (mode != GIMP_NORMAL_CONVOL && total[b] < 0.0)
                        total[b] = - total[b];

                      *d++ = CLAMP (total[b], 0.0, 1.0);
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
                          const gfloat *s  = src + yy * rowstride + xx * components;

                          for (b = 0; b < components; b++)
                            total[b] += *m * s[b];
                        }
                    }

                  for (b = 0; b < components; b++)
                    {
                      total[b] = total[b] / divisor + offset;

                      if (mode != GIMP_NORMAL_CONVOL && total[b] < 0.0)
                        total[b] = - total[b];

                      total[b] = CLAMP (total[b], 0.0, 1.0);
                    }
                }
            }

          dest += dest_roi->width * dest_components;
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
          gfloat *src   = iter->data[0];
          gfloat *dest  = iter->data[1];
          gint    count = iter->length;

          while (count--)
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
          gfloat *src   = iter->data[0];
          gfloat *dest  = iter->data[1];
          gint    count = iter->length;

          while (count--)
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
          gfloat *src   = iter->data[0];
          gfloat *dest  = iter->data[1];
          gint    count = iter->length;

          while (count--)
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
                        gdouble              blend)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (top_buffer, top_rect, 0,
                                   babl_format ("RGBA float"),
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, bottom_buffer, bottom_rect, 0,
                            babl_format ("RGBA float"),
                            GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            babl_format ("RGBA float"),
                            GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *top    = iter->data[0];
      const gfloat *bottom = iter->data[1];
      gfloat       *dest   = iter->data[2];
      gint          count  = iter->length;
      const gfloat  blend1 = 1.0 - blend;
      const gfloat  blend2 = blend;

      while (count--)
        {
          const gfloat a1 = blend1 * bottom[3];
          const gfloat a2 = blend2 * top[3];
          const gfloat a  = a1 + a2;
          gint         b;

          if (a == 0)
            {
              for (b = 0; b < 4; b++)
                dest[b] = 0;
            }
          else
            {
              for (b = 0; b < 3; b++)
                dest[b] =
                  bottom[b] + (bottom[b] * a1 + top[b] * a2 - a * bottom[b]);

              dest[3] = a;
            }

          top    += 4;
          bottom += 4;
          dest   += 4;
        }
    }
}

void
gimp_gegl_apply_mask (GeglBuffer          *mask_buffer,
                      const GeglRectangle *mask_rect,
                      GeglBuffer          *dest_buffer,
                      const GeglRectangle *dest_rect,
                      gdouble              opacity)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (mask_buffer, mask_rect, 0,
                                   babl_format ("Y float"),
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            babl_format ("RGBA float"),
                            GEGL_BUFFER_READWRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *mask  = iter->data[0];
      gfloat       *dest  = iter->data[1];
      gint          count = iter->length;

      while (count--)
        {
          dest[3] *= *mask * opacity;

          mask += 1;
          dest += 4;
        }
    }
}

void
gimp_gegl_combine_mask (GeglBuffer          *mask_buffer,
                        const GeglRectangle *mask_rect,
                        GeglBuffer          *dest_buffer,
                        const GeglRectangle *dest_rect,
                        gdouble              opacity)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (mask_buffer, mask_rect, 0,
                                   babl_format ("Y float"),
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            babl_format ("Y float"),
                            GEGL_BUFFER_READWRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *mask  = iter->data[0];
      gfloat       *dest  = iter->data[1];
      gint          count = iter->length;

      while (count--)
        {
          *dest *= *mask * opacity;

          mask += 1;
          dest += 1;
        }
    }
}

void
gimp_gegl_combine_mask_weird (GeglBuffer          *mask_buffer,
                              const GeglRectangle *mask_rect,
                              GeglBuffer          *dest_buffer,
                              const GeglRectangle *dest_rect,
                              gdouble              opacity,
                              gboolean             stipple)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (mask_buffer, mask_rect, 0,
                                   babl_format ("Y float"),
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            babl_format ("Y float"),
                            GEGL_BUFFER_READWRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *mask  = iter->data[0];
      gfloat       *dest  = iter->data[1];
      gint          count = iter->length;

      if (stipple)
        {
          while (count--)
            {
              dest[0] += (1.0 - dest[0]) * *mask * opacity;

              mask += 1;
              dest += 1;
            }
        }
      else
        {
          while (count--)
            {
              if (opacity > dest[0])
                dest[0] += (opacity - dest[0]) * *mask * opacity;

              mask += 1;
              dest += 1;
            }
        }
    }
}

void
gimp_gegl_replace (GeglBuffer          *top_buffer,
                   const GeglRectangle *top_rect,
                   GeglBuffer          *bottom_buffer,
                   const GeglRectangle *bottom_rect,
                   GeglBuffer          *mask_buffer,
                   const GeglRectangle *mask_rect,
                   GeglBuffer          *dest_buffer,
                   const GeglRectangle *dest_rect,
                   gdouble              opacity,
                   const gboolean      *affect)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (top_buffer, top_rect, 0,
                                   babl_format ("RGBA float"),
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, bottom_buffer, bottom_rect, 0,
                            babl_format ("RGBA float"),
                            GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, mask_buffer, mask_rect, 0,
                            babl_format ("Y float"),
                            GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            babl_format ("RGBA float"),
                            GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *top    = iter->data[0];
      const gfloat *bottom = iter->data[1];
      const gfloat *mask   = iter->data[2];
      gfloat       *dest   = iter->data[3];
      gint          count  = iter->length;

      while (count--)
        {
          gint    b;
          gdouble mask_val = *mask * opacity;

          /* calculate new alpha first. */
          gfloat   s1_a  = bottom[3];
          gfloat   s2_a  = top[3];
          gdouble  a_val = s1_a + mask_val * (s2_a - s1_a);

          if (a_val == 0.0)
            {
              /* In any case, write out versions of the blending
               * function that result when combinations of s1_a, s2_a,
               * and mask_val --> 0 (or mask_val -->1)
               */

              /* 1: s1_a, s2_a, AND mask_val all approach 0+: */
              /* 2: s1_a AND s2_a both approach 0+, regardless of mask_val: */
              if (s1_a + s2_a == 0.0)
                {
                  for (b = 0; b < 3; b++)
                    {
                      gfloat new_val;

                      new_val = bottom[b] + mask_val * (top[b] - bottom[b]);

                      dest[b] = affect[b] ? MIN (new_val, 1.0) : bottom[b];
                    }
                }

              /* 3: mask_val AND s1_a both approach 0+, regardless of s2_a  */
              else if (s1_a + mask_val == 0.0)
                {
                  for (b = 0; b < 3; b++)
                    dest[b] = bottom[b];
                }

              /* 4: mask_val -->1 AND s2_a -->0, regardless of s1_a */
              else if (1.0 - mask_val + s2_a == 0.0)
                {
                  for (b = 0; b < 3; b++)
                    dest[b] = affect[b] ? top[b] : bottom[b];
                }
            }
          else
            {
              gdouble a_recip = 1.0 / a_val;

              /* possible optimization: fold a_recip into s1_a and s2_a */
              for (b = 0; b < 3; b++)
                {
                  gfloat new_val = a_recip * (bottom[b] * s1_a + mask_val *
                                              (top[b] * s2_a - bottom[b] * s1_a));
                  dest[b] = affect[b] ? MIN (new_val, 1.0) : bottom[b];
                }
            }

          dest[3] = affect[3] ? a_val : s1_a;

          top    += 4;
          bottom += 4;
          mask   += 1;
          dest   += 4;
        }
    }
}
