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

#ifndef __TILE_ACCESSOR_H__
#define __TILE_ACCESSOR_H__

/* #define TILE_ACCESSOR_DEBUG */

#include "tile.h"
#include "tile_manager.h"

typedef struct _TileAccessor TileAccessor;

gint tile_accessor_start (TileAccessor *,
			  TileManager *, 
			  int wantread, 
			  int wantwrite);

gint tile_accessor_finish (TileAccessor *);

gint tile_accessor_position_internal (TileAccessor *,
				      int x,
				      int y);


/* tile_accessor_probe returns true if the tile mapped to x,y is valid */

gint tile_accessor_probe (TileAccessor *, 
			  int x, 
			  int y);	

struct _TileAccessor 
{
  short valid;				/* true if this accessor is valid */
  short writable;			/* true if accessor is write-enabled */
  short edge_l, edge_r, edge_t, edge_b;	/* left, right, top, bottom edges */
					/* right and bottom are +1 */
  short x, y;				/* x & y corresp. to pointer */
  short rowspan, colspan;		/* row & column span multipliers */
  short bpp;				/* bytes per pixel */
  short size;				/* tile size in bytes */
  short height, width;			/* width, height */
  void* pointer;			/* pointer to current data */
  void* data;				/* pointer to start of tile data */
  Tile* tile;
  TileManager* tm;
};

#ifdef TILE_ACCESSOR_DEBUG
#define tile_accessor_position(ta,x,y) tile_accessor_position_internal ((ta),(x),(y))
#else
#define tile_accessor_position(ta,_x,_y) \
(((ta)->valid && \
  (ta)->pointer && \
  (ta)->edge_l>=(_x) && (ta)->edge_r<(_x) && \
  (ta)->edge_t>=(_y) && (ta)->edge_b<(_y)) ? \
 (((ta)->pointer = ((ta)->data + \
		    ((((ta)->y)=(_y))-((ta)->edge_t))*((ta)->rowspan) + \
		    ((((ta)->x)=(_x))-((ta)->edge_l))*((ta)->colspan))), \
  1) : \
 tile_accessor_position_internal ((ta),(_x),(_y)))
#endif

#define TILE_ACCESSOR_INVALID { 0 }
     
#endif  /* __TILE_ACCESSOR_H__ */
