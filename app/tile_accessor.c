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

#include "tile_accessor.h"

#include "tile_manager.h"
#include "tile.h"
#include "tile_pvt.h"

gint tile_accessor_start (TileAccessor *acc,
			  TileManager *tm, 
			  int wantread, 
			  int wantwrite)
{
  if (acc->valid && (acc->tm != tm || acc->writable != wantwrite))
    {
      g_warning ("tile_accessor_start: already started");
      tile_accessor_finish (acc);
    }

  acc->tm = tm;
  acc->writable = wantwrite;
  acc->valid = TRUE;
  acc->pointer = NULL;

  return TRUE;
}

gint tile_accessor_finish (TileAccessor *acc)
{
  if (!acc->valid) 
    {
      g_warning ("tile_accessor_finish: not started");
      return TRUE;
    }

  if (acc->pointer) 
    {
      tile_release (acc->tile, acc->writable);
      acc->tile = NULL;
      acc->pointer = NULL;
    }

  acc->valid = FALSE;
  return TRUE;
}

gint tile_accessor_position_internal (TileAccessor *acc,
				      int x,
				      int y)
{
  if (!acc->valid) 
    return FALSE;

  if (acc->pointer &&
      !(acc->edge_l>=x && acc->edge_r<x && 
	acc->edge_t>=y && acc->edge_b<y)) 
    {					/* we have a tile, but it's
					   the wrong one */
      tile_release (acc->tile, acc->writable);
      acc->pointer = NULL;
      acc->tile = NULL;
    }
    
  if (!acc->pointer) 
    {
      acc->tile = tile_manager_get_tile (acc->tm,
					 acc->x = x,
					 acc->y = y,
					 TRUE,
					 acc->writable);
      if (!acc->tile) 
	{
	  acc->pointer = NULL;
	  return FALSE;
	}
      acc->width  = acc->tile->ewidth;
      acc->height = acc->tile->eheight;
      acc->bpp = acc->tile->bpp;
      acc->edge_l = acc->x - (acc->x % TILE_WIDTH);
      acc->edge_r = acc->edge_l + acc->width;
      acc->edge_t = acc->y - (acc->y % TILE_HEIGHT);
      acc->edge_b = acc->edge_t + acc->height;
      acc->rowspan = acc->bpp * acc->width;
      acc->colspan = acc->bpp;
      acc->size = acc->bpp * acc->width * acc->height;
      acc->data = acc->tile->data;

      g_assert (acc->rowspan != 0);
      g_assert (acc->colspan != 0);
      g_assert (acc->bpp != 0);
      g_assert (acc->size != 0);

    }

  acc->pointer = (acc->data + 
		  ((acc->y)-(acc->edge_t))*(acc->rowspan) + 
		  ((acc->x)-(acc->edge_l))*(acc->colspan));

  return TRUE;
}

gint tile_accessor_probe (TileAccessor *acc, 
			  int x, 
			  int y)
{
  Tile *tile;

  tile = tile_manager_get_tile (acc->tm, x, y, FALSE, FALSE);
  
  return tile_is_valid (tile);
}
