#include <stdio.h>

#include "tile.h"
#include "tile_pvt.h"
#include "tile_cache.h"
#include "tile_manager.h"
#include "tile_swap.h"


/* EXPERIMENTAL Copy-On-Write goodies
 *  by Adam D. Moss
 *   adam@gimp.org
 *   adam@foxbox.org
 *
 *
 * C.O.W. Revisions:
 *
 *   97.10.05 - Initial release
 *   97.10.06 - Much faster tile invalidation +
 *              Better swap interaction (should no longer
 *                crash GIMP when GIMP swapfile is full).
 *   97.10.18 - Very stable now, and even more efficient.
 *   98.06.16 - Revised from GIMP 0.99.14 for 1.[01].0 - no
 *                longer so sure about stability until
 *                more comprehensive testing is done.
 *
 *
 * MISC TODO:
 *
 *  tile_invalidate: (tile_manager) - don't let a tile become
 *   invalidated if its ref-count >1, but move it to a delete-on-last-unref
 *   list instead...
 */


static void tile_destroy (Tile *tile);

int tile_count = 0;

void
tile_init (Tile *tile,
	   int   bpp)
{
  tile->ref_count = 0;
  tile->write_count = 0;
  tile->share_count = 0;
  tile->dirty = FALSE;
  tile->valid = FALSE;
  tile->data = NULL;
  tile->ewidth = TILE_WIDTH;
  tile->eheight = TILE_HEIGHT;
  tile->bpp = bpp;
  tile->swap_num = 1;
  tile->swap_offset = -1;
  tile->tlink = NULL;
  tile->next = tile->prev = NULL;
  tile->listhead = NULL;
#ifdef USE_PTHREADS
  {
    pthread_mutex_init(&tile->mutex, NULL);
  }
  tile_count++;
#endif
}

int tile_ref_count = 0;
int tile_share_count = 0;
int tile_active_count = 0;

void
tile_lock (Tile *tile)
{
  /* Increment the global reference count.
   */
  tile_ref_count += 1;

  /* Increment this tile's reference count.
   */

  TILE_MUTEX_LOCK (tile);
  tile->ref_count += 1;

  if (tile->ref_count == 1) 
    {
      if (tile->listhead) 
	{
	  /* remove from cache, move to main store */
	  tile_cache_flush (tile);
	}
      if (tile->data == NULL)
	{
	  /* There is no data, so the tile must be swapped out */
	  tile_swap_in (tile);
	}
      tile_active_count ++;
    }
  TILE_MUTEX_UNLOCK (tile);

  /* Call 'tile_manager_validate' if the tile was invalid.
   */
  if (!tile->valid)
    {
      /* an invalid tile should never be shared, so this should work */
      tile_manager_validate ((TileManager*) tile->tlink->tm, tile);
    }
}


void
tile_release (Tile *tile, int dirty)
{
  /* Decrement the global reference count.
   */
  tile_ref_count -= 1;

  TILE_MUTEX_LOCK(tile);

  /* Decrement this tile's reference count.
   */
  tile->ref_count -= 1;

  /* Decrement write ref count if dirtying
   */
  if (dirty)
    tile->write_count -= 1;

  if (tile->ref_count == 0)
    {
      if (tile->share_count == 0)
	{
	  /* tile is dead */
	  tile_destroy (tile);
	  return;			/* skip terminal unlock */
	}
      else 
	{
	  /* last reference was just released, so move the tile to the
	     tile cache */
	  tile_cache_insert (tile);
	}
      tile_active_count--;
    }

  TILE_MUTEX_UNLOCK (tile);
}

void
tile_alloc (Tile *tile)
{
  if (tile->data)
    return;

  /* Allocate the data for the tile.
   */
  tile->data = g_new (guchar, tile_size (tile));
}

static void
tile_destroy (Tile *tile)
{
  if (tile->ref_count) 
    {
      g_warning ("tried to destroy a ref'd tile");
      return;
    }
  if (tile->share_count)
    {
      g_warning ("tried to destroy an attached tile");
      return;
    }
  if (tile->data) 
    {
      g_free (tile->data);
      tile->data = NULL;
    }
  if (tile->swap_offset != -1)
    {
      /* If the tile is on disk, then delete its
       *  presence there.
       */
      tile_swap_delete (tile);
    }
  if (tile->listhead)
    tile_cache_flush (tile);
  
  TILE_MUTEX_UNLOCK (tile); 
  g_free (tile);
  tile_count --;
}


int
tile_size (Tile *tile)
{
  int size;
  /* Return the actual size of the tile data.
   *  (Based on its effective width and height).
   */
  size = tile->ewidth * tile->eheight * tile->bpp;
  return size;
}


void
tile_attach (Tile *tile, void *tm, int tile_num)
{
  TileLink *tmp;

  if (tile->share_count > 0 && !tile->valid) 
    {
      /* trying to share invalid tiles is problematic, not to mention silly */
      tile_manager_validate ((TileManager*) tile->tlink->tm, tile);
    }
  tile->share_count++;
  tile_share_count++;
#ifdef TILE_DEBUG
  g_print("tile_attach: %p -> (%p,%d) *%d\n", tile, tm, tile_num, tile->share_count);
#endif

  /* link this tile into the tile's tilelink chain */
  tmp = g_new (TileLink, 1);
  tmp->tm = tm;
  tmp->tile_num = tile_num;
  tmp->next = tile->tlink;
  tile->tlink = tmp;
}

void
tile_detach (Tile *tile, void *tm, int tile_num)
{
  TileLink **link;
  TileLink *tmp;

#ifdef TILE_DEBUG
  g_print("tile_detach: %p ~> (%p,%d) *%d\n", tile, tm, tile_num, tile->share_count);
#endif

  for (link = &tile->tlink; *link; link = &(*link)->next)
    if ((*link)->tm == tm && (*link)->tile_num == tile_num)
      break;

  if (*link == NULL) 
    {
      g_warning ("Tried to detach a nonattached tile");
      return;
    }

  tmp = *link;
  *link = tmp->next;
  g_free (tmp);

  tile_share_count--;
  tile->share_count--;
 
  if (tile->share_count == 0 && tile->ref_count == 0)
    {
      tile_destroy (tile);
      return;
    }
  TILE_MUTEX_UNLOCK (tile);
}


