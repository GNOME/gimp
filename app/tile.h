#ifndef __TILE_H__
#define __TILE_H__


#define TILE_WIDTH   64
#define TILE_HEIGHT  64

/* Uncomment for verbose debugging on copy-on-write logic
#define TILE_DEBUG
*/



#include <sys/types.h>
#include <glib.h>

#include "config.h"

typedef struct _Tile Tile;


/* Initializes the fields of a tile to "good" values.
 */
void tile_init (Tile *tile,
		int   bpp);

/*
 * c-o-w
 */
void tile_mirror (Tile *dest_tile, Tile *src_tile);


/* Referencing a tile causes the reference count to be incremented.
 *  If the reference count was previously 0 the tile will will be
 *  swapped into memory from disk.
 *
 * tile_ref2 is a new tile-referencing interface through which you
 *  should register your intent to dirty the tile.  This will facilitate
 *  copy-on-write tile semantics.
 */

#if defined (TILE_DEBUG) && defined (__GNUC__)

#define tile_ref(t) _tile_ref (t, __PRETTY_FUNCTION__)
void _tile_ref (Tile *tile, char *func_name);
#define tile_ref2(t,d) _tile_ref2 (t, d, __PRETTY_FUNCTION__)
void _tile_ref2 (Tile *tile, int dirty, char *func_name);

#else

void tile_ref (Tile *tile);
void tile_ref2 (Tile *tile, int dirty);

#endif


/* Unreferencing a tile causes the reference count to be decremented.
 *  When the reference count reaches 0 the tile data will be swapped
 *  out to disk. Note that the tile may be in the tile cache which
 *  also references the tile causing the reference count not to
 *  fall below 1 until the tile is removed from the cache.
 *  The dirty flag indicates whether the tile data has been dirtied
 *  while referenced.
 */

#if defined (TILE_DEBUG) && defined (__GNUC__)

#define tile_unref(t,d) _tile_unref (t, d, __PRETTY_FUNCTION__)
void _tile_unref (Tile *tile, int dirty, char *func_name);

#else

void tile_unref (Tile *tile, int dirty);

#endif

/* Allocate the data for the tile.
 */
void tile_alloc (Tile *tile);

/* Return the size in bytes of the tiles data.
 */
int tile_size (Tile *tile);

/* Invalidates a tile. This involves setting its invalid bit and
 *  a) if the tile is in memory deleting the tile data or b) if the
 *  tile is in the swap file, deleting the space it occupies. This
 *  will cause the tile data to need to be recalculated.
 * This should probably only be used on sublevel tiles. Invalidating
 *  a toplevel tile would mean that it needs to be recalculated. But
 *  what do you recalculate a toplevel tile from?
 */
void tile_invalidate (Tile *tile);


#endif /* __TILE_H__ */
