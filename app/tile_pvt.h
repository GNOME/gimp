#ifndef __TILE_PVT_H__
#define __TILE_PVT_H__

#ifdef USE_PTHREADS
#include <pthread.h>
#endif


#include <sys/types.h>
#include <glib.h>

#include "config.h"
#include "tile.h"

typedef struct _TileLink TileLink;

struct _TileLink
{
  TileLink *next;
  int tile_num;       /* the number of this tile within the drawable */
  void *tm;           /* A pointer to the tile manager for this tile.
		       *  We need this in order to call the tile managers 
		       *  validate proc whenever the tile is referenced yet 
		       *  invalid.
		       */
};

struct _Tile
{
  short ref_count;    /* reference count. when the reference count is
 		       *  non-zero then the "data" for this tile must
		       *  be valid. when the reference count for a tile
		       *  is 0 then the "data" for this tile must be
		       *  NULL.
		       */
  short write_count;  /* write count: number of references that are
			 for write access */ 
  short share_count;  /* share count: number of tile managers that
			 hold this tile */
  guint dirty : 1;    /* is the tile dirty? has it been modified? */
  guint valid : 1;    /* is the tile valid? */

  /* An array of hints for rendering purposes */
  TileRowHint rowhint[TILE_HEIGHT];

  guchar *data;       /* the data for the tile. this may be NULL in which
		       *  case the tile data is on disk.
		       */

  int ewidth;         /* the effective width of the tile */
  int eheight;        /* the effective height of the tile */
                      /*  a tile's effective width and height may be smaller
		       *  (but not larger) than TILE_WIDTH and TILE_HEIGHT.
		       *  this is to handle edge tiles of a drawable.
		       */
  int bpp;            /* the bytes per pixel (1, 2, 3 or 4) */
  int swap_num;       /* the index into the file table of the file to be used
		       *  for swapping. swap_num 1 is always the global swap file.
		       */
  off_t swap_offset;  /* the offset within the swap file of the tile data.
		       *  if the tile data is in memory this will be set to -1.
		       */
  TileLink *tlink;

  Tile *next;
  Tile *prev;	      /* List pointers for the tile cache lists */
  void *listhead;     /* Pointer to the head of the list this tile is on */
#ifdef USE_PTHREADS
  pthread_mutex_t mutex;
#endif
};

#ifdef USE_PTHREADS
#define TILE_MUTEX_LOCK(tile) pthread_mutex_lock(&((tile)->mutex))
#define TILE_MUTEX_UNLOCK(tile) pthread_mutex_unlock(&((tile)->mutex))
#else
#define TILE_MUTEX_LOCK(tile) /* nothing */
#define TILE_MUTEX_UNLOCK(tile) /* nothing */
#endif


#endif /* __TILE_PVT_H__ */
