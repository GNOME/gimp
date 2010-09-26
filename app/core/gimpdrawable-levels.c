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

#include <gegl.h>

#include "core-types.h"

#include "base/gimphistogram.h"
#include "base/gimplut.h"
#include "base/levels.h"

#include "gegl/gimplevelsconfig.h"

/* temp */
#include "gimp.h"
#include "gimpimage.h"

#include "gimpdrawable.h"
#include "gimpdrawable-histogram.h"
#include "gimpdrawable-levels.h"
#include "gimpdrawable-operation.h"
#include "gimpdrawable-process.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_drawable_levels_internal (GimpDrawable     *drawable,
                                             GimpProgress     *progress,
                                             GimpLevelsConfig *config);


/*  public functions  */

void
gimp_drawable_levels (GimpDrawable *drawable,
                      GimpProgress *progress,
                      gint32        channel,
                      gint32        low_input,
                      gint32        high_input,
                      gdouble       gamma,
                      gint32        low_output,
                      gint32        high_output)
{
  GimpLevelsConfig *config;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (! gimp_drawable_is_indexed (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
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

  gimp_drawable_levels_internal (drawable, progress, config);

  g_object_unref (config);
}

void
gimp_drawable_levels_stretch (GimpDrawable *drawable,
                              GimpProgress *progress)
{
  GimpLevelsConfig *config;
  GimpHistogram    *histogram;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (! gimp_drawable_is_indexed (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable), NULL, NULL, NULL, NULL))
    return;

  config = g_object_new (GIMP_TYPE_LEVELS_CONFIG, NULL);

  histogram = gimp_histogram_new ();
  gimp_drawable_calculate_histogram (drawable, histogram);

  gimp_levels_config_stretch (config, histogram,
                              gimp_drawable_is_rgb (drawable));

  gimp_histogram_unref (histogram);

  gimp_drawable_levels_internal (drawable, progress, config);

  g_object_unref (config);
}


/*  private functions  */

static void
gimp_drawable_levels_internal (GimpDrawable     *drawable,
                               GimpProgress     *progress,
                               GimpLevelsConfig *config)
{
  if (gimp_use_gegl (gimp_item_get_image (GIMP_ITEM (drawable))->gimp))
    {
      GeglNode *levels;

      levels = g_object_new (GEGL_TYPE_NODE,
                             "operation", "gimp:levels",
                             NULL);

      gegl_node_set (levels,
                     "config", config,
                     NULL);

      gimp_drawable_apply_operation (drawable, progress, _("Levels"),
                                     levels, TRUE);

      g_object_unref (levels);
    }
  else
    {
      Levels   levels;
      GimpLut *lut = gimp_lut_new ();

      gimp_levels_config_to_cruft (config, &levels,
                                   gimp_drawable_is_rgb (drawable));
      gimp_lut_setup (lut,
                      (GimpLutFunc) levels_lut_func, &levels,
                      gimp_drawable_bytes (drawable));

      gimp_drawable_process_lut (drawable, progress, _("Levels"), lut);
      gimp_lut_free (lut);
    }
}
