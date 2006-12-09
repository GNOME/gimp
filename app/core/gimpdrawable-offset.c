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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpdrawable.h"
#include "gimpdrawable-offset.h"
#include "gimpimage.h"

#include "gimp-intl.h"


void
gimp_drawable_offset (GimpDrawable   *drawable,
                      GimpContext    *context,
                      gboolean        wrap_around,
                      GimpOffsetType  fill_type,
                      gint            offset_x,
                      gint            offset_y)
{
  GimpItem    *item;
  PixelRegion  srcPR, destPR;
  TileManager *new_tiles;
  gint         width, height;
  gint         src_x, src_y;
  gint         dest_x, dest_y;
  guchar       fill[MAX_CHANNELS] = { 0 };

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  item = GIMP_ITEM (drawable);

  width  = gimp_item_width  (item);
  height = gimp_item_height (item);

  if (wrap_around)
    {
      /*  avoid modulo operation on negative values  */
      while (offset_x < 0)
        offset_x += width;
      while (offset_y < 0)
        offset_y += height;

      offset_x %= width;
      offset_y %= height;
    }
  else
    {
      offset_x = CLAMP (offset_x, -width, width);
      offset_y = CLAMP (offset_y, -height, height);
    }

  if (offset_x == 0 && offset_y == 0)
    return;

  new_tiles = tile_manager_new (width, height, gimp_drawable_bytes (drawable));
  if (offset_x >= 0)
    {
      src_x = 0;
      dest_x = offset_x;
      width = CLAMP ((width - offset_x), 0, width);
    }
  else
    {
      src_x = -offset_x;
      dest_x = 0;
      width = CLAMP ((width + offset_x), 0, width);
    }

  if (offset_y >= 0)
    {
      src_y = 0;
      dest_y = offset_y;
      height = CLAMP ((height - offset_y), 0, height);
    }
  else
    {
      src_y = -offset_y;
      dest_y = 0;
      height = CLAMP ((height + offset_y), 0, height);
    }

  /*  Copy the center region  */
  if (width && height)
    {
      pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                         src_x, src_y, width, height, FALSE);
      pixel_region_init (&destPR, new_tiles,
                         dest_x, dest_y, width, height, TRUE);

      copy_region (&srcPR, &destPR);
    }

  /*  Copy appropriately for wrap around  */
  if (wrap_around == TRUE)
    {
      if (offset_x >= 0 && offset_y >= 0)
        {
          src_x = gimp_item_width  (item) - offset_x;
          src_y = gimp_item_height (item) - offset_y;
        }
      else if (offset_x >= 0 && offset_y < 0)
        {
          src_x = gimp_item_width (item) - offset_x;
          src_y = 0;
        }
      else if (offset_x < 0 && offset_y >= 0)
        {
          src_x = 0;
          src_y = gimp_item_height (item) - offset_y;
        }
      else if (offset_x < 0 && offset_y < 0)
        {
          src_x = 0;
          src_y = 0;
        }

      dest_x = (src_x + offset_x) % gimp_item_width (item);
      if (dest_x < 0)
        dest_x = gimp_item_width (item) + dest_x;

      dest_y = (src_y + offset_y) % gimp_item_height (item);
      if (dest_y < 0)
        dest_y = gimp_item_height (item) + dest_y;

      /*  intersecting region  */
      if (offset_x != 0 && offset_y != 0)
        {
          pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                             src_x, src_y,
                             ABS (offset_x), ABS (offset_y),
                             FALSE);
          pixel_region_init (&destPR, new_tiles,
                             dest_x, dest_y,
                             ABS (offset_x), ABS (offset_y),
                             TRUE);
          copy_region (&srcPR, &destPR);
        }

      /*  X offset  */
      if (offset_x != 0)
        {
          if (offset_y >= 0)
            {
              pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                                 src_x, 0, ABS (offset_x),
                                 gimp_item_height (item) - ABS (offset_y),
                                 FALSE);
              pixel_region_init (&destPR, new_tiles,
                                 dest_x, dest_y + offset_y,
                                 ABS (offset_x),
                                 gimp_item_height (item) - ABS (offset_y),
                                 TRUE);
            }
          else if (offset_y < 0)
            {
              pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                                 src_x, src_y - offset_y,
                                 ABS (offset_x),
                                 gimp_item_height (item) - ABS (offset_y),
                                 FALSE);
              pixel_region_init (&destPR, new_tiles,
                                 dest_x, 0,
                                 ABS (offset_x),
                                 gimp_item_height (item) - ABS (offset_y),
                                 TRUE);
            }

          copy_region (&srcPR, &destPR);
        }

      /*  X offset  */
      if (offset_y != 0)
        {
          if (offset_x >= 0)
            {
              pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                                 0, src_y,
                                 gimp_item_width (item) - ABS (offset_x),
                                 ABS (offset_y), FALSE);
              pixel_region_init (&destPR, new_tiles, dest_x + offset_x, dest_y,
                                 gimp_item_width (item) - ABS (offset_x),
                                 ABS (offset_y), TRUE);
            }
          else if (offset_x < 0)
            {
              pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                                 src_x - offset_x, src_y,
                                 gimp_item_width (item) - ABS (offset_x),
                                 ABS (offset_y), FALSE);
              pixel_region_init (&destPR, new_tiles, 0, dest_y,
                                 gimp_item_width (item) - ABS (offset_x),
                                 ABS (offset_y), TRUE);
            }

          copy_region (&srcPR, &destPR);
        }
    }
  /*  Otherwise, fill the vacated regions  */
  else
    {
      if (fill_type == GIMP_OFFSET_BACKGROUND)
        {
          GimpRGB color;

          gimp_context_get_background (context, &color);

          gimp_rgb_get_uchar (&color, &fill[0], &fill[1], &fill[2]);

          if (gimp_drawable_has_alpha (drawable))
            fill[gimp_drawable_bytes (drawable) - 1] = OPAQUE_OPACITY;
        }

      if (offset_x >= 0 && offset_y >= 0)
        {
          dest_x = 0;
          dest_y = 0;
        }
      else if (offset_x >= 0 && offset_y < 0)
        {
          dest_x = 0;
          dest_y = gimp_item_height (item) + offset_y;
        }
      else if (offset_x < 0 && offset_y >= 0)
        {
          dest_x = gimp_item_width (item) + offset_x;
          dest_y = 0;
        }
      else if (offset_x < 0 && offset_y < 0)
        {
          dest_x = gimp_item_width (item) + offset_x;
          dest_y = gimp_item_height (item) + offset_y;
        }

      /*  intersecting region  */
      if (offset_x != 0 && offset_y != 0)
        {
          pixel_region_init (&destPR, new_tiles, dest_x, dest_y,
                             ABS (offset_x), ABS (offset_y), TRUE);
          color_region (&destPR, fill);
        }

      /*  X offset  */
      if (offset_x != 0)
        {
          if (offset_y >= 0)
            pixel_region_init (&destPR, new_tiles,
                               dest_x, dest_y + offset_y,
                               ABS (offset_x),
                               gimp_item_height (item) - ABS (offset_y),
                               TRUE);
          else if (offset_y < 0)
            pixel_region_init (&destPR, new_tiles,
                               dest_x, 0,
                               ABS (offset_x),
                               gimp_item_height (item) - ABS (offset_y),
                               TRUE);

          color_region (&destPR, fill);
        }

      /*  X offset  */
      if (offset_y != 0)
        {
          if (offset_x >= 0)
            pixel_region_init (&destPR, new_tiles,
                               dest_x + offset_x,
                               dest_y,
                               gimp_item_width (item) - ABS (offset_x),
                               ABS (offset_y),
                               TRUE);
          else if (offset_x < 0)
            pixel_region_init (&destPR, new_tiles,
                               0, dest_y,
                               gimp_item_width (item) - ABS (offset_x),
                               ABS (offset_y),
                               TRUE);

          color_region (&destPR, fill);
        }
    }

  gimp_drawable_set_tiles (drawable, gimp_item_is_attached (item),
                           _("Offset Drawable"), new_tiles);
  tile_manager_unref (new_tiles);
}
