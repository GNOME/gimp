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

#ifndef __TILE_H__
#define __TILE_H__


#define TILE_WIDTH   64
#define TILE_HEIGHT  64


/* Returns a newly allocated Tile with all fields initialized to "good" values.
 */
Tile        * tile_new               (gint      bpp);


/*
 * tile_lock locks a tile into memory.  This does what tile_ref used
 * to do.  It is the responsibility of the tile manager to take care
 * of the copy-on-write semantics.  Locks stack; a tile remains locked
 * in memory as long as it's been locked more times than it has been
 * released.  tile_release needs to know whether the release was for
 * write access.  (This is a hack, and should be handled better.)
 */

void          tile_lock              (Tile     *tile);
void          tile_release           (Tile     *tile,
                                      gboolean  dirty);

/* Allocate the data for the tile.
 */
void          tile_alloc             (Tile     *tile);

/* Return the size in bytes of the tiles data.
 */
gint          tile_size              (Tile     *tile);

gint          tile_ewidth            (Tile     *tile);
gint          tile_eheight           (Tile     *tile);
gint          tile_bpp               (Tile     *tile);

gboolean      tile_is_valid          (Tile     *tile);

void        * tile_data_pointer      (Tile     *tile,
                                      gint      xoff,
                                      gint      yoff);

gint          tile_global_refcount   (void);

/* tile_attach attaches a tile to a tile manager: this function
 * increments the tile's share count and inserts a tilelink into the
 * tile's link list.  tile_detach reverses the process.
 * If a tile's share count is zero after a tile_detach, the tile is
 * discarded.
 */

void          tile_attach            (Tile     *tile,
                                      void     *tm,
                                      gint      tile_num);
void          tile_detach            (Tile     *tile,
                                      void     *tm,
                                      gint      tile_num);


#endif /* __TILE_H__ */
