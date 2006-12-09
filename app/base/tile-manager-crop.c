/* The GIMP -- an image manipulation program
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

#include "paint-funcs/paint-funcs.h"

#include "pixel-region.h"
#include "tile-manager.h"
#include "tile-manager-crop.h"


/*  Crop the buffer to the size of pixels with non-zero transparency */

TileManager *
tile_manager_crop (TileManager *tiles,
                   gint         border)
{
  PixelRegion  PR;
  TileManager *new_tiles;
  gint         bytes, alpha;
  guchar      *data;
  gint         empty;
  gint         x1, y1, x2, y2;
  gint         x, y;
  gint         ex, ey;
  gint         found;
  void        *pr;
  guchar       black[MAX_CHANNELS] = { 0, 0, 0, 0 };

  g_return_val_if_fail (tiles != NULL, NULL);

  bytes = tile_manager_bpp (tiles);
  alpha = bytes - 1;

  /*  go through and calculate the bounds  */
  x1 = tile_manager_width (tiles);
  y1 = tile_manager_height (tiles);
  x2 = 0;
  y2 = 0;

  pixel_region_init (&PR, tiles, 0, 0, x1, y1, FALSE);

  for (pr = pixel_regions_register (1, &PR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      data = PR.data + alpha;
      ex = PR.x + PR.w;
      ey = PR.y + PR.h;

      for (y = PR.y; y < ey; y++)
        {
          found = FALSE;

          for (x = PR.x; x < ex; x++, data += bytes)
            if (*data)
              {
                if (x < x1)
                  x1 = x;
                if (x > x2)
                  x2 = x;

                found = TRUE;
              }

          if (found)
            {
              if (y < y1)
                y1 = y;
              if (y > y2)
                y2 = y;
            }
        }
    }

  x2 = CLAMP (x2 + 1, 0, tile_manager_width (tiles));
  y2 = CLAMP (y2 + 1, 0, tile_manager_height (tiles));

  empty = (x1 == tile_manager_width (tiles) &&
           y1 == tile_manager_height (tiles));

  if (empty)
    {
      /*  If there are no visible pixels, return NULL */

      new_tiles = NULL;
    }
  else if (x1 == 0 && y1 == 0 &&
           x2 == tile_manager_width (tiles)  &&
           y2 == tile_manager_height (tiles) &&
           border == 0)
    {
      /*  If no cropping, return original buffer  */

      new_tiles = tiles;
    }
  else
    {
      /*  Otherwise, crop the original area  */

      PixelRegion srcPR, destPR;
      gint        new_width, new_height;

      new_width  = (x2 - x1) + border * 2;
      new_height = (y2 - y1) + border * 2;
      new_tiles  = tile_manager_new (new_width, new_height, bytes);

      /*  If there is a border, make sure to clear the new tiles first  */
      if (border)
        {
          pixel_region_init (&destPR, new_tiles,
                             0, 0, new_width, border,
                             TRUE);
          color_region (&destPR, black);

          pixel_region_init (&destPR, new_tiles,
                             0, border, border, (y2 - y1),
                             TRUE);
          color_region (&destPR, black);

          pixel_region_init (&destPR, new_tiles,
                             new_width - border, border, border, (y2 - y1),
                             TRUE);
          color_region (&destPR, black);

          pixel_region_init (&destPR, new_tiles,
                             0, new_height - border, new_width, border,
                             TRUE);
          color_region (&destPR, black);
        }

      pixel_region_init (&srcPR, tiles,
                         x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, new_tiles,
                         border, border, (x2 - x1), (y2 - y1), TRUE);

      copy_region (&srcPR, &destPR);

      tile_manager_set_offsets (new_tiles, x1, y1);
    }

  return new_tiles;
}
