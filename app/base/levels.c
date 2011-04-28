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

#include <cairo.h>
#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "base-types.h"

#include "levels.h"


/*  public functions  */

void
levels_init (Levels *levels)
{
  GimpHistogramChannel channel;

  g_return_if_fail (levels != NULL);

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      levels->gamma[channel]       = 1.0;
      levels->low_input[channel]   = 0;
      levels->high_input[channel]  = 255;
      levels->low_output[channel]  = 0;
      levels->high_output[channel] = 255;
    }
}

gfloat
levels_lut_func (Levels *levels,
                 gint    n_channels,
                 gint    channel,
                 gfloat  value)
{
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

      /*  determine input intensity  */
      if (levels->high_input[j] != levels->low_input[j])
        {
          inten = ((gdouble) (255.0 * inten - levels->low_input[j]) /
                   (gdouble) (levels->high_input[j] - levels->low_input[j]));
        }
      else
        {
          inten = (gdouble) (255.0 * inten - levels->low_input[j]);
        }

      /* clamp to new black and white points */
      inten = CLAMP (inten, 0.0, 1.0);

      if (levels->gamma[j] != 0.0)
        {
          inten =  pow ( inten, (1.0 / levels->gamma[j]));
        }

      /*  determine the output intensity  */
      if (levels->high_output[j] >= levels->low_output[j])
        inten = (gdouble) (inten * (levels->high_output[j] -
                                    levels->low_output[j]) +
                           levels->low_output[j]);
      else if (levels->high_output[j] < levels->low_output[j])
        inten = (gdouble) (levels->low_output[j] - inten *
                           (levels->low_output[j] - levels->high_output[j]));

      inten /= 255.0;
    }

  return inten;
}
