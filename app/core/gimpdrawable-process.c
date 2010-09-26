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
#include "base/pixel-processor.h"
#include "base/pixel-region.h"

#include "gimpdrawable.h"
#include "gimpdrawable-process.h"
#include "gimpdrawable-shadow.h"
#include "gimpprogress.h"


void
gimp_drawable_process (GimpDrawable       *drawable,
                       GimpProgress       *progress,
                       const gchar        *undo_desc,
                       PixelProcessorFunc  func,
                       gpointer            data)
{
  gint x, y, width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (undo_desc != NULL);

  if (gimp_item_mask_intersect (GIMP_ITEM (drawable), &x, &y, &width, &height))
    {
      PixelRegion srcPR, destPR;

      pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                         x, y, width, height, FALSE);
      pixel_region_init (&destPR, gimp_drawable_get_shadow_tiles (drawable),
                         x, y, width, height, TRUE);

      pixel_regions_process_parallel (func, data, 2, &srcPR, &destPR);

      gimp_drawable_merge_shadow_tiles (drawable, TRUE, undo_desc);
      gimp_drawable_free_shadow_tiles (drawable);

      gimp_drawable_update (drawable, x, y, width, height);
    }
}
void
gimp_drawable_process_lut (GimpDrawable *drawable,
                           GimpProgress *progress,
                           const gchar  *undo_desc,
                           GimpLut      *lut)
{
  gimp_drawable_process (drawable, progress, undo_desc,
                         (PixelProcessorFunc) gimp_lut_process, lut);
}
