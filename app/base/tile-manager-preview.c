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

#include "base-types.h"

#include "paint-funcs/sample-funcs.h"

#include "pixel-region.h"
#include "temp-buf.h"
#include "tile-manager.h"
#include "tile-manager-preview.h"


static TempBuf * tile_manager_create_preview (TileManager *tiles,
                                              gint         src_x,
                                              gint         src_y,
                                              gint         src_width,
                                              gint         src_height,
                                              gint         dest_width,
                                              gint         dest_height);


TempBuf *
tile_manager_get_preview (TileManager *tiles,
                          gint         width,
                          gint         height)
{
  g_return_val_if_fail (tiles != NULL, NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  return tile_manager_create_preview (tiles,
                                      0, 0,
                                      tile_manager_width (tiles),
                                      tile_manager_height (tiles),
                                      width, height);
}

TempBuf *
tile_manager_get_sub_preview (TileManager *tiles,
                              gint         src_x,
                              gint         src_y,
                              gint         src_width,
                              gint         src_height,
                              gint         dest_width,
                              gint         dest_height)
{
  g_return_val_if_fail (tiles != NULL, NULL);

  g_return_val_if_fail (src_x >= 0 &&
                        src_x < tile_manager_width (tiles), NULL);
  g_return_val_if_fail (src_y >= 0 &&
                        src_y < tile_manager_height (tiles), NULL);

  g_return_val_if_fail (src_width > 0 &&
                        src_x + src_width <= tile_manager_width (tiles), NULL);
  g_return_val_if_fail (src_height > 0 &&
                        src_y + src_height <= tile_manager_height (tiles),
                        NULL);

  g_return_val_if_fail (dest_width > 0 && dest_height > 0, NULL);

  return tile_manager_create_preview (tiles,
                                      src_x, src_y, src_width, src_height,
                                      dest_width, dest_height);
}

static TempBuf *
tile_manager_create_preview (TileManager *tiles,
                             gint         src_x,
                             gint         src_y,
                             gint         src_width,
                             gint         src_height,
                             gint         dest_width,
                             gint         dest_height)
{
  TempBuf     *preview;
  PixelRegion  srcPR;
  PixelRegion  destPR;
  gint         subsample = 1;

  preview = temp_buf_new (dest_width, dest_height,
                          tile_manager_bpp (tiles), 0, 0, NULL);

  pixel_region_init (&srcPR, tiles, src_x, src_y, src_width, src_height, FALSE);

  pixel_region_init_temp_buf (&destPR, preview, 0, 0, dest_width, dest_height);

  /*  calculate 'acceptable' subsample  */
  while ((dest_width  * (subsample + 1) * 2 < src_width) &&
         (dest_height * (subsample + 1) * 2 < src_height))
    subsample += 1;

  subsample_region (&srcPR, &destPR, subsample);

  return preview;
}
