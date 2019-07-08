/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-loops-sse2.c
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

#include "gimp-gegl-types.h"

#include "gimp-gegl-loops-sse2.h"


#if COMPILE_SSE2_INTRINISICS

#include <emmintrin.h>


/* helper function of gimp_gegl_smudge_with_paint_process_sse2()
 * src and dest can be the same address
 */
static inline void
gimp_gegl_smudge_with_paint_blend_sse2 (const gfloat *src1,
                                        gfloat        src1_rate,
                                        const gfloat *src2,
                                        gfloat        src2_rate,
                                        gfloat       *dest,
                                        gboolean      no_erasing_src2)
{
  /* 2017/4/13 shark0r : According to my test, SSE decreases about 25%
   * execution time
   */

  __m128  v_src1 = _mm_loadu_ps (src1);
  __m128  v_src2 = _mm_loadu_ps (src2);
  __m128 *v_dest = (__v4sf *) dest;

  gfloat  orginal_src2_alpha;
  gfloat  src1_alpha;
  gfloat  src2_alpha;
  gfloat  result_alpha;

  orginal_src2_alpha = v_src2[3];
  src1_alpha         = src1_rate * v_src1[3];
  src2_alpha         = src2_rate * orginal_src2_alpha;
  result_alpha       = src1_alpha + src2_alpha;

  if (result_alpha == 0)
    {
      *v_dest = _mm_set1_ps (0);
      return;
    }

  *v_dest = (v_src1 * _mm_set1_ps (src1_alpha) +
             v_src2 * _mm_set1_ps (src2_alpha)) /
            _mm_set1_ps (result_alpha);

  if (no_erasing_src2)
    {
      result_alpha = MAX (result_alpha, orginal_src2_alpha);
    }

  dest[3] = result_alpha;
}

/* helper function of gimp_gegl_smudge_with_paint()
 *
 * note that it's the caller's responsibility to verify that the buffers are
 * properly aligned
 */
void
gimp_gegl_smudge_with_paint_process_sse2 (gfloat       *accum,
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
      gimp_gegl_smudge_with_paint_blend_sse2 (accum, rate, canvas, 1 - rate,
                                              accum, no_erasing);

      /* blend accum_buffer and brush color/pixmap to paint_buffer */
      if (brush_a == 0) /* pure smudge */
        {
          memcpy (paint, accum, sizeof (gfloat) * 4);
        }
      else
        {
          const gfloat *src1 = brush_color ? brush_color : paint;

          gimp_gegl_smudge_with_paint_blend_sse2 (src1, flow, accum, 1 - flow,
                                                  paint, no_erasing);
        }

      accum  += 4;
      canvas += 4;
      paint  += 4;
    }
}

#endif /* COMPILE_SSE2_INTRINISICS */
