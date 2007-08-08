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

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager-preview.h"

#include "paint-funcs/subsample-region.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpdrawable-preview.h"
#include "gimplayer.h"
#include "gimppreviewcache.h"


/*  local function prototypes  */

static TempBuf * gimp_drawable_preview_private (GimpDrawable *drawable,
                                                gint          width,
                                                gint          height);
static TempBuf * gimp_drawable_indexed_preview (GimpDrawable *drawable,
                                                const guchar *cmap,
                                                gint          src_x,
                                                gint          src_y,
                                                gint          src_width,
                                                gint          src_height,
                                                gint          dest_width,
                                                gint          dest_height);


/*  public functions  */

TempBuf *
gimp_drawable_get_preview (GimpViewable *viewable,
                           GimpContext  *context,
                           gint          width,
                           gint          height)
{
  GimpDrawable *drawable;
  GimpImage    *image;

  drawable = GIMP_DRAWABLE (viewable);
  image   = gimp_item_get_image (GIMP_ITEM (drawable));

  if (! image->gimp->config->layer_previews)
    return NULL;

  /* Ok prime the cache with a large preview if the cache is invalid */
  if (! drawable->preview_valid                  &&
      width  <= PREVIEW_CACHE_PRIME_WIDTH        &&
      height <= PREVIEW_CACHE_PRIME_HEIGHT       &&
      image                                     &&
      image->width  > PREVIEW_CACHE_PRIME_WIDTH &&
      image->height > PREVIEW_CACHE_PRIME_HEIGHT)
    {
      TempBuf *tb = gimp_drawable_preview_private (drawable,
                                                   PREVIEW_CACHE_PRIME_WIDTH,
                                                   PREVIEW_CACHE_PRIME_HEIGHT);

      /* Save the 2nd call */
      if (width  == PREVIEW_CACHE_PRIME_WIDTH &&
          height == PREVIEW_CACHE_PRIME_HEIGHT)
        return tb;
    }

  /* Second call - should NOT visit the tile cache...*/
  return gimp_drawable_preview_private (drawable, width, height);
}

gint
gimp_drawable_preview_bytes (GimpDrawable *drawable)
{
  GimpImageBaseType base_type;
  gint              bytes = 0;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), 0);

  base_type = GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable));

  switch (base_type)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      bytes = gimp_drawable_bytes (drawable);
      break;

    case GIMP_INDEXED:
      bytes = gimp_drawable_has_alpha (drawable) ? 4 : 3;
      break;
    }

  return bytes;
}

TempBuf *
gimp_drawable_get_sub_preview (GimpDrawable *drawable,
                               gint          src_x,
                               gint          src_y,
                               gint          src_width,
                               gint          src_height,
                               gint          dest_width,
                               gint          dest_height)
{
  GimpItem    *item;
  GimpImage   *image;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);
  g_return_val_if_fail (src_width  > 0, NULL);
  g_return_val_if_fail (src_height > 0, NULL);
  g_return_val_if_fail (dest_width  > 0, NULL);
  g_return_val_if_fail (dest_height > 0, NULL);

  item = GIMP_ITEM (drawable);

  g_return_val_if_fail ((src_x + src_width)  <= gimp_item_width  (item), NULL);
  g_return_val_if_fail ((src_y + src_height) <= gimp_item_height (item), NULL);

  image = gimp_item_get_image (item);

  if (! image->gimp->config->layer_previews)
    return NULL;

  if (GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable)) == GIMP_INDEXED)
    return gimp_drawable_indexed_preview (drawable,
                                          gimp_image_get_colormap (image),
                                          src_x, src_y, src_width, src_height,
                                          dest_width, dest_height);

  return tile_manager_get_sub_preview (gimp_drawable_get_tiles (drawable),
                                       src_x, src_y, src_width, src_height,
                                       dest_width, dest_height);
}


/*  private functions  */

static TempBuf *
gimp_drawable_preview_private (GimpDrawable *drawable,
                               gint          width,
                               gint          height)
{
  TempBuf *ret_buf;

  if (! drawable->preview_valid ||
      ! (ret_buf = gimp_preview_cache_get (&drawable->preview_cache,
                                           width, height)))
    {
      GimpItem *item = GIMP_ITEM (drawable);

      ret_buf = gimp_drawable_get_sub_preview (drawable,
                                               0, 0,
                                               gimp_item_width (item),
                                               gimp_item_height (item),
                                               width,
                                               height);

      if (! drawable->preview_valid)
        gimp_preview_cache_invalidate (&drawable->preview_cache);

      drawable->preview_valid = TRUE;

      gimp_preview_cache_add (&drawable->preview_cache, ret_buf);
    }

  return ret_buf;
}

static TempBuf *
gimp_drawable_indexed_preview (GimpDrawable *drawable,
                               const guchar *cmap,
                               gint          src_x,
                               gint          src_y,
                               gint          src_width,
                               gint          src_height,
                               gint          dest_width,
                               gint          dest_height)
{
  TempBuf     *preview_buf;
  PixelRegion  srcPR;
  PixelRegion  destPR;
  gint         bytes     = gimp_drawable_preview_bytes (drawable);
  gint         subsample = 1;

  /*  calculate 'acceptable' subsample  */
  while ((dest_width  * (subsample + 1) * 2 < src_width) &&
         (dest_height * (subsample + 1) * 2 < src_width))
    subsample += 1;

  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     src_x, src_y, src_width, src_height,
                     FALSE);

  preview_buf = temp_buf_new (dest_width, dest_height, bytes, 0, 0, NULL);

  pixel_region_init_temp_buf (&destPR, preview_buf,
                              0, 0, dest_width, dest_height);

  subsample_indexed_region (&srcPR, &destPR, cmap, subsample);

  return preview_buf;
}
