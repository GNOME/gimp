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
#ifndef __TILE_MANAGER_H__
#define __TILE_MANAGER_H__


#include "tile.h"


typedef struct _TileManager  TileManager;

typedef void (*TileValidateProc) (TileManager *tm,
				  Tile        *tile);


/* Creates a new tile manager with the specified
 *  width for the toplevel. The toplevel sizes is
 *  used to compute the number of levels and there
 *  size. Each level is 1/2 the width and height of
 *  the level above it.
 * The toplevel is level 0. The smallest level in the
 *  hierarchy is "nlevels - 1". That level will be smaller
 *  than TILE_WIDTH x TILE_HEIGHT
 */
TileManager* tile_manager_new (int toplevel_width,
			       int toplevel_height,
			       int bpp);

/* Destroy a tile manager and all the tiles it contains.
 */
void tile_manager_destroy (TileManager *tm);

/* Set the validate procedure for the tile manager.
 *  The validate procedure is called when an invalid tile
 *  is referenced. If the procedure is NULL, then the tile
 *  is set to valid and its memory is allocated, but
 *  not initialized.
 */
void tile_manager_set_validate_proc (TileManager      *tm,
				     TileValidateProc  proc);

/* Get a specified tile from a tile manager. 
 */
Tile* tile_manager_get_tile (TileManager *tm,
			     int          xpixel,
			     int          ypixel,
			     int          wantread,
			     int          wantwrite);

/* Get a specified tile from a tile manager.
 */
Tile* tile_manager_get (TileManager *tm,
			int          tile_num,
			int          wantread,
			int          wantwrite);

/* Request that (if possible) the tile at x,y be swapped
 * in.  This is only a hint to improve performance; no guarantees.
 * The tile may be swapped in or otherwise made more accessible
 * if it is convenient...
 */
void tile_manager_get_async (TileManager *tm,
			     int          xpixel,
			     int          ypixel);

void tile_manager_map_tile (TileManager *tm,
			    int          xpixel,
			    int          ypixel,
			    Tile        *srctile);

void tile_manager_map (TileManager *tm,
		       int          time_num,
		       Tile        *srctile);

/* Validate a tiles memory.
 */
void tile_manager_validate (TileManager *tm,
			    Tile        *tile);

void tile_invalidate (Tile **tile_ptr, TileManager *tm, int tile_num);
void tile_invalidate_tile (Tile **tile_ptr, TileManager *tm, 
			   int xpixel, int ypixel);

/* Given a toplevel tile, this procedure will invalidate
 *  (set the dirty bit) for this toplevel tile.
 */
void tile_manager_invalidate_tiles (TileManager *tm,
				    Tile        *toplevel_tile);

void tile_manager_set_user_data (TileManager *tm,
				 void        *user_data);

void *tile_manager_get_user_data (TileManager *tm);

int tile_manager_level_width  (TileManager *tm);
int tile_manager_level_height (TileManager *tm);
int tile_manager_level_bpp    (TileManager *tm);

void tile_manager_get_tile_coordinates (TileManager *tm, Tile *tile,
					int *x, int *y);

void tile_manager_map_over_tile (TileManager *tm, Tile *tile, Tile *srctile);


#endif /* __TILE_MANAGER_H__ */
