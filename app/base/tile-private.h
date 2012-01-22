/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TILE_PRIVATE_H__
#define __TILE_PRIVATE_H__


/*  Uncomment to enable global counters to profile the tile system. */
/*  #define TILE_PROFILING */


typedef struct _TileLink TileLink;

struct _TileLink
{
  TileLink    *next;
  gint         tile_num; /* the number of this tile within the drawable */
  TileManager *tm;       /* A pointer to the tile manager for this tile.
                          *  We need this in order to call the tile managers
                          *  validate proc whenever the tile is referenced
                          *  yet invalid.
                          */
};


struct _Tile
{
  gshort  ref_count;    /* reference count. when the reference count is
                          *  non-zero then the "data" for this tile must
                         *  be valid. when the reference count for a tile
                         *  is 0 then the "data" for this tile must be
                         *  NULL.
                         */
  gshort  write_count;  /* write count: number of references that are
                           for write access */
  guint   share_count;  /* share count: number of tile managers that
                           hold this tile */
  guint   dirty : 1;    /* is the tile dirty? has it been modified? */
  guint   valid : 1;    /* is the tile valid? */
  guint  cached : 1;    /* is the tile cached */

#ifdef TILE_PROFILING

  guint zorched : 1;    /* was the tile flushed due to cache pressure
			   [zorched]? */
  guint zorchout: 1;    /* was the tile swapped out due to cache
			   pressure [zorched]? */
  guint  inonce : 1;    /* has the tile been swapped in at least once? */
  guint outonce : 1;    /* has the tile been swapped out at least once? */

#endif

  guchar  bpp;          /* the bytes per pixel (1, 2, 3 or 4) */
  gushort ewidth;       /* the effective width of the tile */
  gushort eheight;      /* the effective height of the tile
                         *  a tile's effective width and height may be smaller
                         *  (but not larger) than TILE_WIDTH and TILE_HEIGHT.
                         *  this is to handle edge tiles of a drawable.
                         */
  gint    size;         /* size of the tile data (ewidth * eheight * bpp) */

  TileRowHint *rowhint; /* An array of hints for rendering purposes */

  guchar *data;         /* the data for the tile. this may be NULL in which
                         *  case the tile data is on disk.
                         */

  gint64  swap_offset;  /* the offset within the swap file of the tile data.
                         * if the tile data is in memory this will be set
                         * to -1.
                         */

  TileLink *tlink;

  Tile     *next;       /* List pointers for the tile cache lists */
  Tile     *prev;
};


/*  tile_data_pointer() as a macro so that it can be inlined
 *
 *  Note that (y) & (TILE_HEIGHT-1) is equivalent to (y) % TILE_HEIGHT
 *  for positive power-of-two divisors
 */
#define TILE_DATA_POINTER(tile,x,y) \
  ((tile)->data + \
   (((y) & (TILE_HEIGHT-1)) * \
    (tile)->ewidth + ((x) & (TILE_WIDTH-1))) * (tile)->bpp)


#endif /* __TILE_PRIVATE_H__ */
