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
 * tile_lock locks a tile into memory.  This does what tile_ref used
 * to do.  It is the responsibility of the tile manager to take care
 * of the copy-on-write semantics.  Locks stack; a tile remains locked
 * in memory as long as it's been locked more times than it has been
 * released.  tile_release needs to know whether the release was for
 * write access.  (This is a hack, and should be handled better.)
 */

void tile_lock (Tile *);
void tile_release (Tile *, int);

/* Allocate the data for the tile.
 */
void tile_alloc (Tile *tile);

/* Return the size in bytes of the tiles data.
 */
int tile_size (Tile *tile);

/* tile_attach attaches a tile to a tile manager: this function
 * increments the tile's share count and inserts a tilelink into the
 * tile's link list.  tile_detach reverses the process.
 * If a tile's share count is zero after a tile_detach, the tile is
 * discarded.
 */

void tile_attach (Tile *tile, void *tm, int tile_num);
void tile_detach (Tile *tile, void *tm, int tile_num);


#endif /* __TILE_H__ */
