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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimpcurve.h"
#include "gimpcurve-map.h"


gdouble
gimp_curve_map_value (GimpCurve *curve,
                      gdouble    value)
{
  g_return_val_if_fail (GIMP_IS_CURVE (curve), 0.0);

  if (curve->identity)
    {
      return value;
    }

  if (value < 0.0)
    {
      return curve->samples[0];
    }
  else if (value >= 1.0)
    {
      return curve->samples[curve->n_samples - 1];
    }
  else  /* interpolate the curve */
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
}

void
gimp_curve_map_pixels (GimpCurve *curve_all,
                       GimpCurve *curve_red,
                       GimpCurve *curve_green,
                       GimpCurve *curve_blue,
                       GimpCurve *curve_alpha,
                       gfloat    *src,
                       gfloat    *dest,
                       glong      samples)
{
  guint mask = 0;

  g_return_if_fail (GIMP_IS_CURVE (curve_all));
  g_return_if_fail (GIMP_IS_CURVE (curve_red));
  g_return_if_fail (GIMP_IS_CURVE (curve_green));
  g_return_if_fail (GIMP_IS_CURVE (curve_blue));
  g_return_if_fail (GIMP_IS_CURVE (curve_alpha));

  mask |= (gimp_curve_is_identity (curve_all)   ? 0 : 1) << 0;
  mask |= (gimp_curve_is_identity (curve_red)   ? 0 : 1) << 1;
  mask |= (gimp_curve_is_identity (curve_green) ? 0 : 1) << 2;
  mask |= (gimp_curve_is_identity (curve_blue)  ? 0 : 1) << 3;
  mask |= (gimp_curve_is_identity (curve_alpha) ? 0 : 1) << 4;

  switch (mask)
    {
    case 0:  /*  all curves are identity, nothing to do      */
      break;

    case 1:  /*  only the overall curve needs to be applied  */
      while (samples--)
        {
          dest[0] = gimp_curve_map_value (curve_all, src[0]);
          dest[1] = gimp_curve_map_value (curve_all, src[1]);
          dest[2] = gimp_curve_map_value (curve_all, src[2]);
          /* don't apply the overall curve to the alpha channel */
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

    case 2:  /*  only the red curve needs to be applied      */
      while (samples--)
        {
          dest[0] = gimp_curve_map_value (curve_red, src[0]);
          dest[1] = src[1];
          dest[2] = src[2];
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

   case 4:  /*  only the green curve needs to be applied     */
      while (samples--)
        {
          dest[0] = src[0];
          dest[1] = gimp_curve_map_value (curve_green, src[1]);
          dest[2] = src[2];
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

    case 8:  /*  only the blue curve needs to be applied     */
      while (samples--)
        {
          dest[0] = src[0];
          dest[1] = src[1];
          dest[2] = gimp_curve_map_value (curve_green, src[2]);
          dest[3] = src[3];

          src  += 4;
          dest += 4;
        }
      break;

     case 16: /*  only the alpha curve needs to be applied   */
      while (samples--)
        {
          dest[0] = src[0];
          dest[1] = src[1];
          dest[2] = src[2];
          dest[3] = gimp_curve_map_value (curve_alpha, src[3]);

          src  += 4;
          dest += 4;
        }
      break;

    default: /*  apply all curves                            */
      while (samples--)
        {
          dest[0] = gimp_curve_map_value (curve_all,
                                          gimp_curve_map_value (curve_red,
                                                                src[0]));
          dest[1] = gimp_curve_map_value (curve_all,
                                          gimp_curve_map_value (curve_green,
                                                                src[1]));
          dest[2] = gimp_curve_map_value (curve_all,
                                          gimp_curve_map_value (curve_blue,
                                                                src[2]));
          /* don't apply the overall curve to the alpha channel */
          dest[3] = gimp_curve_map_value (curve_alpha, src[3]);

          src  += 4;
          dest += 4;
        }
    }
}
