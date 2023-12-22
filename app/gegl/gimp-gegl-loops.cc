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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <gegl-buffer-backend.h>

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
#include "core/gimp-utils.h"
#include "core/gimpprogress.h"


#define PIXELS_PER_THREAD \
  (/* each thread costs as much as */ 64.0 * 64.0 /* pixels */)

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
  GeglRectangle real_dest_rect;

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  if (! src_rect)
    src_rect = gegl_buffer_get_extent (src_buffer);

  if (! dest_rect)
    dest_rect = src_rect;

  real_dest_rect        = *dest_rect;
  real_dest_rect.width  = src_rect->width;
  real_dest_rect.height = src_rect->height;

  dest_rect = &real_dest_rect;

  if (gegl_buffer_get_format (src_buffer) ==
      gegl_buffer_get_format (dest_buffer))
    {
      gboolean      skip_abyss = FALSE;
      GeglRectangle src_abyss;
      GeglRectangle dest_abyss;

      if (abyss_policy == GEGL_ABYSS_NONE)
        {
          src_abyss  = *gegl_buffer_get_abyss (src_buffer);
          dest_abyss = *gegl_buffer_get_abyss (dest_buffer);

          skip_abyss = ! (gegl_rectangle_contains (&src_abyss,  src_rect) &&
                          gegl_rectangle_contains (&dest_abyss, dest_rect));
        }

      if (skip_abyss)
        {
          if (src_buffer < dest_buffer)
            {
              gegl_tile_handler_lock (GEGL_TILE_HANDLER (src_buffer));
              gegl_tile_handler_lock (GEGL_TILE_HANDLER (dest_buffer));
            }
          else
            {
              gegl_tile_handler_lock (GEGL_TILE_HANDLER (dest_buffer));
              gegl_tile_handler_lock (GEGL_TILE_HANDLER (src_buffer));
            }

          gegl_buffer_set_abyss (src_buffer,  src_rect);
          gegl_buffer_set_abyss (dest_buffer, dest_rect);
        }

      gegl_buffer_copy (src_buffer, src_rect, abyss_policy,
                        dest_buffer, dest_rect);

      if (skip_abyss)
        {
          gegl_buffer_set_abyss (src_buffer,  &src_abyss);
          gegl_buffer_set_abyss (dest_buffer, &dest_abyss);

          gegl_tile_handler_unlock (GEGL_TILE_HANDLER (src_buffer));
          gegl_tile_handler_unlock (GEGL_TILE_HANDLER (dest_buffer));
        }
    }
  else
    {
      gegl_parallel_distribute_area (
        src_rect, PIXELS_PER_THREAD,
        [=] (const GeglRectangle *src_area)
        {
          SHIFTED_AREA (dest, src);

          gegl_buffer_copy (src_buffer, src_area, abyss_policy,
                            dest_buffer, dest_area);
        });
    }
}

void
gimp_gegl_clear (GeglBuffer          *buffer,
                 const GeglRectangle *rect)
{
  const Babl *format;
  gint        bpp;
  gint        n_components;
  gint        bpc;
  gint        alpha_offset;

  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  if (! rect)
    rect = gegl_buffer_get_extent (buffer);

  format = gegl_buffer_get_format (buffer);

  if (! babl_format_has_alpha (format))
    return;

  bpp          = babl_format_get_bytes_per_pixel (format);
  n_components = babl_format_get_n_components (format);
  bpc          = bpp / n_components;
  alpha_offset = (n_components - 1) * bpc;

  gegl_parallel_distribute_area (
    rect, PIXELS_PER_THREAD,
    [=] (const GeglRectangle *area)
    {
      GeglBufferIterator *iter;

      iter = gegl_buffer_iterator_new (buffer, area, 0, format,
                                       GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE,
                                       1);

      while (gegl_buffer_iterator_next (iter))
        {
          guint8 *data = (guint8 *) iter->items[0].data;
          gint    i;

          data += alpha_offset;

          for (i = 0; i < iter->length; i++)
            {
              memset (data, 0, bpc);

              data += bpp;
            }
        }
    });
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
                                   babl_format_has_alpha (src_format),
                                   babl_format_get_space (src_format));
  else
    src_format = gimp_babl_format (gimp_babl_format_get_base_type (src_format),
                                   GIMP_PRECISION_FLOAT_LINEAR,
                                   babl_format_has_alpha (src_format),
                                   babl_format_get_space (src_format));

  dest_format = gegl_buffer_get_format (dest_buffer);

  if (babl_format_is_palette (dest_format))
    dest_format = gimp_babl_format (GIMP_RGB,
                                    GIMP_PRECISION_FLOAT_LINEAR,
                                    babl_format_has_alpha (dest_format),
                                    babl_format_get_space (dest_format));
  else
    dest_format = gimp_babl_format (gimp_babl_format_get_base_type (dest_format),
                                    GIMP_PRECISION_FLOAT_LINEAR,
                                    babl_format_has_alpha (dest_format),
                                    babl_format_get_space (dest_format));

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

  gegl_parallel_distribute_area (
    dest_rect, PIXELS_PER_THREAD,
    [=] (const GeglRectangle *dest_area)
    {
      const gint          components  = src_components;
      const gint          a_component = components - 1;
      const gint          margin      = kernel_size / 2;
      GeglBufferIterator *dest_iter;

      /* Set up dest iterator */
      dest_iter = gegl_buffer_iterator_new (dest_buffer, dest_area, 0, dest_format,
                                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);

      while (gegl_buffer_iterator_next (dest_iter))
        {
          /*  Convolve the src image using the convolution kernel, writing
           *  to dest Convolve is not tile-enabled--use accordingly
           */
          gfloat     *dest    = (gfloat *) dest_iter->items[0].data;
          const gint  x1      = 0;
          const gint  y1      = 0;
          const gint  x2      = src_rect->width  - 1;
          const gint  y2      = src_rect->height - 1;
          const gint  dest_x1 = dest_iter->items[0].roi.x;
          const gint  dest_y1 = dest_iter->items[0].roi.y;
          const gint  dest_x2 = dest_iter->items[0].roi.x + dest_iter->items[0].roi.width;
          const gint  dest_y2 = dest_iter->items[0].roi.y + dest_iter->items[0].roi.height;
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

              dest += dest_iter->items[0].roi.width * dest_components;
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

  gegl_parallel_distribute_area (
    src_rect, PIXELS_PER_THREAD,
    [=] (const GeglRectangle *src_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (dest, src);

      iter = gegl_buffer_iterator_new (src_buffer, src_area, 0,
                                       babl_format ("R'G'B'A float"),
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

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
              gfloat *src   = (gfloat *) iter->items[0].data;
              gfloat *dest  = (gfloat *) iter->items[1].data;
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
              gfloat *src   = (gfloat *) iter->items[0].data;
              gfloat *dest  = (gfloat *) iter->items[1].data;
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
              gfloat *src   = (gfloat *) iter->items[0].data;
              gfloat *dest  = (gfloat *) iter->items[1].data;
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
                             GeglColor           *brush_color,
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
#if COMPILE_SSE2_INTRINISICS
  gboolean       sse2 = (gimp_cpu_accel_get_support () &
                         GIMP_CPU_ACCEL_X86_SSE2);
#endif

  if (! accum_rect)
    accum_rect = gegl_buffer_get_extent (accum_buffer);

  if (! canvas_rect)
    canvas_rect = gegl_buffer_get_extent (canvas_buffer);

  /* convert brush color to linear RGBA float */
  if (brush_color)
    {
      gegl_color_get_pixel (brush_color, babl_format ("RGBA float"), brush_color_float);
      brush_a *= brush_color_float[3];
    }

  gegl_parallel_distribute_area (
    accum_rect, PIXELS_PER_THREAD,
    [=] (const GeglRectangle *accum_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (canvas, accum);

      iter = gegl_buffer_iterator_new (accum_buffer, accum_area, 0,
                                       babl_format ("RGBA float"),
                                       GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 3);

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
          gfloat       *accum  = (gfloat *)       iter->items[0].data;
          const gfloat *canvas = (const gfloat *) iter->items[1].data;
          gfloat       *paint  = (gfloat *)       iter->items[2].data;
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

  gegl_parallel_distribute_area (
    mask_rect, PIXELS_PER_THREAD,
    [=] (const GeglRectangle *mask_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (dest, mask);

      iter = gegl_buffer_iterator_new (mask_buffer, mask_area, 0,
                                       babl_format ("Y float"),
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

      gegl_buffer_iterator_add (iter, dest_buffer, dest_area, 0,
                                babl_format ("RGBA float"),
                                GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          const gfloat *mask  = (const gfloat *) iter->items[0].data;
          gfloat       *dest  = (gfloat *)       iter->items[1].data;
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

  gegl_parallel_distribute_area (
    mask_rect, PIXELS_PER_THREAD,
    [=] (const GeglRectangle *mask_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (dest, mask);

      iter = gegl_buffer_iterator_new (mask_buffer, mask_area, 0,
                                       babl_format ("Y float"),
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

      gegl_buffer_iterator_add (iter, dest_buffer, dest_area, 0,
                                babl_format ("Y float"),
                                GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          const gfloat *mask  = (const gfloat *) iter->items[0].data;
          gfloat       *dest  = (gfloat *)       iter->items[1].data;
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

  gegl_parallel_distribute_area (
    mask_rect, PIXELS_PER_THREAD,
    [=] (const GeglRectangle *mask_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (dest, mask);

      iter = gegl_buffer_iterator_new (mask_buffer, mask_area, 0,
                                       babl_format ("Y float"),
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

      gegl_buffer_iterator_add (iter, dest_buffer, dest_area, 0,
                                babl_format ("Y float"),
                                GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          const gfloat *mask  = (const gfloat *) iter->items[0].data;
          gfloat       *dest  = (gfloat *)       iter->items[1].data;
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

  gegl_parallel_distribute_area (
    indexed_rect, PIXELS_PER_THREAD,
    [=] (const GeglRectangle *indexed_area)
    {
      GeglBufferIterator *iter;

      SHIFTED_AREA (mask, indexed);

      iter = gegl_buffer_iterator_new (indexed_buffer, indexed_area, 0,
                                       indexed_format,
                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

      gegl_buffer_iterator_add (iter, mask_buffer, mask_area, 0,
                                babl_format ("Y float"),
                                GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          const guchar *indexed = (const guchar *) iter->items[0].data;
          gfloat       *mask    = (gfloat *)       iter->items[1].data;
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

gboolean
gimp_gegl_is_index_used (GeglBuffer          *indexed_buffer,
                         const GeglRectangle *indexed_rect,
                         const Babl          *indexed_format,
                         gint                 index)
{
  GeglBufferIterator *iter;
  gboolean            found = FALSE;

  if (! indexed_rect)
    indexed_rect = gegl_buffer_get_extent (indexed_buffer);

  iter = gegl_buffer_iterator_new (indexed_buffer, indexed_rect, 0,
                                   indexed_format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);

  /* I initially had an implementation using gegl_parallel_distribute_area()
   * which turned out to be much slower than the simpler iteration on the whole
   * buffer at once. I think the cost of threading and using GRWLock is just far
   * too high for such very basic value check.
   * See gegl_parallel_distribute_area() implementation in commit dbaa8b6a1c.
   */
  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *indexed = (const guchar *) iter->items[0].data;
      gint          count   = iter->length;

      while (count--)
        {
          if (*indexed == index)
            {
              /*
               * Position of one item using this color index:
               gint x = iter->items[0].roi.x + (iter->length - count - 1) % iter->items[0].roi.width;
               gint y = iter->items[0].roi.y + (gint) ((iter->length - count - 1) / iter->items[0].roi.width);
               */
              found = TRUE;
              break;
            }
          indexed++;
        }

      if (found)
        {
          gegl_buffer_iterator_stop (iter);
          break;
        }
    }

  return found;
}

void
gimp_gegl_shift_index (GeglBuffer          *indexed_buffer,
                       const GeglRectangle *indexed_rect,
                       const Babl          *indexed_format,
                       gint                 from_index,
                       gint                 shift)
{
  gboolean indexed_format_has_alpha;

  if (! indexed_rect)
    indexed_rect = gegl_buffer_get_extent (indexed_buffer);

  indexed_format_has_alpha = babl_format_has_alpha (indexed_format);

  gegl_parallel_distribute_area (
    indexed_rect, PIXELS_PER_THREAD,
    [=] (const GeglRectangle *indexed_area)
    {
      GeglBufferIterator *iter;

      iter = gegl_buffer_iterator_new (indexed_buffer, indexed_area, 0,
                                       indexed_format,
                                       GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);

      while (gegl_buffer_iterator_next (iter))
        {
          guchar *indexed = (guchar *) iter->items[0].data;
          gint    count   = iter->length;

          while (count--)
            {
              if (*indexed >= from_index)
                *indexed += shift;

              indexed += (indexed_format_has_alpha ? 2 : 1);
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

  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (GIMP_IS_COLOR_PROFILE (src_profile));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));
  g_return_if_fail (GIMP_IS_COLOR_PROFILE (dest_profile));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

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

      gegl_parallel_distribute_area (
        src_rect, PIXELS_PER_THREAD,
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

  const Babl    *average_format;
  GeglRectangle  roi;
  GSList        *sums    = NULL;
  GSList        *list;
  Sum            average = {};
  gint           c;

  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (color != NULL);

  average_format = babl_format_with_space ("RaGaBaA float",
                                           babl_format_get_space (format));

  if (! rect)
    rect = gegl_buffer_get_extent (buffer);

  if (! format)
    format = gegl_buffer_get_format (buffer);

  if (clip_to_buffer)
    gegl_rectangle_intersect (&roi, rect, gegl_buffer_get_extent (buffer));
  else
    roi = *rect;

  gegl_parallel_distribute_area (
    &roi, PIXELS_PER_THREAD,
    [&] (const GeglRectangle *area)
    {
      Sum                *sum;
      GeglBufferIterator *iter;
      gfloat              color[4] = {};
      gint                n        = 0;

      iter = gegl_buffer_iterator_new (buffer, area, 0, average_format,
                                       GEGL_BUFFER_READ, abyss_policy, 1);

      while (gegl_buffer_iterator_next (iter))
        {
          const gfloat *p = (const gfloat *) iter->items[0].data;
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
