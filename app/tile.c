#include <stdio.h>

#include "tile.h"
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

/*
 * Define MUCH_TILE_DEBUG in addition to TILE_DEBUG to get
 * debugging for every single tile_ref2 and tile_unref (that's
 * a lot).
#define MUCH_TILE_DEBUG heckyeah
*/



void
tile_init (Tile *tile,
	   int   bpp)
{
  tile->ref_count = 0;
  tile->dirty = FALSE;
  tile->valid = FALSE;
  tile->data = NULL;
  tile->real_tile_ptr = NULL;
  tile->mirrored_by = NULL;
  tile->ewidth = TILE_WIDTH;
  tile->eheight = TILE_HEIGHT;
  tile->bpp = bpp;
  tile->tile_num = -1;
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
  /* While things get moved over to the new tile_ref2
   * interface, tile_ref is a wrapper which just says
   * 'yes, we'll be dirtying the tile'
   */

#if defined (TILE_DEBUG) && defined (__GNUC__)
  printf("COW-Warning: function %s is using obsolete tile_ref interface\n",
	 func_name);
#endif

#if defined (TILE_DEBUG) && defined (__GNUC__)
  _tile_ref2 (tile, TRUE, func_name);
#else
  tile_ref2 (tile, TRUE);
#endif
}


gboolean
tile_is_mirroring (Tile *tile)
{
  return (tile->real_tile_ptr != NULL);
}


gboolean
tile_is_mirrored (Tile *tile)
{
  return (tile->mirrored_by != NULL);
}


gboolean
tile_is_real (Tile *tile)
{
  return (tile->real_tile_ptr == NULL);
}


/* Follow the real_tile_ptr links back to the tile which provides
 *  the real source data for the given tile
 */
Tile *
tile_find_nonmirroring (Tile *tile)
{
  if (! tile_is_mirroring (tile))
    {
      return tile;
    }
  else
    {
      return tile_find_nonmirroring (tile->real_tile_ptr);
    }
}


/* 
 */
Tile *
tile_find_finalmirroring (Tile *tile)
{
  if (! tile_is_mirrored (tile))
    {
      return tile;
    }
  else
    {
      return tile_find_finalmirroring (tile->mirrored_by);
    }
}


/* Take a mirroring-tile and turn it into a bona fide self-contained
 *  tile.
 */
void
tile_devirtualize (Tile *tile)
{
  Tile *real_tile;

  /* Sanity */
  if (tile_is_real (tile))
    g_error ("Tried to devirtualize a real tile");
#if defined (TILE_DEBUG)
  if (tile->ref_count == 0)
    g_warning ("Trying to devirtualize a mirroring-tile with no ref_count");
#endif

  /* Go find the tile ('real_tile') which owns the real data
   */
  real_tile = tile_find_nonmirroring (tile);

  /* Sanity */
#if defined (TILE_DEBUG)
  if (real_tile->ref_count == 0)
    g_warning ("Trying to devirtualize a mirroring-tile whose real_tile has no ref_count");
#endif
  if (!real_tile->valid)
    g_warning ("Trying to devirtualize a mirroring-tile whose real_tile is !valid");

  /* Copy the actual tile data from the real_tile to this tile
   */
  tile->data = NULL;
  tile_alloc (tile);
/*  printf ("{ %dx%d : %d  - %p[%p]->%p[%p] }", real_tile->ewidth, real_tile->eheight,
	  real_tile->bpp, real_tile, real_tile->data, tile, tile->data);
  fflush(stdout);*/

  memcpy (tile->data, real_tile->data, tile_size(real_tile));

  /* 'tile' is now a real tile. */
  tile->real_tile_ptr = NULL;
  tile->valid = TRUE;

  tile_cache_insert(tile);

#if defined (TILE_DEBUG)
  g_print ("Tile at %p is now devirtualized.\n", tile);
#endif
}


/* Make this tile self-contained.  
 *
 * The next tile in the linked-list of tiles which are mirroring 'tile'
 *  is promoted to a real physical tile and unlinked from 'tile'.  This
 *  renders 'tile' safe for dirtying (or destruction).
 */
void
tile_isolate (Tile *tile)
{
  Tile *temp_tileptr;

  /* Sanity
   */
  if (! (tile_is_mirrored (tile) || tile_is_mirroring (tile)))
    {
      g_warning ("Tried to isolate a tile which is neither a mirror source "
		 "nor destination");
      return;
    }

  /* This tile is both linked to and linked from? */
  if (tile_is_mirrored (tile) && tile_is_mirroring (tile))
    {
      temp_tileptr = tile->real_tile_ptr;

#if defined (TILE_DEBUG)
      g_print ("tile %p: was middle of chain - relinking %p and %p\n",
	       tile,
	       temp_tileptr,
	       tile->mirrored_by);
#endif

      tile->mirrored_by->real_tile_ptr = temp_tileptr;
      temp_tileptr->mirrored_by = tile->mirrored_by;
      
      tile_ref2 (temp_tileptr, FALSE);
      tile_devirtualize (tile);
      tile_unref (temp_tileptr, FALSE);

      tile->mirrored_by = NULL;

      return;
    }

  /* This tile is mirroring another, but is not mirrored itself? */
  if (tile_is_mirroring (tile))
    {
      temp_tileptr = tile->real_tile_ptr;

#if defined (TILE_DEBUG)
      g_print ("tile %p: was end of chain - cauterizing %p\n",
	       tile,
	       temp_tileptr);
#endif

      /* We stop mirroring the tile which we previously were -
       *  so reset that tile's mirrored_by pointer.
       */
      temp_tileptr->mirrored_by = NULL;

      tile_ref2 (temp_tileptr, FALSE);
      tile_devirtualize (tile);
      tile_unref (temp_tileptr, FALSE);

      return;
    }

  /* This tile is mirrored by another, but is not itself a mirror. */
  if (tile_is_mirrored (tile))
    {
#if defined (TILE_DEBUG)
      g_print ("tile %p: was source of chain - devirtualizing %p\n",
	       tile,
	       tile->mirrored_by);
#endif

     temp_tileptr = tile->mirrored_by;

      tile_ref2 (temp_tileptr, FALSE);
      tile_devirtualize (temp_tileptr);
      tile_unref (temp_tileptr, FALSE);

      /* The tile which was dependant on this one no longer is -
       *  so we can unref once.
       */
      tile_unref (tile, FALSE);

      tile->mirrored_by = NULL;

      return;
    }
}


/* Turns dest_tile into a mirroring-tile which mirrors the given
 *  src_tile using copy-on-write.
 */
void
tile_mirror (Tile *dest_tile, Tile *src_tile)
{
  Tile *finalmirroring;

  if (dest_tile == src_tile)
    {
      g_warning ("TRIED TO MIRROR TILE TO ITSELF");
      return;
    }

#if defined (TILE_DEBUG)
  g_print ("mirroring "); 
#endif

  if (tile_is_real (dest_tile))
    {
#if defined (TILE_DEBUG)
      g_print ("TO REAL ");   
#endif
      tile_invalidate (dest_tile);
    }
  else
    {
      tile_invalidate (dest_tile);
    }

  /*      dest_tile->ref_count = 0; */
  dest_tile->dirty = FALSE;
  dest_tile->valid = FALSE;
  dest_tile->data = NULL;
  dest_tile->ewidth = src_tile->ewidth;
  dest_tile->eheight = src_tile->eheight;
  dest_tile->bpp = src_tile->bpp;
  dest_tile->tile_num = -1; /* ! */

  /* 
   */
  finalmirroring = tile_find_finalmirroring (src_tile);
  dest_tile->real_tile_ptr = finalmirroring;
  finalmirroring->mirrored_by = dest_tile;
  
#if defined (TILE_DEBUG)
  g_print ("%p -> %p\n", finalmirroring, dest_tile);
#endif

  /* The following should be irrelevant in a mirroring tile - mirroring
   *  tiles by definition don't have real data of their own, so they can't
   *  be swapped.  They don't have associated TileManagers either, since they
   *  rely on their mirrored source tile to contain validated data.
   */
  dest_tile->swap_num = 1;
  dest_tile->swap_offset = -1;
  dest_tile->tm = NULL;
}


void
#if defined (TILE_DEBUG) && defined (__GNUC__)
_tile_ref2 (Tile *tile, int dirty, char *func_name)
#else
tile_ref2 (Tile *tile, int dirty)
#endif
{
#ifdef USE_PTHREADS
  pthread_mutex_lock(&(tile->mutex));
#endif
#if defined (TILE_DEBUG) && defined (__GNUC__) && defined (MUCH_TILE_DEBUG)
  g_print ("tile_ref2:   %02d  %c  %p  %s\n", tile->ref_count,
	   dirty?'d':' ',
	   tile,
	   func_name);
#endif

  /*g_print ("tile_ref2:   %02d  %c  %p\n", tile->ref_count,
	   dirty?'d':' ',
	   tile);*/



  /* Increment the global reference count.
   */
  tile_ref_count += 1;

  /* Increment this tile's reference count.
   */
  tile->ref_count += 1;

#if defined (TILE_DEBUG)
  if (tile_is_mirrored (tile) && dirty)
    {
      g_print ("Dirtying a mirrored tile: %p.\n", tile);
    }
#endif

/*
  if (dirty && tile->dirty)
    {
      g_print ("Not good: Dirtying a write-locked tile: %p.\n", tile);
    } */


  /* if this is a read-only attachment to a mirroring tile,
   *  then ref the chain, update the data pointer, and return.
   */
  if ((!dirty) && (tile_is_mirroring (tile)))
    {
      /* ref each of the tiles in the chain, back to the
       *  'real' tile which sits at the start.
       */
#if USE_PTHREADS
      pthread_mutex_unlock(&(tile->mutex));
#endif
      tile_ref2 (tile->real_tile_ptr, FALSE);
      tile->data = tile->real_tile_ptr->data;
      return;
    }


  /* dirty, or clean-and-real */


  /* Real tile - first reference. */
  if (!tile_is_mirroring (tile))
    {
      /* If this is the first reference to the tile then
       *  swap the tile data in from disk. Note: this will
       *  properly handle the case where the tile data isn't
       *  on disk.
       */
      if (tile->ref_count == 1)
	{
	  tile_swap_in (tile);
	}
      
      /* Insert the tile into the cache. If the tile is already
       *  in the cache this will have the affect of "touching"
       *  the tile.
       */
      tile_cache_insert (tile);
    }


  /* Read/write attachment to a mirrored/ing tile - must be
   *  thoughtful.
   */
  if (dirty)
    {
      /* Doing a read/write reference to a mirroring/ed tile -
       *  we'll have to turn the tile into a 'real' tile.
       *  Then return - we're done.
       */
      {
	if (tile_is_mirroring (tile) | tile_is_mirrored (tile))
	  {
#if defined (TILE_DEBUG)
	    g_print ("r/w to mir'd/ing - isolating: ");
#endif

	    tile_isolate (tile);
	  }
      }
    }

  /* Mark the tile as dirty if it's being ref'd as dirtyable.
   */
  tile->dirty |= dirty;

#if USE_PTHREADS
  pthread_mutex_unlock(&(tile->mutex));
#endif

  /* Call 'tile_manager_validate' if the tile was invalid.
   */
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

#if defined (TILE_DEBUG) && defined (__GNUC__) && defined (MUCH_TILE_DEBUG)
  g_print ("tile_unref:  %02d  %c  %p  %s\n", tile->ref_count,
	   dirty?'d':' ',
	   tile, func_name);
#endif

  /*    g_print ("tile_unref:  %02d  %c  %p\n", tile->ref_count,
	   dirty?'d':' ',
	   tile);*/

  /* Decrement the global reference count.
   */
  tile_ref_count -= 1;

  /* Decrement this tile's reference count.
   */
  tile->ref_count -= 1;

  /* Mark the tile dirty if indicated
   *
   *  commented out - we now dirty on ref, not unref
   */
  /*
  tile->dirty |= dirty;
  */

#if defined (TILE_DEBUG)
  if (tile_is_mirroring (tile))
    {
      /* Mirroring tiles aren't allowed to be submitted as dirty -
       * they should have been isolated at ref time.
       */
      if (dirty)
	g_warning ("Mirroring tile unref'd as dirty.");
    }

  if (tile_is_mirrored (tile))
    {
      /* Mirrored tiles aren't allowed to be submitted as dirty -
       * they should have been isolated at ref time.
       */
      fflush(stdout);
      if (dirty)
	g_warning ("Mirrored tile unref'd as dirty.");
    }
#endif


  /* When we unref a mirroring tile, also unref the tile which
   * was being mirrored.
   */
  if (tile_is_mirroring (tile))
    {
      /* Mirroring tiles aren't allowed to be submitted as dirty -
       * they should have been ref'd as dirty in the first place so we
       * could turn them into 'real' tiles.
       */
      if (dirty)
	{
	  g_warning ("Bleh, tried to unref a mirroring tile as dirty.");
	}

      /* Go find the mirrored tile and unref that too. */
#if USE_PTHREADS
      pthread_mutex_unlock(&(tile->mutex));
#endif
      tile_unref (tile->real_tile_ptr, FALSE);

      return;
    }

  /* If this was the last reference to the tile, then
   *  swap it out to disk.
   */
  if (tile->ref_count == 0)
    {
      /*  Only need to swap out in two cases:
       *   1)  The tile is dirty               }
       *   2)  The tile has never been swapped }  and is not mirroring
       */
      if ((tile->dirty || tile->swap_offset == -1)
	  && !tile_is_mirroring (tile))
	tile_swap_out (tile);
      /*  Otherwise, just throw out the data--the same stuff is in swap
       */
      else
	{
	  if (! tile_is_mirroring (tile))
	    g_free (tile->data);

	  tile->data = NULL;
	}
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
#if defined (TILE_DEBUG)
  if (tile->ref_count > 1)
    {
      g_print (" (inv%p:ref%d) ", tile, tile->ref_count);
    }
  else
    {
      g_print (" (inv%p) ", tile);      
    }
#endif

  /* If this tile is mirrored/ing, then maybe isolate it before we
   *  invalidate it, so that we don't accidentally delete a tile
   *  whose data is still in use by a mirror.
   */
  if (tile_is_mirrored (tile))
    {
      if (tile_is_mirroring (tile))
	{
	  /* tile is in the middle of a chain.  just relink its
	   *  successor and predecessor.  that's all we need to do for
	   *  a cleanup.
	   */
#if defined (TILE_DEBUG)
      g_print ("tile %p: was middle of chain - relinking %p and %p, "
	       "no isolation\n",
	       tile,
	       tile->real_tile_ptr,
	       tile->mirrored_by);
#endif
	  tile->mirrored_by->real_tile_ptr = tile->real_tile_ptr;
	  tile->real_tile_ptr->mirrored_by = tile->mirrored_by;
	}
      else
	{
	  /* tile is real, and mirrored.  Copy its vital statistics to
	   *  its successor in the tile chain, so it can be safely deleted.
	   */
#if defined (TILE_DEBUG)
	  g_print ("tile %p: was source of chain - successor %p swallows soul"
		   ", no isolation\n",
		   tile,
		   tile->mirrored_by);
#endif

	  /* remove 'tile' from cache - but keep the ref_count up
	   *  so that the tile_unref() which tile_cache_flush() calls
	   *  won't invalidate the tile (we'll be doing that ourselves).
	   */
	  tile->ref_count++;
	  tile_cache_flush (tile);
	  tile->ref_count--;

	  /* imbue our successor with our data pointer, validity,
	   *  tile manager, swap_num, swap_offset, and dirty
	   *  flag
	   */
	  tile->mirrored_by->data = tile->data;
	  tile->data = NULL;

	  tile->mirrored_by->dirty = tile->dirty;
	  tile->dirty = FALSE;
	  tile->mirrored_by->valid = tile->valid;
	  tile->valid = FALSE;
	  tile->mirrored_by->swap_num = tile->swap_num;
	  tile->swap_num = 0;
	  tile->mirrored_by->swap_offset = tile->swap_offset;
	  tile->swap_num = -1;
	  tile->mirrored_by->tm = tile->tm;
	  tile->tm = NULL;
	  
	  /* sever links with our successor in both directions.
	   *  our successor is now the new chain source.
	   *
	   * also register this newly-born 'real' tile with the tile cache.
	   */
	  tile->mirrored_by->real_tile_ptr = NULL;
	  tile_cache_insert (tile->mirrored_by);
	  tile->mirrored_by = NULL;

	  /* This tile is as clean and invalid as it's going to get.
	   *  Return.
	   */
	  return;
	}
    }
  else /* not mirrored, maybe mirroring */
    {
      /* for a non-real tile at the end of a chain, the only cleanup
       *  we have to do for its safe destruction is cauterize the
       *  flapping mirrored_by pointer of its predecessor on the chain.
       */
      if (tile_is_mirroring (tile))
	{
#if defined (TILE_DEBUG)
	  g_print ("tile %p: was end of chain - cauterizing %p, no "
		   "isolation\n",
		   tile,
		   tile->real_tile_ptr);
#endif
	  tile->real_tile_ptr->mirrored_by = NULL;
	}
    }

  /* If this isn't a 'real' tile then it doesn't need invalidating,
   *  since it doesn't have any unique data associated with it.
   */
  if (!tile_is_real (tile))
    {
      if (tile->valid)
	{
	  g_warning ("tried to invalidate a mirroring tile which was valid.");
	  tile->valid = FALSE;
	}
      return;
    }


  /* Only 'real' tiles permitted past this point.
   */
  

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


