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

#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

extern "C"
{

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "gimp-babl.h"
#include "gimp-gegl-loops.h"
#include "gimp-gegl-loops-sse2.h"

#include "core/gimp-atomic.h"
#include "core/gimp-parallel.h"
#include "core/gimp-utils.h"
#include "core/gimpprogress.h"


#define MIN_PARALLEL_SUB_SIZE 64
#define MIN_PARALLEL_SUB_AREA (MIN_PARALLEL_SUB_SIZE * MIN_PARALLEL_SUB_SIZE)

#define SHIFTED_AREA(dest, src)                                                \
  const GeglRectangle dest##_area_ = {                                         \
    src##_area->x + (dest##_rect->x - src##_rect->x),                          \
    src##_area->y + (dest##_rect->y - src##_rect->y),                          \
    src##_area->width, src##_area->height                                      \
  };                                                                           \
  const GeglRectangle * const dest##_area = &dest##_area_


void
gimp_gegl_buffer_copy (GeglBuffer          *src_buffer,
                       const GeglRectangle *src_rect,
                       GeglAbyssPolicy      abyss_policy,
                       GeglBuffer          *dest_buffer,
                       const GeglRectangle *dest_rect)
{
  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  if (gegl_buffer_get_format (src_buffer) ==
      gegl_buffer_get_format (dest_buffer))
    {
      gegl_buffer_copy (src_buffer, src_rect, abyss_policy,
                        dest_buffer, dest_rect);
    }
  else
    {
      if (! src_rect)
        src_rect = gegl_buffer_get_extent (src_buffer);

      if (! dest_rect)
        dest_rect = src_rect;

      gimp_parallel_distribute_area (src_rect, MIN_PARALLEL_SUB_AREA,
                                     [=] (const GeglRectangle *src_area)
        {
          SHIFTED_AREA (dest, src);

          gegl_buffer_copy (src_buffer, src_area, abyss_policy,
                            dest_buffer, dest_area);
        });
    }
}

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
  gfloat     *src;
  gint        src_rowstride;

  const Babl *src_format;
  const Babl *dest_format;
  gint        src_components;
  gint        dest_components;
  gfloat      offset;

  if (! src_rect)
    src_rect = gegl_buffer_get_extent (src_buffer);

  if (! dest_rect)
    dest_rect = gegl_buffer_get_extent (dest_buffer);

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

  /* Get source pixel data */
  src_rowstride = src_components * src_rect->width;
  src = g_new (gfloat, src_rowstride * src_rect->height);
  gegl_buffer_get (src_buffer, src_rect, 1.0, src_format, src,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  /*  If the mode is NEGATIVE_CONVOL, the offset should be 0.5  */
  if (mode == GIMP_NEGATIVE_CONVOL)
    {
      offset = 0.5;
      mode = GIMP_NORMAL_CONVOL;
    }
  else
    {
      offset = 0.0;
    }

  gimp_parallel_distribute_area (dest_rect, MIN_PARALLEL_SUB_AREA,
                                 [=] (const GeglRectangle *dest_area)
    {
      const gint          components  = src_components;
      const gint          a_component = components - 1;
      const gint          margin      = kernel_size / 2;
      GeglBufferIterator *dest_iter;

      /* Set up dest iterator */
      dest_iter = gegl_buffer_iterator_new (dest_buffer, dest_area, 0, dest_format,
                                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (dest_iter))
        {
          /*  Convolve the src image using the convolution kernel, writing
           *  to dest Convolve is not tile-enabled--use accordingly
           */
          gfloat     *dest    = (gfloat *) dest_iter->data[0];
          const gint  x1      = 0;
          const gint  y1      = 0;
          const gint  x2      = src_rect->width  - 1;
          const gint  y2      = src_rect->height - 1;
          const gint  dest_x1 = dest_iter->roi[0].x;
          const gint  dest_y1 = dest_iter->roi[0].y;
          const gint  dest_x2 = dest_iter->roi[0].x + dest_iter->roi[0].width;
          const gint  dest_y2 = dest_iter->roi[0].y + dest_iter->roi[0].height;
          gint        x, y;

          for (y = dest_y1; y < dest_y2; y++)
            {
              gfloat *d = dest;

              if (alpha_weighting)
                {
                  for (x = dest_x1; x < dest_x2; x++)
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
                              const gfloat *s  = src + yy * src_rowstride + xx * components;
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
                  for (x = dest_x1; x < dest_x2; x++)
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
                              const gfloat *s  = src + yy * src_rowstride + xx * components;

                              for (b = 0; b < components; b++)
                                total[b] += *m * s[b];
                            }
                        }

                      for (b = 0; b < components; b++)
                        {
                          total[b] = total[b] / divisor + offset;

                          if (mode != GIMP_NORMAL_CONVOL && total[b] < 0.0)
                            total[b] = - total[b];

                          *d++ = CLAMP (total[b], 0.0, 1.0);
                        }
                    }
                }

              dest += dest_iter->roi[0].width * dest_components;
            }
        }
    });

  g_free (src);
}

static inline gfloat
odd_powf (gfloat x,
          gfloat y)
{
  if (x >= 0.0f)
    return  powf ( x, y);
  else
    return -powf (-x, y);
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
  if (type == GIMP_DODGE_BURN_TYPE_BURN)
    exposure = -exposure;

  if (! src_rect)
    src_rect = gegl_buffer_get_extent (src_buffer);

  if (! dest_rect)
    dest_rect = gegl_buffer_get_extent (dest_buffer);

  gimp_parallel_distribute_area (src_rect, MIN_PARALLEL_SUB_AREA,
                                 [=] (const GeglRectangle *src_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (dest, src);

      iter = gegl_buffer_iterator_new (src_buffer, src_area, 0,
                                       babl_format ("R'G'B'A float"),
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (iter, dest_buffer, dest_area, 0,
                                babl_format ("R'G'B'A float"),
                                GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

      switch (mode)
        {
          gfloat factor;

        case GIMP_TRANSFER_HIGHLIGHTS:
          factor = 1.0 + exposure * (0.333333);

          while (gegl_buffer_iterator_next (iter))
            {
              gfloat *src   = (gfloat *) iter->data[0];
              gfloat *dest  = (gfloat *) iter->data[1];
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

        case GIMP_TRANSFER_MIDTONES:
          if (exposure < 0)
            factor = 1.0 - exposure * (0.333333);
          else
            factor = 1.0 / (1.0 + exposure);

          while (gegl_buffer_iterator_next (iter))
            {
              gfloat *src   = (gfloat *) iter->data[0];
              gfloat *dest  = (gfloat *) iter->data[1];
              gint    count = iter->length;

              while (count--)
                {
                  *dest++ = odd_powf (*src++, factor);
                  *dest++ = odd_powf (*src++, factor);
                  *dest++ = odd_powf (*src++, factor);

                  *dest++ = *src++;
                }
            }
          break;

        case GIMP_TRANSFER_SHADOWS:
          if (exposure >= 0)
            factor = 0.333333 * exposure;
          else
            factor = -0.333333 * exposure;

          while (gegl_buffer_iterator_next (iter))
            {
              gfloat *src   = (gfloat *) iter->data[0];
              gfloat *dest  = (gfloat *) iter->data[1];
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
    });
}

/* helper function of gimp_gegl_smudge_with_paint_process()
   src and dest can be the same address
 */
static inline void
gimp_gegl_smudge_with_paint_blend (const gfloat *src1,
                                   gfloat        src1_rate,
                                   const gfloat *src2,
                                   gfloat        src2_rate,
                                   gfloat       *dest,
                                   gboolean      no_erasing_src2)
{
  gfloat orginal_src2_alpha;
  gfloat src1_alpha;
  gfloat src2_alpha;
  gfloat result_alpha;
  gint   b;

  orginal_src2_alpha = src2[3];
  src1_alpha         = src1_rate * src1[3];
  src2_alpha         = src2_rate * orginal_src2_alpha;
  result_alpha       = src1_alpha + src2_alpha;

  if (result_alpha == 0)
    {
      memset (dest, 0, sizeof (gfloat) * 4);
      return;
    }

  for (b = 0; b < 3; b++)
    dest[b] = (src1[b] * src1_alpha + src2[b] * src2_alpha) / result_alpha;

  if (no_erasing_src2)
    {
      result_alpha = MAX (result_alpha, orginal_src2_alpha);
    }

  dest[3] = result_alpha;
}

/* helper function of gimp_gegl_smudge_with_paint() */
static void
gimp_gegl_smudge_with_paint_process (gfloat       *accum,
                                     const gfloat *canvas,
                                     gfloat       *paint,
                                     gint          count,
                                     const gfloat *brush_color,
                                     gfloat        brush_a,
                                     gboolean      no_erasing,
                                     gfloat        flow,
                                     gfloat        rate)
{
  while (count--)
    {
      /* blend accum_buffer and canvas_buffer to accum_buffer */
      gimp_gegl_smudge_with_paint_blend (accum, rate, canvas, 1 - rate,
                                         accum, no_erasing);

      /* blend accum_buffer and brush color/pixmap to paint_buffer */
      if (brush_a == 0) /* pure smudge */
        {
          memcpy (paint, accum, sizeof (gfloat) * 4);
        }
      else
        {
          const gfloat *src1 = brush_color ? brush_color : paint;

          gimp_gegl_smudge_with_paint_blend (src1, flow, accum, 1 - flow,
                                             paint, no_erasing);
        }

      accum  += 4;
      canvas += 4;
      paint  += 4;
    }
}

/*  smudge painting calculation. Currently only smudge tool uses this function
 *  Accum = rate*Accum + (1-rate)*Canvas
 *  if brush_color!=NULL
 *    Paint = flow*brushColor + (1-flow)*Accum
 *  else
 *    Paint = flow*Paint + (1-flow)*Accum
 */
void
gimp_gegl_smudge_with_paint (GeglBuffer          *accum_buffer,
                             const GeglRectangle *accum_rect,
                             GeglBuffer          *canvas_buffer,
                             const GeglRectangle *canvas_rect,
                             const GimpRGB       *brush_color,
                             GeglBuffer          *paint_buffer,
                             gboolean             no_erasing,
                             gdouble              flow,
                             gdouble              rate)
{
  gfloat         brush_color_float[4];
  gfloat         brush_a = flow;
  GeglAccessMode paint_buffer_access_mode = (brush_color ?
                                             GEGL_ACCESS_WRITE :
                                             GEGL_ACCESS_READWRITE);
  gboolean       sse2 = (gimp_cpu_accel_get_support () &
                         GIMP_CPU_ACCEL_X86_SSE2);

  if (! accum_rect)
    accum_rect = gegl_buffer_get_extent (accum_buffer);

  if (! canvas_rect)
    canvas_rect = gegl_buffer_get_extent (canvas_buffer);

  /* convert brush color from double to float */
  if (brush_color)
    {
      const gdouble *brush_color_ptr = &brush_color->r;
      gint           b;

      for (b = 0; b < 4; b++)
        brush_color_float[b] = brush_color_ptr[b];

      brush_a *= brush_color_ptr[3];
    }

  gimp_parallel_distribute_area (accum_rect, MIN_PARALLEL_SUB_AREA,
                                 [=] (const GeglRectangle *accum_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (canvas, accum);

      iter = gegl_buffer_iterator_new (accum_buffer, accum_area, 0,
                                       babl_format ("RGBA float"),
                                       GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (iter, canvas_buffer, canvas_area, 0,
                                babl_format ("RGBA float"),
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (iter, paint_buffer,
                                GEGL_RECTANGLE (accum_area->x - accum_rect->x,
                                                accum_area->y - accum_rect->y,
                                                0, 0),
                                0,
                                babl_format ("RGBA float"),
                                paint_buffer_access_mode, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          gfloat       *accum  = (gfloat *)       iter->data[0];
          const gfloat *canvas = (const gfloat *) iter->data[1];
          gfloat       *paint  = (gfloat *)       iter->data[2];
          gint          count  = iter->length;

#if COMPILE_SSE2_INTRINISICS
          if (sse2 && ((guintptr) accum                                     |
                       (guintptr) canvas                                    |
                       (guintptr) (brush_color ? brush_color_float : paint) |
                       (guintptr) paint) % 16 == 0)
            {
              gimp_gegl_smudge_with_paint_process_sse2 (accum, canvas, paint, count,
                                                        brush_color ? brush_color_float :
                                                                      NULL,
                                                        brush_a,
                                                        no_erasing, flow, rate);
            }
          else
#endif
            {
              gimp_gegl_smudge_with_paint_process (accum, canvas, paint, count,
                                                   brush_color ? brush_color_float :
                                                                 NULL,
                                                   brush_a,
                                                   no_erasing, flow, rate);
            }
        }
    });
}

void
gimp_gegl_apply_mask (GeglBuffer          *mask_buffer,
                      const GeglRectangle *mask_rect,
                      GeglBuffer          *dest_buffer,
                      const GeglRectangle *dest_rect,
                      gdouble              opacity)
{
  if (! mask_rect)
    mask_rect = gegl_buffer_get_extent (mask_buffer);

  if (! dest_rect)
    dest_rect = gegl_buffer_get_extent (dest_buffer);

  gimp_parallel_distribute_area (mask_rect, MIN_PARALLEL_SUB_AREA,
                                 [=] (const GeglRectangle *mask_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (dest, mask);

      iter = gegl_buffer_iterator_new (mask_buffer, mask_area, 0,
                                       babl_format ("Y float"),
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (iter, dest_buffer, dest_area, 0,
                                babl_format ("RGBA float"),
                                GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          const gfloat *mask  = (const gfloat *) iter->data[0];
          gfloat       *dest  = (gfloat *)       iter->data[1];
          gint          count = iter->length;

          while (count--)
            {
              dest[3] *= *mask * opacity;

              mask += 1;
              dest += 4;
            }
        }
    });
}

void
gimp_gegl_combine_mask (GeglBuffer          *mask_buffer,
                        const GeglRectangle *mask_rect,
                        GeglBuffer          *dest_buffer,
                        const GeglRectangle *dest_rect,
                        gdouble              opacity)
{
  if (! mask_rect)
    mask_rect = gegl_buffer_get_extent (mask_buffer);

  if (! dest_rect)
    dest_rect = gegl_buffer_get_extent (dest_buffer);

  gimp_parallel_distribute_area (mask_rect, MIN_PARALLEL_SUB_AREA,
                                 [=] (const GeglRectangle *mask_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (dest, mask);

      iter = gegl_buffer_iterator_new (mask_buffer, mask_area, 0,
                                       babl_format ("Y float"),
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (iter, dest_buffer, dest_area, 0,
                                babl_format ("Y float"),
                                GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          const gfloat *mask  = (const gfloat *) iter->data[0];
          gfloat       *dest  = (gfloat *)       iter->data[1];
          gint          count = iter->length;

          while (count--)
            {
              *dest *= *mask * opacity;

              mask += 1;
              dest += 1;
            }
        }
    });
}

void
gimp_gegl_combine_mask_weird (GeglBuffer          *mask_buffer,
                              const GeglRectangle *mask_rect,
                              GeglBuffer          *dest_buffer,
                              const GeglRectangle *dest_rect,
                              gdouble              opacity,
                              gboolean             stipple)
{
  if (! mask_rect)
    mask_rect = gegl_buffer_get_extent (mask_buffer);

  if (! dest_rect)
    dest_rect = gegl_buffer_get_extent (dest_buffer);

  gimp_parallel_distribute_area (mask_rect, MIN_PARALLEL_SUB_AREA,
                                 [=] (const GeglRectangle *mask_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (dest, mask);

      iter = gegl_buffer_iterator_new (mask_buffer, mask_area, 0,
                                       babl_format ("Y float"),
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (iter, dest_buffer, dest_area, 0,
                                babl_format ("Y float"),
                                GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          const gfloat *mask  = (const gfloat *) iter->data[0];
          gfloat       *dest  = (gfloat *)       iter->data[1];
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
    });
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
  if (! top_rect)
    top_rect = gegl_buffer_get_extent (top_buffer);

  if (! bottom_rect)
    bottom_rect = gegl_buffer_get_extent (bottom_buffer);

  if (! mask_rect)
    mask_rect = gegl_buffer_get_extent (mask_buffer);

  if (! dest_rect)
    dest_rect = gegl_buffer_get_extent (dest_buffer);

  gimp_parallel_distribute_area (top_rect, MIN_PARALLEL_SUB_AREA,
                                 [=] (const GeglRectangle *top_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (bottom, top);
      SHIFTED_AREA (mask, top);
      SHIFTED_AREA (dest, top);

      iter = gegl_buffer_iterator_new (top_buffer, top_area, 0,
                                       babl_format ("RGBA float"),
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (iter, bottom_buffer, bottom_area, 0,
                                babl_format ("RGBA float"),
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (iter, mask_buffer, mask_area, 0,
                                babl_format ("Y float"),
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (iter, dest_buffer, dest_area, 0,
                                babl_format ("RGBA float"),
                                GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          const gfloat *top    = (const gfloat *) iter->data[0];
          const gfloat *bottom = (const gfloat *) iter->data[1];
          const gfloat *mask   = (const gfloat *) iter->data[2];
          gfloat       *dest   = (gfloat *)       iter->data[3];
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

                          dest[b] = affect[b] ? new_val : bottom[b];
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
                      dest[b] = affect[b] ? new_val : bottom[b];
                    }
                }

              dest[3] = affect[3] ? a_val : s1_a;

              top    += 4;
              bottom += 4;
              mask   += 1;
              dest   += 4;
            }
        }
    });
}

void
gimp_gegl_index_to_mask (GeglBuffer          *indexed_buffer,
                         const GeglRectangle *indexed_rect,
                         const Babl          *indexed_format,
                         GeglBuffer          *mask_buffer,
                         const GeglRectangle *mask_rect,
                         gint                 index)
{
  if (! indexed_rect)
    indexed_rect = gegl_buffer_get_extent (indexed_buffer);

  if (! mask_rect)
    mask_rect = gegl_buffer_get_extent (mask_buffer);

  gimp_parallel_distribute_area (indexed_rect, MIN_PARALLEL_SUB_AREA,
                                 [=] (const GeglRectangle *indexed_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (mask, indexed);

      iter = gegl_buffer_iterator_new (indexed_buffer, indexed_area, 0,
                                       indexed_format,
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (iter, mask_buffer, mask_area, 0,
                                babl_format ("Y float"),
                                GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          const guchar *indexed = (const guchar *) iter->data[0];
          gfloat       *mask    = (gfloat *)       iter->data[1];
          gint          count   = iter->length;

          while (count--)
            {
              if (*indexed == index)
                *mask = 1.0;
              else
                *mask = 0.0;

              indexed++;
              mask++;
            }
        }
    });
}

static void
gimp_gegl_convert_color_profile_progress (GimpProgress *progress,
                                          gdouble       value)
{
  if (gegl_is_main_thread ())
    gimp_progress_set_value (progress, value);
}

void
gimp_gegl_convert_color_profile (GeglBuffer               *src_buffer,
                                 const GeglRectangle      *src_rect,
                                 GimpColorProfile         *src_profile,
                                 GeglBuffer               *dest_buffer,
                                 const GeglRectangle      *dest_rect,
                                 GimpColorProfile         *dest_profile,
                                 GimpColorRenderingIntent  intent,
                                 gboolean                  bpc,
                                 GimpProgress             *progress)
{
  GimpColorTransform *transform;
  guint               flags = 0;
  const Babl         *src_format;
  const Babl         *dest_format;

  src_format  = gegl_buffer_get_format (src_buffer);
  dest_format = gegl_buffer_get_format (dest_buffer);

  if (bpc)
    flags |= GIMP_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION;

  flags |= GIMP_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE;

  transform = gimp_color_transform_new (src_profile,  src_format,
                                        dest_profile, dest_format,
                                        intent,
                                        (GimpColorTransformFlags) flags);

  if (! src_rect)
    src_rect = gegl_buffer_get_extent (src_buffer);

  if (! dest_rect)
    dest_rect = gegl_buffer_get_extent (dest_buffer);

  if (transform)
    {
      if (progress)
        {
          g_signal_connect_swapped (
            transform, "progress",
            G_CALLBACK (gimp_gegl_convert_color_profile_progress),
            progress);
        }

      GIMP_TIMER_START ();

      gimp_parallel_distribute_area (src_rect, MIN_PARALLEL_SUB_AREA,
                                     [=] (const GeglRectangle *src_area)
        {
          SHIFTED_AREA (dest, src);

          gimp_color_transform_process_buffer (transform,
                                               src_buffer,  src_area,
                                               dest_buffer, dest_area);
        });

      GIMP_TIMER_END ("converting buffer");

      g_object_unref (transform);
    }
  else
    {
      gimp_gegl_buffer_copy (src_buffer,  src_rect, GEGL_ABYSS_NONE,
                             dest_buffer, dest_rect);

      if (progress)
        gimp_progress_set_value (progress, 1.0);
    }
}

void
gimp_gegl_average_color (GeglBuffer          *buffer,
                         const GeglRectangle *rect,
                         gboolean             clip_to_buffer,
                         GeglAbyssPolicy      abyss_policy,
                         const Babl          *format,
                         gpointer             color)
{
  typedef struct
  {
    gfloat color[4];
    gint   n;
  } Sum;

  const Babl        *average_format = babl_format ("RaGaBaA float");
  GeglRectangle      roi;
  GSList * volatile  sums           = NULL;
  GSList            *list;
  Sum                average        = {};
  gint               c;

  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (color != NULL);

  if (! rect)
    rect = gegl_buffer_get_extent (buffer);

  if (! format)
    format = gegl_buffer_get_format (buffer);

  if (clip_to_buffer)
    gegl_rectangle_intersect (&roi, rect, gegl_buffer_get_extent (buffer));
  else
    roi = *rect;

  gimp_parallel_distribute_area (&roi, MIN_PARALLEL_SUB_AREA,
                                 [&] (const GeglRectangle *area)
    {
      Sum                *sum;
      GeglBufferIterator *iter;
      gfloat              color[4] = {};
      gint                n        = 0;

      iter = gegl_buffer_iterator_new (buffer, area, 0, average_format,
                                       GEGL_BUFFER_READ, abyss_policy);

      while (gegl_buffer_iterator_next (iter))
        {
          const gfloat *p = (const gfloat *) iter->data[0];
          gint          i;

          for (i = 0; i < iter->length; i++)
            {
              gint c;

              for (c = 0; c < 4; c++)
                color[c] += p[c];

              p += 4;
            }

          n += iter->length;
        }

      sum = g_slice_new (Sum);

      memcpy (sum->color, color, sizeof (color));
      sum->n = n;

      gimp_atomic_slist_push_head (&sums, sum);
    });

  for (list = sums; list; list = g_slist_next (list))
    {
      Sum *sum = (Sum *) list->data;

      for (c = 0; c < 4; c++)
        average.color[c] += sum->color[c];

      average.n += sum->n;

      g_slice_free (Sum, sum);
    }

  g_slist_free (sums);

  if (average.n > 0)
    {
      for (c = 0; c < 4; c++)
        average.color[c] /= average.n;
    }

  babl_process (babl_fish (average_format, format), average.color, color, 1);
}

} /* extern "C" */
