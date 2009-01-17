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

#include "base/gimplut.h"
#include "base/lut-funcs.h"

#include "gegl/gimpposterizeconfig.h"

/* temp */
#include "gimp.h"
#include "gimpimage.h"

#include "gimpdrawable.h"
#include "gimpdrawable-operation.h"
#include "gimpdrawable-posterize.h"
#include "gimpdrawable-process.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_drawable_posterize (GimpDrawable *drawable,
                         GimpProgress *progress,
                         gint          levels)
{
  GimpPosterizeConfig *config;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (! gimp_drawable_is_indexed (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  config = g_object_new (GIMP_TYPE_POSTERIZE_CONFIG,
                         "levels", levels,
                         NULL);

  if (gimp_use_gegl (gimp_item_get_image (GIMP_ITEM (drawable))->gimp))
    {
      GeglNode *node;

      node = g_object_new (GEGL_TYPE_NODE,
                           "operation", "gimp:posterize",
                           NULL);
      gegl_node_set (node,
                     "config", config,
                     NULL);

      gimp_drawable_apply_operation (drawable, progress, _("Posterize"),
                                     node, TRUE);
      g_object_unref (node);
    }
  else
    {
      GimpLut *lut;

      lut = posterize_lut_new (config->levels, gimp_drawable_bytes (drawable));

      gimp_drawable_process_lut (drawable, progress, _("Posterize"), lut);
      gimp_lut_free (lut);
    }

  g_object_unref (config);
}
