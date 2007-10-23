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

#ifndef __TILE_ROWHINTS_H__
#define __TILE_ROWHINTS_H__


/*
 * Explicit guchar type rather than enum since gcc chooses an int
 * representation but arrays of TileRowHints are quite space-critical
 * in GIMP.
 */
typedef guchar TileRowHint;

#define TILEROWHINT_UNKNOWN     0
#define TILEROWHINT_OPAQUE      1
#define TILEROWHINT_TRANSPARENT 2
#define TILEROWHINT_MIXED       3
#define TILEROWHINT_OUTOFRANGE  4
#define TILEROWHINT_UNDEFINED   5
#define TILEROWHINT_BROKEN      6

TileRowHint   tile_get_rowhint       (Tile        *tile,
                                      gint         yoff);
void          tile_set_rowhint       (Tile        *tile,
                                      gint         yoff,
                                      TileRowHint  rowhint);
void          tile_allocate_rowhints (Tile        *tile);
void          tile_update_rowhints   (Tile        *tile,
                                      gint         start,
                                      gint         rows);


#endif /* __TILE_ROWHINTS_H__ */
