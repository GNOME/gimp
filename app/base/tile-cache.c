#include <gtk/gtkmain.h>
#include <glib.h>
#include "gimprc.h"
#include "tile.h"
#include "tile_cache.h"
#include "tile_swap.h"
#ifdef USE_PTHREADS
#include <pthread.h>
#endif

#include "tile_pvt.h"			/* ick. */

#include "stdio.h"

/*  This is the percentage of the maximum cache size that should be cleared
 *   from the cache when an eviction is necessary
 */
#define FREE_QUANTUM 0.1

static void  tile_cache_init           (void);
static gint  tile_cache_zorch_next     (void);
static void  tile_cache_flush_internal (Tile *tile);

#ifdef USE_PTHREADS
static void* tile_idle_thread          (void *);
#else
static gint  tile_idle_preswap         (gpointer);
#endif

static int initialize = TRUE;

typedef struct _TileList {
  Tile* first;
  Tile* last;
} TileList;

static unsigned long max_tile_size = TILE_WIDTH * TILE_HEIGHT * 4;
static unsigned long cur_cache_size = 0;
static unsigned long max_cache_size = 0;
static unsigned long cur_cache_dirty = 0;
static TileList clean_list = { NULL, NULL };
static TileList dirty_list = { NULL, NULL };

#ifdef USE_PTHREADS
static pthread_t preswap_thread;
static pthread_mutex_t dirty_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t dirty_signal = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t tile_mutex  = PTHREAD_MUTEX_INITIALIZER;
#else
static gint idle_swapper = 0;
#endif


void
tile_cache_insert (Tile *tile)
{
  TileList *list;
  TileList *newlist;

  if (initialize)
    tile_cache_init ();

#if USE_PTHREADS
  pthread_mutex_lock(&tile_mutex);
#endif
  if (tile->data == NULL) goto out;	

  /* First check and see if the tile is already
   *  in the cache. In that case we will simply place
   *  it at the end of the tile list to indicate that
   *  it was the most recently accessed tile.
   */

  list = (TileList*)(tile->listhead);
  newlist = (tile->dirty || tile->swap_offset == -1) ? &dirty_list : &clean_list;

  /* if list is NULL, the tile is not in the cache */

  if (list) 
    {
      /* Tile is in the cache.  Remove it from its current list and
	 put it at the tail of the proper list (clean or dirty) */

      if (tile->next) 
	tile->next->prev = tile->prev;
      else
	list->last = tile->prev;
      
      if (tile->prev)
	tile->prev->next = tile->next;
      else
	list->first = tile->next;

      tile->listhead = NULL;
      if (list == &dirty_list) cur_cache_dirty -= tile_size (tile);
    }
  else
    {
      /* The tile was not in the cache. First check and see
       *  if there is room in the cache. If not then we'll have
       *  to make room first. Note: it might be the case that the
       *  cache is smaller than the size of a tile in which case
       *  it won't be possible to put it in the cache.
       */
      while ((cur_cache_size + max_tile_size) > max_cache_size)
	{
	  if (!tile_cache_zorch_next()) goto out;
	}
      
      /* Note the increase in the number of bytes the cache
       *  is referencing.
       */
      cur_cache_size += tile_size (tile);

      /* Reference the tile so that it won't be swapped out
       *  to disk. Swap the tile in if necessary.
       * "tile_ref" cannot be used here since it calls this
       *  function.
       */
      tile->ref_count += 1;
      {
	extern int tile_ref_count;
	tile_ref_count += 1;
      }
      if (tile->ref_count == 1)
	{
	  tile_swap_in (tile);

	  /*  the tile must be clean  */
	  tile->dirty = FALSE;
	}
    }

  /* Put the tile at the end of the proper list */

  tile->next = NULL;
  tile->prev = newlist->last;
  tile->listhead = newlist;

  if (newlist->last) newlist->last->next = tile;
  else               newlist->first = tile;
  newlist->last = tile;

  if (tile->dirty) 
    {
      cur_cache_dirty += tile_size (tile);
#ifdef USE_PTHREADS
      pthread_mutex_lock(&dirty_mutex);
      pthread_cond_signal(&dirty_signal);
      pthread_mutex_unlock(&dirty_mutex);
#endif
    }
out:
#ifdef USE_PTHREADS
  pthread_mutex_unlock(&tile_mutex);
#endif

}

void
tile_cache_flush (Tile *tile)
{
  if (initialize)
    tile_cache_init ();

#ifdef USE_PTHREADS
  pthread_mutex_lock(&tile_mutex);
#endif

  tile_cache_flush_internal (tile);

#ifdef USE_PTHREADS
  pthread_mutex_unlock(&tile_mutex);
#endif
}

static void
tile_cache_flush_internal (Tile *tile)
{
  TileList *list;

  /* Find where the tile is in the cache.
   */
  
  list = (TileList*)(tile->listhead);

  if (list) 
    {
      /* Note the decrease in the number of bytes the cache
       *  is referencing.
       */
      cur_cache_size -= tile_size (tile);
      if (list == &dirty_list) cur_cache_dirty -= tile_size (tile);

      if (tile->next) 
	tile->next->prev = tile->prev;
      else
	list->last = tile->prev;
      
      if (tile->prev)
	tile->prev->next = tile->next;
      else
	list->first = tile->next;

      tile->listhead = NULL;
      
      /* Unreference the tile.
       */
      {
	extern int tile_ref_count;
	tile_ref_count -= 1;
      }

      /* Decrement the reference count.
       */
      tile->ref_count -= 1;

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
    }
}

void
tile_cache_set_size (unsigned long cache_size)
{
  if (initialize)
    tile_cache_init ();

  max_cache_size = cache_size;

#if USE_PTHREADS
  pthread_mutex_lock(&tile_mutex);
#endif
  while (cache_size >= max_cache_size)
    {
      if (!tile_cache_zorch_next ()) break;
    }
#if USE_PTHREADS
  pthread_mutex_unlock(&tile_mutex);
#endif
}


static void
tile_cache_init ()
{
  if (initialize)
    {
      initialize = FALSE;

      clean_list.first = clean_list.last = NULL;
      dirty_list.first = dirty_list.last = NULL;

      max_cache_size = tile_cache_size;

#ifdef USE_PTHREADS
      pthread_create (&preswap_thread, NULL, &tile_idle_thread, NULL);
#else
      idle_swapper = gtk_timeout_add (250,
				      (GtkFunction) tile_idle_preswap, 
				      (gpointer) 0);
#endif
    }
}

static gint
tile_cache_zorch_next ()
{
  Tile *tile;

  if      (clean_list.first) tile = clean_list.first;
  else if (dirty_list.first) tile = dirty_list.first;
  else return FALSE;

#ifdef USE_PTHREADS
  pthread_mutex_unlock(&tile_mutex);
  pthread_mutex_lock(&(tile->mutex));
  pthread_mutex_lock(&tile_mutex);
#endif
  tile_cache_flush_internal (tile);
#ifdef USE_PTHREADS
  pthread_mutex_unlock(&(tile->mutex));
#endif
  return TRUE;
}

#if USE_PTHREADS
static void *
tile_idle_thread (void *data)
{
  Tile *tile;
  TileList *list;

  fprintf (stderr,"starting tile preswapper\n");

  while(1) 
    {
      if (!dirty_list.first) sleep(1);
      if (!dirty_list.first) {
	pthread_mutex_lock(&dirty_mutex);
	pthread_cond_wait(&dirty_signal,&dirty_mutex);
	pthread_mutex_unlock(&dirty_mutex);
      }
      if ((tile = dirty_list.first) &&
	  (pthread_mutex_lock(&(tile->mutex)) == 0))
	{
	  pthread_mutex_lock(&tile_mutex);

	  list = tile->listhead;

	  if (list == &dirty_list) cur_cache_dirty -= tile_size (tile);
	  
	  if (tile->next) 
	    tile->next->prev = tile->prev;
	  else
	    list->last = tile->prev;
      
	  if (tile->prev)
	    tile->prev->next = tile->next;
	  else
	    list->first = tile->next;

	  tile->next = NULL;
	  tile->prev = clean_list.last;
	  tile->listhead = &clean_list;

	  if (clean_list.last) clean_list.last->next = tile;
	  else                 clean_list.first = tile;
	  clean_list.last = tile;

	  pthread_mutex_unlock(&tile_mutex);
	  tile_swap_out(tile);
	  pthread_mutex_unlock(&(tile->mutex));
	}
    }
}

#else
static gint
tile_idle_preswap (gpointer data)
{
  Tile *tile;
  if (cur_cache_dirty*2 < max_cache_size) return TRUE;
  if ((tile = dirty_list.first)) 
    {
      tile_swap_out(tile);

      dirty_list.first = tile->next;
      if (tile->next) 
	tile->next->prev = NULL;
      else
	dirty_list.last = NULL;

      tile->next = NULL;
      tile->prev = clean_list.last;
      tile->listhead = &clean_list;
      if (clean_list.last) clean_list.last->next = tile;
      else                 clean_list.first = tile;
      clean_list.last = tile;
      cur_cache_dirty -= tile_size (tile);
    }
  
  return TRUE;
}
#endif
