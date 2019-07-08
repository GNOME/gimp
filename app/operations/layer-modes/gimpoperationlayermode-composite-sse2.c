/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlayermode-composite-sse2.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *               2017 Øyvind Kolås <pippin@gimp.org>
 *               2017 Ell
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

#include <gegl-plugin.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "../operations-types.h"

#include "gimpoperationlayermode-composite.h"


#if COMPILE_SSE2_INTRINISICS

/* SSE2 */
#include <emmintrin.h>


/*  non-subtractive compositing functions.  these functions expect comp[ALPHA]
 *  to be the same as layer[ALPHA].  when in[ALPHA] or layer[ALPHA] are zero,
 *  the value of comp[RED..BLUE] is unconstrained (in particular, it may be
 *  NaN).
 */


void
gimp_operation_layer_mode_composite_clip_to_backdrop_sse2 (const gfloat *in,
                                                           const gfloat *layer,
                                                           const gfloat *comp,
                                                           const gfloat *mask,
                                                           gfloat        opacity,
                                                           gfloat       *out,
                                                           gint          samples)
{
  if ((((uintptr_t)in)   | /* alignment check */
       ((uintptr_t)comp) |
       ((uintptr_t)out)   ) & 0x0F)
    {
      gimp_operation_layer_mode_composite_clip_to_backdrop (in, layer, comp,
                                                            mask, opacity, out,
                                                            samples);
    }
  else
    {
      const __v4sf *v_in      = (const __v4sf*) in;
      const __v4sf *v_comp    = (const __v4sf*) comp;
            __v4sf *v_out     = (__v4sf*) out;
      const __v4sf  v_one     =  _mm_set1_ps (1.0f);
      const __v4sf  v_opacity =  _mm_set1_ps (opacity);

      while (samples--)
        {
          __v4sf alpha, rgba_in, rgba_comp;

          rgba_in   = *v_in ++;
          rgba_comp = *v_comp++;

          alpha = (__v4sf)_mm_shuffle_epi32((__m128i)rgba_comp,_MM_SHUFFLE(3,3,3,3)) * v_opacity;

          if (mask)
            {
              alpha = alpha * _mm_set1_ps (*mask++);
            }

          if (rgba_in[ALPHA] != 0.0f && _mm_ucomineq_ss (alpha, _mm_setzero_ps ()))
            {
              __v4sf out_pixel, out_pixel_rbaa, out_alpha;

              out_alpha      = (__v4sf)_mm_shuffle_epi32((__m128i)rgba_in,_MM_SHUFFLE(3,3,3,3));
              out_pixel      = rgba_comp * alpha + rgba_in * (v_one - alpha);
              out_pixel_rbaa = _mm_shuffle_ps (out_pixel, out_alpha, _MM_SHUFFLE (3, 3, 2, 0));
              out_pixel      = _mm_shuffle_ps (out_pixel, out_pixel_rbaa, _MM_SHUFFLE (2, 1, 1, 0));

              *v_out++ = out_pixel;
            }
          else
            {
              *v_out ++ = rgba_in;
            }
        }
    }
}

#endif /* COMPILE_SSE2_INTRINISICS */
