#ifndef __TILE_PVT_H__
#define __TILE_PVT_H__

#ifdef USE_PTHREADS
#include <pthread.h>
#endif


#include <sys/types.h>
#include <glib.h>

#include "config.h"

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

  guchar *data;       /* the data for the tile. this may be NULL in which
		       *  case the tile data is on disk.
		       */

  Tile *real_tile_ptr;/* if this tile's 'data' pointer is just a copy-on-write
		       *  mirror of another's, this is that source tile.
		       *  (real_tile itself can actually be a virtual tile
		       *  too.)  This is NULL if this tile is not a virtual
		       *  tile.
		       */
  Tile *mirrored_by;  /* If another tile is mirroring this one, this is
		       *  a pointer to that tile, otherwise this is NULL.
		       *  Note that only one tile may be _directly_ mirroring
		       *  another given tile.  This ensures that the graph
		       *  of mirrorings is no more complex than a linked
		       *  list.
		       */

  int ewidth;         /* the effective width of the tile */
  int eheight;        /* the effective height of the tile */
                      /*  a tile's effective width and height may be smaller
		       *  (but not larger) than TILE_WIDTH and TILE_HEIGHT.
		       *  this is to handle edge tiles of a drawable.
		       */
  int bpp;            /* the bytes per pixel (1, 2, 3 or 4) */
  int tile_num;       /* the number of this tile within the drawable */

  int swap_num;       /* the index into the file table of the file to be used
		       *  for swapping. swap_num 1 is always the global swap file.
		       */
  off_t swap_offset;  /* the offset within the swap file of the tile data.
		       *  if the tile data is in memory this will be set to -1.
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

#endif /* __TILE_PVT_H__ */
