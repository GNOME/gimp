/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "ligmacurve.h"
#include "ligmacurve-map.h"


enum
{
  CURVE_NONE   = 0,
  CURVE_COLORS = 1 << 0,
  CURVE_RED    = 1 << 1,
  CURVE_GREEN  = 1 << 2,
  CURVE_BLUE   = 1 << 3,
  CURVE_ALPHA  = 1 << 4
};

static guint           ligma_curve_get_apply_mask   (LigmaCurve *curve_colors,
                                                    LigmaCurve *curve_red,
                                                    LigmaCurve *curve_green,
                                                    LigmaCurve *curve_blue,
                                                    LigmaCurve *curve_alpha);
static inline gdouble  ligma_curve_map_value_inline (LigmaCurve *curve,
                                                    gdouble    value);


gdouble
ligma_curve_map_value (LigmaCurve *curve,
                      gdouble    value)
{
  g_return_val_if_fail (LIGMA_IS_CURVE (curve), 0.0);

  return ligma_curve_map_value_inline (curve, value);
}

void
ligma_curve_map_pixels (LigmaCurve *curve_colors,
                       LigmaCurve *curve_red,
                       LigmaCurve *curve_green,
                       LigmaCurve *curve_blue,
                       LigmaCurve *curve_alpha,
                       gfloat    *src,
                       gfloat    *dest,
                       glong      samples)
{
  g_return_if_fail (LIGMA_IS_CURVE (curve_colors));
  g_return_if_fail (LIGMA_IS_CURVE (curve_red));
  g_return_if_fail (LIGMA_IS_CURVE (curve_green));
  g_return_if_fail (LIGMA_IS_CURVE (curve_blue));
  g_return_if_fail (LIGMA_IS_CURVE (curve_alpha));

  switch (ligma_curve_get_apply_mask (curve_colors,
                                     curve_red,
                                     curve_green,
                                     curve_blue,
                                     curve_alpha))
    {
    case CURVE_NONE:
      memcpy (dest, src, samples * 4 * sizeof (gfloat));
      break;

    case CURVE_COLORS:
      while (samples--)
        {
          dest[0] = ligma_curve_map_value_inline (curve_colors, src[0]);
          dest[1] = ligma_curve_map_value_inline (curve_colors, src[1]);
          dest[2] = ligma_curve_map_value_inline (curve_colors, src[2]);
          /* don't apply the colors curve to the alpha channel */
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

    case CURVE_RED:
      while (samples--)
        {
          dest[0] = ligma_curve_map_value_inline (curve_red, src[0]);
          dest[1] = src[1];
          dest[2] = src[2];
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

    case CURVE_GREEN:
      while (samples--)
        {
          dest[0] = src[0];
          dest[1] = ligma_curve_map_value_inline (curve_green, src[1]);
          dest[2] = src[2];
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

    case CURVE_BLUE:
      while (samples--)
        {
          dest[0] = src[0];
          dest[1] = src[1];
          dest[2] = ligma_curve_map_value_inline (curve_blue, src[2]);
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

     case CURVE_ALPHA:
      while (samples--)
        {
          dest[0] = src[0];
          dest[1] = src[1];
          dest[2] = src[2];
          dest[3] = ligma_curve_map_value_inline (curve_alpha, src[3]);

          src  += 4;
          dest += 4;
        }
      break;

    case (CURVE_RED | CURVE_GREEN | CURVE_BLUE):
      while (samples--)
        {
          dest[0] = ligma_curve_map_value_inline (curve_red,   src[0]);
          dest[1] = ligma_curve_map_value_inline (curve_green, src[1]);
          dest[2] = ligma_curve_map_value_inline (curve_blue,  src[2]);
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

    default:
      while (samples--)
        {
          dest[0] = ligma_curve_map_value_inline (curve_colors,
                                                 ligma_curve_map_value_inline (curve_red,
                                                                              src[0]));
          dest[1] = ligma_curve_map_value_inline (curve_colors,
                                                 ligma_curve_map_value_inline (curve_green,
                                                                              src[1]));
          dest[2] = ligma_curve_map_value_inline (curve_colors,
                                                 ligma_curve_map_value_inline (curve_blue,
                                                                              src[2]));
          /* don't apply the colors curve to the alpha channel */
          dest[3] = ligma_curve_map_value_inline (curve_alpha, src[3]);

          src  += 4;
          dest += 4;
        }
      break;
    }
}

static guint
ligma_curve_get_apply_mask (LigmaCurve *curve_colors,
                           LigmaCurve *curve_red,
                           LigmaCurve *curve_green,
                           LigmaCurve *curve_blue,
                           LigmaCurve *curve_alpha)
{
  return ((ligma_curve_is_identity (curve_colors) ? 0 : CURVE_COLORS) |
          (ligma_curve_is_identity (curve_red)    ? 0 : CURVE_RED)    |
          (ligma_curve_is_identity (curve_green)  ? 0 : CURVE_GREEN)  |
          (ligma_curve_is_identity (curve_blue)   ? 0 : CURVE_BLUE)   |
          (ligma_curve_is_identity (curve_alpha)  ? 0 : CURVE_ALPHA));
}

static inline gdouble
ligma_curve_map_value_inline (LigmaCurve *curve,
                             gdouble    value)
{
  if (curve->identity)
    {
      if (isfinite (value))
        return CLAMP (value, 0.0, 1.0);

      return 0.0;
    }

  /*  check for known values first, so broken values like NaN
   *  delivered by broken drivers don't run into the interpolation
   *  code
   */
  if (value > 0.0 && value < 1.0) /* interpolate the curve */
    {
      gdouble f;
      gint    index;

      /*  map value to the sample space  */
      value = value * (curve->n_samples - 1);

      /*  determine the indices of the closest sample points  */
      index = (gint) value;

      /*  calculate the position between the sample points  */
      f = value - index;

      return (1.0 - f) * curve->samples[index] + f * curve->samples[index + 1];
    }
  else if (value >= 1.0)
    {
      return curve->samples[curve->n_samples - 1];
    }
  else
    {
      return curve->samples[0];
    }
}
