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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __TILE_MANAGER_PVT_H__
#define __TILE_MANAGER_PVT_H__

#include "tile.h"

struct _TileLevel
{
  int width;                       /* the width of the tiled area */
  int height;                      /* the height of the tiled area */
  int bpp;                         /* the bpp of each tile */

  int ntile_rows;                  /* the number of tiles in each row */
  int ntile_cols;                  /* the number of tiles in each columns */

  Tile *tiles;                     /* the tiles for this level */
};

struct _TileManager
{
  int x, y;                        /* tile manager offsets  */
  int nlevels;                     /* the number of tile levels in the hierarchy */
  TileLevel *levels;               /* the hierarchy */
  TileValidateProc validate_proc;  /* this proc is called when an attempt to get an
				    *  invalid tile is made.
				    */
  void *user_data;                 /* hook for hanging data off of */
};


#endif /* __TILE_MANAGER_PVT_H__ */
