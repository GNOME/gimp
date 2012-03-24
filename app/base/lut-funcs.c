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

#include "core/core-types.h"

#include "core/gimphistogram.h"
#include "gimplut.h"
#include "lut-funcs.h"


/* --------------- equalize ------------- */

typedef struct
{
  GimpHistogram *histogram;
  gint           part[5][256];
} hist_lut_struct;

static gfloat
equalize_lut_func (hist_lut_struct *hlut,
                   gint             nchannels,
                   gint             channel,
                   gfloat           value)
{
  gint j;

  /* don't equalize the alpha channel */
  if ((nchannels == 2 || nchannels == 4) && channel == nchannels - 1)
    return value;

  j = RINT (CLAMP (value * 255.0, 0, 255));

  return hlut->part[channel][j] / 255.;
}

static void
equalize_lut_setup (GimpLut       *lut,
                    GimpHistogram *hist,
                    gint           n_channels)
{
  gint            i, k;
  hist_lut_struct hlut;
  gdouble         pixels;

  g_return_if_fail (lut != NULL);
  g_return_if_fail (hist != NULL);

  /* Find partition points */
  pixels = gimp_histogram_get_count (hist, GIMP_HISTOGRAM_VALUE, 0, 255);

  for (k = 0; k < n_channels; k++)
    {
      gdouble sum = 0;

      for (i = 0; i < 256; i++)
        {
          gdouble histi = gimp_histogram_get_channel (hist, k, i);

          sum += histi;

          hlut.part[k][i] = RINT (sum * 255. / pixels);
        }
    }

  gimp_lut_setup (lut, (GimpLutFunc) equalize_lut_func, &hlut, n_channels);
}

GimpLut *
equalize_lut_new (GimpHistogram *histogram,
                  gint           n_channels)
{
  GimpLut *lut;

  g_return_val_if_fail (histogram != NULL, NULL);

  lut = gimp_lut_new ();

  equalize_lut_setup (lut, histogram, n_channels);

  return lut;
}
