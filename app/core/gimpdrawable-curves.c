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

#include "base/curves.h"
#include "base/gimplut.h"

#include "gegl/gimpcurvesconfig.h"

/* temp */
#include "gimp.h"
#include "gimpimage.h"

#include "gimpcurve.h"
#include "gimpdrawable.h"
#include "gimpdrawable-curves.h"
#include "gimpdrawable-operation.h"
#include "gimpdrawable-process.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_drawable_curves (GimpDrawable     *drawable,
                                    GimpProgress     *progress,
                                    GimpCurvesConfig *config);


/*  public functions  */

void
gimp_drawable_curves_spline (GimpDrawable *drawable,
                             GimpProgress *progress,
                             gint32        channel,
                             const guint8 *points,
                             gint          n_points)
{
  GimpCurvesConfig *config;
  GimpCurve        *curve;
  gint              i;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (! gimp_drawable_is_indexed (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (channel >= GIMP_HISTOGRAM_VALUE &&
                    channel <= GIMP_HISTOGRAM_ALPHA);

  if (channel == GIMP_HISTOGRAM_ALPHA)
    g_return_if_fail (gimp_drawable_has_alpha (drawable));

  if (gimp_drawable_is_gray (drawable))
    g_return_if_fail (channel == GIMP_HISTOGRAM_VALUE ||
                      channel == GIMP_HISTOGRAM_ALPHA);

  config = g_object_new (GIMP_TYPE_CURVES_CONFIG, NULL);

  curve = config->curve[channel];

  gimp_data_freeze (GIMP_DATA (curve));

  /* FIXME: create a curves object with the right number of points */
  /*  unset the last point  */
  gimp_curve_set_point (curve, curve->n_points - 1, -1, -1);

  n_points = MIN (n_points / 2, curve->n_points);

  for (i = 0; i < n_points; i++)
    gimp_curve_set_point (curve, i,
                          (gdouble) points[i * 2]     / 255.0,
                          (gdouble) points[i * 2 + 1] / 255.0);

  gimp_data_thaw (GIMP_DATA (curve));

  gimp_drawable_curves (drawable, progress, config);

  g_object_unref (config);
}

void
gimp_drawable_curves_explicit (GimpDrawable *drawable,
                               GimpProgress *progress,
                               gint32        channel,
                               const guint8 *points,
                               gint          n_points)
{
  GimpCurvesConfig *config;
  GimpCurve        *curve;
  gint              i;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (! gimp_drawable_is_indexed (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (channel >= GIMP_HISTOGRAM_VALUE &&
                    channel <= GIMP_HISTOGRAM_ALPHA);

  if (channel == GIMP_HISTOGRAM_ALPHA)
    g_return_if_fail (gimp_drawable_has_alpha (drawable));

  if (gimp_drawable_is_gray (drawable))
    g_return_if_fail (channel == GIMP_HISTOGRAM_VALUE ||
                      channel == GIMP_HISTOGRAM_ALPHA);

  config = g_object_new (GIMP_TYPE_CURVES_CONFIG, NULL);

  curve = config->curve[channel];

  gimp_data_freeze (GIMP_DATA (curve));

  gimp_curve_set_curve_type (curve, GIMP_CURVE_FREE);

  for (i = 0; i < 256; i++)
    gimp_curve_set_curve (curve,
                          (gdouble) i         / 255.0,
                          (gdouble) points[i] / 255.0);

  gimp_data_thaw (GIMP_DATA (curve));

  gimp_drawable_curves (drawable, progress, config);

  g_object_unref (config);
}


/*  private functions  */

static void
gimp_drawable_curves (GimpDrawable     *drawable,
                      GimpProgress     *progress,
                      GimpCurvesConfig *config)
{
  if (gimp_use_gegl (gimp_item_get_image (GIMP_ITEM (drawable))->gimp))
    {
      GeglNode *node;

      node = g_object_new (GEGL_TYPE_NODE,
                           "operation", "gimp:curves",
                           NULL);
      gegl_node_set (node,
                     "config", config,
                     NULL);

      gimp_drawable_apply_operation (drawable, progress, C_("undo-type", "Curves"),
                                     node, TRUE);
      g_object_unref (node);
    }
  else
    {
      GimpLut *lut = gimp_lut_new ();
      Curves   cruft;

      gimp_curves_config_to_cruft (config, &cruft,
                                   gimp_drawable_is_rgb (drawable));

      gimp_lut_setup (lut,
                      (GimpLutFunc) curves_lut_func,
                      &cruft,
                      gimp_drawable_bytes (drawable));

      gimp_drawable_process_lut (drawable, progress, C_("undo-type", "Curves"), lut);
      gimp_lut_free (lut);
    }
}
