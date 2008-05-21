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

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/desaturate.h"
#include "base/pixel-processor.h"
#include "base/pixel-region.h"

#include "gegl/gimpdesaturateconfig.h"

/* temp */
#include "gimp.h"
#include "gimpimage.h"

#include "gimpdrawable.h"
#include "gimpdrawable-desaturate.h"
#include "gimpdrawable-operation.h"
#include "gimpdrawable-shadow.h"

#include "gimp-intl.h"


void
gimp_drawable_desaturate (GimpDrawable       *drawable,
                          GimpDesaturateMode  mode)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_drawable_is_rgb (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  if (gimp_use_gegl (GIMP_ITEM (drawable)->image->gimp))
    {
      GeglNode *desaturate;
      GObject  *config;

      desaturate = g_object_new (GEGL_TYPE_NODE,
                                 "operation", "gimp-desaturate",
                                 NULL);

      config = g_object_new (GIMP_TYPE_DESATURATE_CONFIG,
                             "mode", mode,
                             NULL);

      gegl_node_set (desaturate,
                     "config", config,
                     NULL);

      g_object_unref (config);

      gimp_drawable_apply_operation (drawable, desaturate, TRUE,
                                     NULL, _("Desaturate"));

      g_object_unref  (desaturate);
    }
  else
    {
      PixelRegion         srcPR, destPR;
      PixelProcessorFunc  function;
      gint                x, y;
      gint                width, height;
      gboolean            has_alpha;

      if (! gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
        return;

      switch (mode)
        {
        case GIMP_DESATURATE_LIGHTNESS:
          function = (PixelProcessorFunc) desaturate_region_lightness;
          break;

          break;
        case GIMP_DESATURATE_LUMINOSITY:
          function = (PixelProcessorFunc) desaturate_region_luminosity;
          break;

        case GIMP_DESATURATE_AVERAGE:
          function = (PixelProcessorFunc) desaturate_region_average;
          break;

        default:
          g_return_if_reached ();
          return;
        }

      has_alpha = gimp_drawable_has_alpha (drawable);

      pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                         x, y, width, height, FALSE);
      pixel_region_init (&destPR, gimp_drawable_get_shadow_tiles (drawable),
                         x, y, width, height, TRUE);

      pixel_regions_process_parallel (function, GINT_TO_POINTER (has_alpha),
                                      2, &srcPR, &destPR);

      gimp_drawable_merge_shadow_tiles (drawable, TRUE, _("Desaturate"));
      gimp_drawable_free_shadow_tiles (drawable);

      gimp_drawable_update (drawable, x, y, width, height);
    }
}
