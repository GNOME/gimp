#ifndef __TILE_H__
#define __TILE_H__


#define TILE_WIDTH   64
#define TILE_HEIGHT  64
#define TILE_DEBUG


#include <sys/types.h>
#include <glib.h>

#include "config.h"

#ifdef USE_PTHREADS
#include <pthread.h>
#endif


typedef struct _Tile Tile;

struct _Tile
{
  short ref_count;    /* reference count. when the reference count is
 		       *  non-zero then the "data" for this tile must
		       *  be valid. when the reference count for a tile
		       *  is 0 then the "data" for this tile must be
		       *  NULL.
		       */
  guint dirty : 1;    /* is the tile dirty? has it been modified? */
  guint valid : 1;    /* is the tile valid? */

  int ewidth;         /* the effective width of the tile */
  int eheight;        /* the effective height of the tile */
                      /* a tiles effective width and height may be smaller
		       *  (but not larger) than TILE_WIDTH and TILE_HEIGHT.
		       * this is to handle edge tiles of a drawable.
		       */
  int bpp;            /* the bytes per pixel (1, 2, 3 or 4) */
  int tile_num;       /* the number of this tile within the drawable */

  guchar *data;       /* the data for the tile. this may be NULL in which
		       *  case the tile data is on disk.
		       */

  int swap_num;       /* the index into the file table of the file to be used
		       *  for swapping. swap_num 1 is always the global swap file.
		       */
  off_t swap_offset;  /* the offset within the swap file of the tile data.
		       * if the tile data is in memory this will be set to -1.
		       */

  void *tm;           /* A pointer to the tile manager for this tile.
		       *  We need this in order to call the tile managers validate
		       *  proc whenever the tile is referenced yet invalid.
		       */
  Tile *next;
  Tile *prev;	      /* List pointers for the tile cache lists */
  void *listhead;     /* Pointer to the head of the list this tile is on */
#ifdef USE_PTHREADS
  pthread_mutex_t mutex;
#endif
};


/* Initializes the fields of a tile to "good" values.
 */
void tile_init (Tile *tile,
		int   bpp);

/* Referencing a tile causes the reference count to be incremented.
 *  If the reference count was previously 0 the tile will will be
 *  swapped into memory from disk.
 */

#if defined (TILE_DEBUG) && defined (__GNUC__)

#define tile_ref(t) _tile_ref (t, __PRETTY_FUNCTION__)
void _tile_ref (Tile *tile, char *func_name);

#else

void tile_ref (Tile *tile);

#endif


/* Unrefercing a tile causes the reference count to be decremented.
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
