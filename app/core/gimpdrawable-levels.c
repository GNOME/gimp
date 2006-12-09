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

#include <string.h>

#include <glib-object.h>

#include "core-types.h"

#include "base/gimphistogram.h"
#include "base/gimplut.h"
#include "base/levels.h"
#include "base/lut-funcs.h"
#include "base/pixel-processor.h"
#include "base/pixel-region.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpdrawable.h"
#include "gimpdrawable-histogram.h"
#include "gimpdrawable-levels.h"

#include "gimp-intl.h"

void
gimp_drawable_levels (GimpDrawable   *drawable,
                      GimpContext    *context,
                      gint32          channel,
                      gint32          low_input,
                      gint32          high_input,
                      gdouble         gamma,
                      gint32          low_output,
                      gint32          high_output)
{
  gint         x, y, width, height;
  PixelRegion  srcPR, destPR;
  Levels       levels;
  GimpLut     *lut;

  /* parameter checking */
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (! gimp_drawable_is_indexed (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (channel >= GIMP_HISTOGRAM_VALUE &&
                    channel <= GIMP_HISTOGRAM_ALPHA);
  g_return_if_fail (low_input   >= 0   && low_input   <= 255);
  g_return_if_fail (high_input  >= 0   && high_input  <= 255);
  g_return_if_fail (gamma       >= 0.1 && gamma       <= 10.0);
  g_return_if_fail (low_output  >= 0   && low_output  <= 255);
  g_return_if_fail (high_output >= 0   && high_output <= 255);

  if (channel == GIMP_HISTOGRAM_ALPHA)
    g_return_if_fail (gimp_drawable_has_alpha (drawable));

  if (gimp_drawable_is_gray (drawable))
    g_return_if_fail (channel == GIMP_HISTOGRAM_VALUE ||
                      channel == GIMP_HISTOGRAM_ALPHA);

  if (! gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    return;

  /* FIXME: hack */
  if (gimp_drawable_is_gray (drawable) &&
      channel == GIMP_HISTOGRAM_ALPHA)
    channel = 1;

  lut = gimp_lut_new ();

  levels_init (&levels);

  levels.low_input[channel]   = low_input;
  levels.high_input[channel]  = high_input;
  levels.gamma[channel]       = gamma;
  levels.low_output[channel]  = low_output;
  levels.high_output[channel] = high_output;

  /* setup the lut */
  gimp_lut_setup (lut,
                  (GimpLutFunc) levels_lut_func,
                   &levels,
                   gimp_drawable_bytes (drawable));

  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     x, y, width, height, FALSE);
  pixel_region_init (&destPR, gimp_drawable_get_shadow_tiles (drawable),
                     x, y, width, height, TRUE);

  pixel_regions_process_parallel ((PixelProcessorFunc) gimp_lut_process,
                                  lut, 2, &srcPR, &destPR);

  gimp_lut_free (lut);

  gimp_drawable_merge_shadow (drawable, TRUE, _("Levels"));
  gimp_drawable_update (drawable, x, y, width, height);
}


void
gimp_drawable_levels_stretch (GimpDrawable *drawable,
                              GimpContext  *context)
{
  gint           x, y, width, height;
  PixelRegion    srcPR, destPR;
  Levels         levels;
  GimpLut       *lut;
  GimpHistogram *hist;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (! gimp_drawable_is_indexed (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_CONTEXT  (context));

  if (! gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    return;

  /* Build the histogram */
  hist = gimp_histogram_new ();

  gimp_drawable_calculate_histogram (drawable, hist);

  /* Calculate the levels */
  levels_init    (&levels);
  levels_stretch (&levels, hist, ! gimp_drawable_is_gray (drawable));

  /* Set up the lut */
  lut  = gimp_lut_new ();
  gimp_lut_setup (lut,
                  (GimpLutFunc) levels_lut_func,
                  &levels,
                  gimp_drawable_bytes (drawable));

  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     x, y, width, height, FALSE);
  pixel_region_init (&destPR, gimp_drawable_get_shadow_tiles (drawable),
                     x, y, width, height, TRUE);

  pixel_regions_process_parallel ((PixelProcessorFunc) gimp_lut_process,
                                  lut, 2, &srcPR, &destPR);

  gimp_lut_free (lut);
  gimp_histogram_free (hist);

  gimp_drawable_merge_shadow (drawable, TRUE, _("Levels"));
  gimp_drawable_update (drawable, x, y, width, height);
}
