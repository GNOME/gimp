/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997-2004 Adam D. Moss <adam@gimp.org>
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

#include <cairo.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "gimpdrawable.h"
#include "gimpdrawable-convert.h"
#include "gimpimage.h"


void
gimp_drawable_convert_rgb (GimpDrawable *drawable,
                           gboolean      push_undo)
{
  GimpImageType  type;
  TileManager   *tiles;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (! gimp_drawable_is_rgb (drawable));

  type = GIMP_RGB_IMAGE;

  if (gimp_drawable_has_alpha (drawable))
    type = GIMP_IMAGE_TYPE_WITH_ALPHA (type);

  tiles = tile_manager_new (gimp_item_get_width  (GIMP_ITEM (drawable)),
                            gimp_item_get_height (GIMP_ITEM (drawable)),
                            GIMP_IMAGE_TYPE_BYTES (type));

  gimp_drawable_convert_tiles_rgb (drawable, tiles);

  gimp_drawable_set_tiles (drawable, push_undo, NULL,
                           tiles, type);
  tile_manager_unref (tiles);
}

void
gimp_drawable_convert_grayscale (GimpDrawable *drawable,
                                 gboolean      push_undo)
{
  GimpImageType  type;
  TileManager   *tiles;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (! gimp_drawable_is_gray (drawable));

  type = GIMP_GRAY_IMAGE;

  if (gimp_drawable_has_alpha (drawable))
    type = GIMP_IMAGE_TYPE_WITH_ALPHA (type);

  tiles = tile_manager_new (gimp_item_get_width  (GIMP_ITEM (drawable)),
                            gimp_item_get_height (GIMP_ITEM (drawable)),
                            GIMP_IMAGE_TYPE_BYTES (type));

  gimp_drawable_convert_tiles_grayscale (drawable, tiles);

  gimp_drawable_set_tiles (drawable, push_undo, NULL,
                           tiles, type);
  tile_manager_unref (tiles);
}

void
gimp_drawable_convert_tiles_rgb (GimpDrawable *drawable,
                                 TileManager  *new_tiles)
{
  PixelRegion   srcPR, destPR;
  gint          row, col;
  gint          offset;
  GimpImageType type;
  gboolean      has_alpha;
  const guchar *src, *s;
  guchar       *dest, *d;
  const guchar *cmap;
  gpointer      pr;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (new_tiles != NULL);

  type      = gimp_drawable_type (drawable);
  has_alpha = gimp_drawable_has_alpha (drawable);

  g_return_if_fail (tile_manager_bpp (new_tiles) == (has_alpha ? 4 : 3));

  cmap = gimp_drawable_get_colormap (drawable);

  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     0, 0,
                     gimp_item_get_width  (GIMP_ITEM (drawable)),
                     gimp_item_get_height (GIMP_ITEM (drawable)),
                     FALSE);
  pixel_region_init (&destPR, new_tiles,
                     0, 0,
                     gimp_item_get_width  (GIMP_ITEM (drawable)),
                     gimp_item_get_height (GIMP_ITEM (drawable)),
                     TRUE);

  switch (GIMP_IMAGE_TYPE_BASE_TYPE (type))
    {
    case GIMP_GRAY:
      for (pr = pixel_regions_register (2, &srcPR, &destPR);
           pr != NULL;
           pr = pixel_regions_process (pr))
        {
          src  = srcPR.data;
          dest = destPR.data;

          for (row = 0; row < srcPR.h; row++)
            {
              s = src;
              d = dest;

              for (col = 0; col < srcPR.w; col++)
                {
                  d[RED] = *s;
                  d[GREEN] = *s;
                  d[BLUE] = *s;

                  d += 3;
                  s++;
                  if (has_alpha)
                    *d++ = *s++;
                }

              src += srcPR.rowstride;
              dest += destPR.rowstride;
            }
        }
      break;

    case GIMP_INDEXED:
      for (pr = pixel_regions_register (2, &srcPR, &destPR);
           pr != NULL;
           pr = pixel_regions_process (pr))
        {
          src  = srcPR.data;
          dest = destPR.data;

          for (row = 0; row < srcPR.h; row++)
            {
              s = src;
              d = dest;

              for (col = 0; col < srcPR.w; col++)
                {
                  offset = *s++ * 3;
                  d[RED] = cmap[offset + 0];
                  d[GREEN] = cmap[offset + 1];
                  d[BLUE] = cmap[offset + 2];

                  d += 3;
                  if (has_alpha)
                    *d++ = *s++;
                }

              src += srcPR.rowstride;
              dest += destPR.rowstride;
            }
        }
      break;

    default:
      break;
    }
}

void
gimp_drawable_convert_tiles_grayscale (GimpDrawable *drawable,
                                       TileManager  *new_tiles)
{
  PixelRegion   srcPR, destPR;
  gint          row, col;
  gint          offset, val;
  GimpImageType type;
  gboolean      has_alpha;
  const guchar *src, *s;
  guchar       *dest, *d;
  const guchar *cmap;
  gpointer      pr;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (new_tiles != NULL);

  type      = gimp_drawable_type (drawable);
  has_alpha = gimp_drawable_has_alpha (drawable);

  g_return_if_fail (tile_manager_bpp (new_tiles) == (has_alpha ? 2 : 1));

  cmap = gimp_drawable_get_colormap (drawable);

  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     0, 0,
                     gimp_item_get_width  (GIMP_ITEM (drawable)),
                     gimp_item_get_height (GIMP_ITEM (drawable)),
                     FALSE);
  pixel_region_init (&destPR, new_tiles,
                     0, 0,
                     gimp_item_get_width  (GIMP_ITEM (drawable)),
                     gimp_item_get_height (GIMP_ITEM (drawable)),
                     TRUE);

  switch (GIMP_IMAGE_TYPE_BASE_TYPE (type))
    {
    case GIMP_RGB:
      for (pr = pixel_regions_register (2, &srcPR, &destPR);
           pr != NULL;
           pr = pixel_regions_process (pr))
        {
          src = srcPR.data;
          dest = destPR.data;

          for (row = 0; row < srcPR.h; row++)
            {
              s = src;
              d = dest;
              for (col = 0; col < srcPR.w; col++)
                {
                  val = GIMP_RGB_LUMINANCE (s[RED],
                                            s[GREEN],
                                            s[BLUE]) + 0.5;
                  *d++ = (guchar) val;
                  s += 3;
                  if (has_alpha)
                    *d++ = *s++;
                }

              src += srcPR.rowstride;
              dest += destPR.rowstride;
            }
        }
      break;

    case GIMP_INDEXED:
      for (pr = pixel_regions_register (2, &srcPR, &destPR);
           pr != NULL;
           pr = pixel_regions_process (pr))
        {
          src = srcPR.data;
          dest = destPR.data;

          for (row = 0; row < srcPR.h; row++)
            {
              s = src;
              d = dest;

              for (col = 0; col < srcPR.w; col++)
                {
                  offset = *s++ * 3;
                  val = GIMP_RGB_LUMINANCE (cmap[offset+0],
                                            cmap[offset+1],
                                            cmap[offset+2]) + 0.5;
                  *d++ = (guchar) val;
                  if (has_alpha)
                    *d++ = *s++;
                }

              src += srcPR.rowstride;
              dest += destPR.rowstride;
            }
        }
      break;

    default:
      break;
    }
}
