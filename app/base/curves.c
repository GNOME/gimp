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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "base-types.h"

#include "curves.h"
#include "gimplut.h"


/*  public functions  */

void
curves_init (Curves *curves)
{
  GimpHistogramChannel channel;

  g_return_if_fail (curves != NULL);

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      gint j;

      for (j = 0; j < 256; j++)
        curves->curve[channel][j] = j;
    }
}

gfloat
curves_lut_func (Curves *curves,
                 gint    n_channels,
                 gint    channel,
                 gfloat  value)
{
  gfloat  f;
  gint    index;
  gdouble inten;
  gint    j;

  if (n_channels <= 2)
    j = channel;
  else
    j = channel + 1;

  inten = value;

  /* For RGB and RGBA images this runs through the loop with j = channel + 1
   * the first time and j = 0 the second time
   *
   * For GRAY images this runs through the loop with j = 0 the first and
   * only time
   */
  for (; j >= 0; j -= (channel + 1))
    {
      /* don't apply the overall curve to the alpha channel */
      if (j == 0 && (n_channels == 2 || n_channels == 4) &&
          channel == n_channels - 1)
        return inten;

      if (inten < 0.0)
        {
          inten = curves->curve[j][0]/255.0;
        }
      else if (inten >= 1.0)
        {
          inten = curves->curve[j][255]/255.0;
        }
      else /* interpolate the curve */
        {
          index = floor (inten * 255.0);
          f = inten * 255.0 - index;
          inten = ((1.0 - f) * curves->curve[j][index    ] +
                          f  * curves->curve[j][index + 1] ) / 255.0;
        }
    }

  return inten;
}
