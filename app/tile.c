#include "tile.h"
#include "tile_cache.h"
#include "tile_manager.h"
#include "tile_swap.h"


void
tile_init (Tile *tile,
	   int   bpp)
{
  tile->ref_count = 0;
  tile->dirty = FALSE;
  tile->valid = FALSE;
  tile->ewidth = TILE_WIDTH;
  tile->eheight = TILE_HEIGHT;
  tile->bpp = bpp;
  tile->tile_num = -1;
  tile->data = NULL;
  tile->swap_num = 1;
  tile->swap_offset = -1;
  tile->tm = NULL;
  tile->next = tile->prev = NULL;
  tile->listhead = NULL;
#ifdef USE_PTHREADS
  {
    pthread_mutex_init(&tile->mutex, NULL);
  }
#endif
}

int tile_ref_count = 0;

void
#if defined (TILE_DEBUG) && defined (__GNUC__)
_tile_ref (Tile *tile, char *func_name)
#else
tile_ref (Tile *tile)
#endif
{
#ifdef USE_PTHREADS
  pthread_mutex_lock(&(tile->mutex));
#endif
  /*  g_print ("tile_ref:    0x%08x  %s\n", tile, func_name); */

  tile_ref_count += 1;

  /* Increment the reference count.
   */
  tile->ref_count += 1;

  /* If this is the first reference to the tile then
   *  swap the tile data in from disk. Note: this will
   *  properly handle the case where the tile data isn't
   *  on disk.
   */
  if (tile->ref_count == 1)
    {
      tile_swap_in (tile);

      /*  the tile must be clean  */
      tile->dirty = FALSE;
    }

  /* Insert the tile into the cache. If the tile is already
   *  in the cache this will have the affect of "touching"
   *  the tile.
   */
  tile_cache_insert (tile);

  /* Call 'tile_manager_validate' if the tile was invalid.
   */

#if USE_PTHREADS
  pthread_mutex_unlock(&(tile->mutex));
#endif
  if (!tile->valid)
    tile_manager_validate ((TileManager*) tile->tm, tile);
  

}

void
#if defined (TILE_DEBUG) && defined (__GNUC__)
_tile_unref (Tile *tile, int dirty, char *func_name)
#else
tile_unref (Tile *tile, int dirty)
#endif
{
#ifdef USE_PTHREADS
  pthread_mutex_lock(&(tile->mutex));
#endif
  /*  g_print ("tile_unref:  0x%08x  %s\n", tile, func_name); */

  tile_ref_count -= 1;

  /* Decrement the reference count.
   */
  tile->ref_count -= 1;

  /* Mark the tile dirty if indicated
   */
  if (dirty && !tile->dirty) 
    {
      tile->dirty = TRUE;
      tile_cache_insert (tile);
    }

  /* If this was the last reference to the tile, then
   *  swap it out to disk.
   */
  if (tile->ref_count == 0)
    {
      /*  Only need to swap out in two cases:
       *   1)  The tile is dirty
       *   2)  The tile has never been swapped
       */
      if (tile->dirty || tile->swap_offset == -1)
	tile_swap_out (tile);
      /*  Otherwise, just throw out the data--the same stuff is in swap
       */
      g_free (tile->data);
      tile->data = NULL;
    }
#if USE_PTHREADS
  pthread_mutex_unlock(&(tile->mutex));
#endif
}

void
tile_alloc (Tile *tile)
{
  if (tile->data)
    goto out;

  /* Allocate the data for the tile.
   */
  tile->data = g_new (guchar, tile_size (tile));
out:
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
tile_invalidate (Tile *tile)
{
#ifdef USE_PTHREADS
  pthread_mutex_lock(&(tile->mutex));
#endif
  /* Invalidate the tile. (Must be valid first).
   */
  if (tile->valid)
    {
      tile->valid = FALSE;

      if (tile->data)
	{
	  /* If the tile is in memory then trick the
	   *  tile cache routines. We temporarily increment
	   *  the reference count so that flushing a tile does
	   *  not cause it to go out to disk.
	   */
	  tile->ref_count += 1;
	  tile_cache_flush (tile);
	  tile->ref_count -= 1;

	  /* We then free the tile data.
	   */
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
    }
#if USE_PTHREADS
  pthread_mutex_unlock(&(tile->mutex));
#endif
}
