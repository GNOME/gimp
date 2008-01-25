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

#include <gegl.h>

#include "core-types.h"

#include "base/gimphistogram.h"
#include "base/gimplut.h"
#include "base/levels.h"
#include "base/pixel-processor.h"
#include "base/pixel-region.h"

#include "gegl/gimplevelsconfig.h"

/* temp */
#include "gimp.h"
#include "gimpimage.h"

#include "gimpcontext.h"
#include "gimpdrawable.h"
#include "gimpdrawable-histogram.h"
#include "gimpdrawable-levels.h"
#include "gimpdrawable-operation.h"

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
  GimpLevelsConfig *config;

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

  config = g_object_new (GIMP_TYPE_LEVELS_CONFIG, NULL);

  g_object_set (config,
                "channel", channel,
                NULL);

  g_object_set (config,
                "low-input",   low_input   / 255.0,
                "high-input",  high_input  / 255.0,
                "gamma",       gamma,
                "low-output",  low_output  / 255.0,
                "high-output", high_output / 255.0,
                NULL);

  if (gimp_use_gegl (GIMP_ITEM (drawable)->image->gimp))
    {
      GeglNode *levels;

      levels = g_object_new (GEGL_TYPE_NODE,
                             "operation", "gimp-levels",
                             NULL);

      gegl_node_set (levels,
                     "config", config,
                     NULL);

      gimp_drawable_apply_operation (drawable, levels, TRUE,
                                     NULL, _("Levels"));

      g_object_unref (levels);
    }
  else
    {
      PixelRegion  srcPR, destPR;
      Levels       levels;
      GimpLut     *lut;
      gint         x, y;
      gint         width, height;

      if (! gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
        return;

      gimp_levels_config_to_cruft (config, &levels,
                                   gimp_drawable_is_rgb (drawable));

      lut = gimp_lut_new ();
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

  g_object_unref (config);
}

void
gimp_drawable_levels_stretch (GimpDrawable *drawable,
                              GimpContext  *context)
{
  GimpLevelsConfig *config;
  GimpHistogram    *histogram;
  gint              x, y;
  gint              width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (! gimp_drawable_is_indexed (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_CONTEXT  (context));

  if (! gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    return;

  config = g_object_new (GIMP_TYPE_LEVELS_CONFIG, NULL);

  histogram = gimp_histogram_new ();
  gimp_drawable_calculate_histogram (drawable, histogram);

  gimp_levels_config_stretch (config, histogram,
                              gimp_drawable_is_rgb (drawable));

  gimp_histogram_free (histogram);

  if (gimp_use_gegl (GIMP_ITEM (drawable)->image->gimp))
    {
      GeglNode *levels;

      levels = g_object_new (GEGL_TYPE_NODE,
                             "operation", "gimp-levels",
                             NULL);

      gegl_node_set (levels,
                     "config", config,
                     NULL);

      gimp_drawable_apply_operation (drawable, levels, TRUE,
                                     NULL, _("Levels"));

      g_object_unref (levels);
    }
  else
    {
      PixelRegion  srcPR, destPR;
      Levels       levels;
      GimpLut     *lut;

      gimp_levels_config_to_cruft (config, &levels,
                                   gimp_drawable_is_rgb (drawable));

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

      gimp_drawable_merge_shadow (drawable, TRUE, _("Levels"));

      gimp_drawable_update (drawable, x, y, width, height);
    }

  g_object_unref (config);
}
