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


typedef struct _TileLevel    TileLevel;
typedef struct _TileManager  TileManager;

typedef void (*TileValidateProc) (TileManager *tm,
				  Tile        *tile,
				  int          level);


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

/* Calculate the number of levels necessary to have a complete
 *  hierarchy. This procedure is normally called twice with
 *  the width and then height and the maximum value returned
 *  is then used as the number of levels an image needs.
 */
int tile_manager_calc_levels (int size,
			      int tile_size);

/* Set the number of levels this tile manager is managing.
 *  This procedure may destroy unnecessary levels in the
 *  tile manager if the new number of levels is less than
 *  the old number of levels.
 * Any newly added levels will consist of invalid tiles.
 */
void tile_manager_set_nlevels (TileManager *tm,
			       int          nlevels);

/* Set the validate procedure for the tile manager.
 *  The validate procedure is called when an invalid tile
 *  is referenced. If the procedure is NULL, then the tile
 *  is set to valid and its memory is allocated, but
 *  not initialized.
 */
void tile_manager_set_validate_proc (TileManager      *tm,
				     TileValidateProc  proc);

/* Get a specified tile from a tile manager. The tile
 *  is from the given level and contains the specified
 *  pixel. Be aware that the pixel coordinates are
 *  dependent on the level.
 */
Tile* tile_manager_get_tile (TileManager *tm,
			     int          xpixel,
			     int          ypixel,
			     int          level,
			     int          wantread,
			     int          wantwrite);

/* Get a specified tile from a tile manager.
 */
Tile* tile_manager_get (TileManager *tm,
			int          tile_num,
			int          level,
			int          wantread,
			int          wantwrite);

void tile_manager_map_tile (TileManager *tm,
			    int          xpixel,
			    int          ypixel,
			    int          level,
			    Tile        *srctile);

void tile_manager_map (TileManager *tm,
		       int          time_num,
		       int          level,
		       Tile        *srctile);

/* Validate a tiles memory.
 */
void tile_manager_validate (TileManager *tm,
			    Tile        *tile);

/* Given a toplevel tile, this procedure will invalidate
 *  (set the dirty bit) for all tiles in lower levels which
 *  contain this toplevel tile.
 * Note: if a level hasn't been created then the tile for that
 *       level won't be invalidated.
 */
void tile_manager_invalidate_tiles (TileManager *tm,
				    Tile        *toplevel_tile);

/* Invalidates all the tiles in the sublevels.
 */
void tile_manager_invalidate_sublevels (TileManager *tm);

/* Update a portion lower level tile given a toplevel tile.
 */
void tile_manager_update_tile (TileManager *tm,
			       Tile        *toplevel_tile,
			       int          level);


#endif /* __TILE_MANAGER_H__ */
