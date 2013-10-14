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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "operations/gimplevelsconfig.h"

#include "gimpdrawable.h"
#include "gimpdrawable-histogram.h"
#include "gimpdrawable-levels.h"
#include "gimpdrawable-operation.h"
#include "gimphistogram.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_drawable_levels_stretch (GimpDrawable *drawable,
                              GimpProgress *progress)
{
  GimpLevelsConfig *config;
  GimpHistogram    *histogram;
  GeglNode         *levels;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable), NULL, NULL, NULL, NULL))
    return;

  config = g_object_new (GIMP_TYPE_LEVELS_CONFIG, NULL);

  histogram = gimp_histogram_new (TRUE);
  gimp_drawable_calculate_histogram (drawable, histogram);

  gimp_levels_config_stretch (config, histogram,
                              gimp_drawable_is_rgb (drawable));

  g_object_unref (histogram);

  levels = g_object_new (GEGL_TYPE_NODE,
                         "operation", "gimp:levels",
                         NULL);

  gegl_node_set (levels,
                 "config", config,
                 NULL);

  gimp_drawable_apply_operation (drawable, progress, _("Levels"),
                                 levels);

  g_object_unref (levels);
  g_object_unref (config);
}
