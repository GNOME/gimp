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

#include <glib-object.h>

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "gimpdrawable.h"
#include "gimpdrawable-shadow.h"


TileManager *
gimp_drawable_get_shadow_tiles (GimpDrawable *drawable)
{
  GimpItem *item;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  item = GIMP_ITEM (drawable);

  if (drawable->shadow)
    {
      if ((gimp_item_width  (item) != tile_manager_width  (drawable->shadow)) ||
          (gimp_item_height (item) != tile_manager_height (drawable->shadow)) ||
          (drawable->bytes         != tile_manager_bpp    (drawable->shadow)))
        {
          gimp_drawable_free_shadow_tiles (drawable);
        }
      else
        {
          return drawable->shadow;
        }
    }

  drawable->shadow = tile_manager_new (gimp_item_width  (item),
                                       gimp_item_height (item),
                                       drawable->bytes);

  return drawable->shadow;
}

void
gimp_drawable_free_shadow_tiles (GimpDrawable *drawable)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  if (drawable->shadow)
    {
      tile_manager_unref (drawable->shadow);
      drawable->shadow = NULL;
    }
}

void
gimp_drawable_merge_shadow_tiles (GimpDrawable *drawable,
                                  gboolean      push_undo,
                                  const gchar  *undo_desc)
{
  gint x, y;
  gint width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (drawable->shadow != NULL);

  /*  A useful optimization here is to limit the update to the
   *  extents of the selection mask, as it cannot extend beyond
   *  them.
   */
  if (gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    {
      TileManager *tiles = tile_manager_ref (drawable->shadow);
      PixelRegion  shadowPR;

      pixel_region_init (&shadowPR, tiles, x, y, width, height, FALSE);

      gimp_drawable_apply_region (drawable, &shadowPR,
                                  push_undo, undo_desc,
                                  GIMP_OPACITY_OPAQUE, GIMP_REPLACE_MODE,
                                  NULL, x, y);

      tile_manager_unref (tiles);
    }
}
