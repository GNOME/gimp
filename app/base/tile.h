#ifndef __TILE_H__
#define __TILE_H__


/* We make one big assumptions about the tilesize at several places
   in the code: All tiles are squares!				    */

#define TILE_WIDTH   64
#define TILE_HEIGHT  64
#define TILE_SHIFT   6  /* This has to be the lg(TILE_WIDTH) to the base of 2 */

/* Uncomment for verbose debugging on copy-on-write logic
#define TILE_DEBUG
*/

/* sanity checking on new tile hinting code */
/* #define HINTS_SANITY */

#include <sys/types.h>
#include <glib.h>

#include "config.h"

typedef struct _Tile Tile;

/*  explicit guchar type rather than enum since gcc chooses an int
 *  representation but arrays of TileRowHints are quite space-critical
 *  in GIMP.
 */
typedef guchar TileRowHint;
#define TILEROWHINT_BROKEN      0
#define TILEROWHINT_OPAQUE      1
#define TILEROWHINT_TRANSPARENT 2
#define TILEROWHINT_MIXED       3
#define TILEROWHINT_OUTOFRANGE  4
#define TILEROWHINT_UNDEFINED   5
#define TILEROWHINT_UNKNOWN     6


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

int tile_ewidth (Tile *tile);
int tile_eheight (Tile *tile);
int tile_bpp (Tile *tile);

int tile_is_valid (Tile *tile);

void tile_mark_valid (Tile *tile);

/* DOCUMENT ME -- adm */
TileRowHint tile_get_rowhint (Tile *tile, int yoff);
void tile_set_rowhint (Tile *tile, int yoff, TileRowHint rowhint);
void tile_sanitize_rowhints (Tile *tile);

void *tile_data_pointer (Tile *tile, int xoff, int yoff);

/* tile_attach attaches a tile to a tile manager: this function
 * increments the tile's share count and inserts a tilelink into the
 * tile's link list.  tile_detach reverses the process.
 * If a tile's share count is zero after a tile_detach, the tile is
 * discarded.
 */

void tile_attach (Tile *tile, void *tm, int tile_num);
void tile_detach (Tile *tile, void *tm, int tile_num);


#endif /* __TILE_H__ */
