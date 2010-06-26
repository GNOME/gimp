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

#include "base/hue-saturation.h"

#include "gegl/gimphuesaturationconfig.h"

/* temp */
#include "gimp.h"
#include "gimpimage.h"

#include "gimpdrawable.h"
#include "gimpdrawable-hue-saturation.h"
#include "gimpdrawable-operation.h"
#include "gimpdrawable-process.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_drawable_hue_saturation (GimpDrawable *drawable,
                              GimpProgress *progress,
                              GimpHueRange  range,
                              gdouble       hue,
                              gdouble       saturation,
                              gdouble       lightness)
{
  GimpHueSaturationConfig *config;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (! gimp_drawable_is_indexed (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  config = g_object_new (GIMP_TYPE_HUE_SATURATION_CONFIG,
                         "range", range,
                         NULL);

  g_object_set (config,
                "hue",        hue        / 180.0,
                "saturation", saturation / 100.0,
                "lightness",  lightness  / 100.0,
                NULL);

  if (gimp_use_gegl (gimp_item_get_image (GIMP_ITEM (drawable))->gimp))
    {
      GeglNode *node;

      node = g_object_new (GEGL_TYPE_NODE,
                           "operation", "gimp:hue-saturation",
                           NULL);
      gegl_node_set (node,
                     "config", config,
                     NULL);

      gimp_drawable_apply_operation (drawable, progress, _("Hue-Saturation"),
                                     node, TRUE);
      g_object_unref (node);
    }
  else
    {
      HueSaturation cruft;

      gimp_hue_saturation_config_to_cruft (config, &cruft);

      gimp_drawable_process (drawable, progress, _("Hue_Saturation"),
                             (PixelProcessorFunc) hue_saturation, &cruft);
    }

  g_object_unref (config);
}
