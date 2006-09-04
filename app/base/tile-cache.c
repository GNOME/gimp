/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "base-types.h"

#include "tile.h"
#include "tile-cache.h"
#include "tile-swap.h"
#include "tile-private.h"


#define IDLE_SWAPPER_TIMEOUT  250


static gboolean  tile_cache_zorch_next     (void);
static void      tile_cache_flush_internal (Tile     *tile);

#ifdef ENABLE_THREADED_TILE_SWAPPER
static gpointer  tile_idle_thread          (gpointer  data);
#else
static gboolean  tile_idle_preswap         (gpointer  data);
#endif


static gboolean initialize = TRUE;

typedef struct _TileList
{
  Tile *first;
  Tile *last;
} TileList;

static gulong   max_tile_size   = TILE_WIDTH * TILE_HEIGHT * 4;
static gulong   cur_cache_size  = 0;
static gulong   max_cache_size  = 0;
static gulong   cur_cache_dirty = 0;
static TileList clean_list      = { NULL, NULL };
static TileList dirty_list      = { NULL, NULL };


#ifdef ENABLE_MP

static GStaticMutex   tile_cache_mutex = G_STATIC_MUTEX_INIT;

#define CACHE_LOCK    g_static_mutex_lock (&tile_cache_mutex)
#define CACHE_UNLOCK  g_static_mutex_unlock (&tile_cache_mutex)

#else

#define CACHE_LOCK   /* nothing */
#define CACHE_UNLOCK /* nothing */

#endif


#ifdef ENABLE_THREADED_TILE_SWAPPER

static GThread *preswap_thread = NULL;
static GMutex  *dirty_mutex    = NULL;
static GCond   *dirty_signal   = NULL;

#else

static guint    idle_swapper   = 0;

#endif


void
tile_cache_init (gulong tile_cache_size)
{
  if (initialize)
    {
      initialize = FALSE;

      clean_list.first = clean_list.last = NULL;
      dirty_list.first = dirty_list.last = NULL;

      max_cache_size = tile_cache_size;

#ifdef ENABLE_THREADED_TILE_SWAPPER
      dirty_mutex  = g_mutex_new ();
      dirty_signal = g_cond_new ();

      preswap_thread = g_thread_create (&tile_idle_thread, NULL, FALSE, NULL);
#endif
    }
}

void
tile_cache_exit (void)
{
#ifndef ENABLE_THREADED_TILE_SWAPPER
  if (idle_swapper)
    {
      g_source_remove (idle_swapper);
      idle_swapper = 0;
    }
#endif

  tile_cache_set_size (0);
}

void
tile_cache_insert (Tile *tile)
{
  TileList *list;
  TileList *newlist;

  CACHE_LOCK;

  if (! tile->data)
    goto out;

  /* First check and see if the tile is already
   *  in the cache. In that case we will simply place
   *  it at the end of the tile list to indicate that
   *  it was the most recently accessed tile.
   */

  list = (TileList *) tile->listhead;

  newlist = ((tile->dirty || tile->swap_offset == -1) ?
             &dirty_list : &clean_list);

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

      if (list == &dirty_list)
        cur_cache_dirty -= tile->size;
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
          if (! tile_cache_zorch_next ())
            {
              g_warning ("cache: unable to find room for a tile");
              goto out;
            }
        }

      cur_cache_size += tile->size;
    }

  /* Put the tile at the end of the proper list */

  tile->next = NULL;
  tile->prev = newlist->last;
  tile->listhead = newlist;

  if (newlist->last)
    newlist->last->next = tile;
  else
    newlist->first = tile;

  newlist->last = tile;

  /* gosgood@idt.net 1999-12-04                                  */
  /* bytes on cur_cache_dirty miscounted in CVS 1.12:            */
  /* Invariant: test for selecting dirty list should be the same */
  /* as counting files dirty.                                    */

  if (tile->dirty || (tile->swap_offset == -1))
    {
      cur_cache_dirty += tile->size;

#ifdef ENABLE_THREADED_TILE_SWAPPER
      g_mutex_lock (dirty_mutex);
      g_cond_signal (dirty_signal);
      g_mutex_unlock (dirty_mutex);
#else
      if (! idle_swapper &&
          cur_cache_dirty * 2 > max_cache_size)
        {
          idle_swapper = g_timeout_add_full (G_PRIORITY_LOW,
                                             IDLE_SWAPPER_TIMEOUT,
                                             tile_idle_preswap,
                                             NULL, NULL);
        }
#endif
    }

out:
  CACHE_UNLOCK;
}

void
tile_cache_flush (Tile *tile)
{
  CACHE_LOCK;

  tile_cache_flush_internal (tile);

  CACHE_UNLOCK;
}

static void
tile_cache_flush_internal (Tile *tile)
{
  TileList *list;

  /* Find where the tile is in the cache.
   */

  list = (TileList *) tile->listhead;

  if (list)
    {
      cur_cache_size -= tile->size;

      if (list == &dirty_list)
        cur_cache_dirty -= tile->size;

      if (tile->next)
        tile->next->prev = tile->prev;
      else
        list->last = tile->prev;

      if (tile->prev)
        tile->prev->next = tile->next;
      else
        list->first = tile->next;

      tile->listhead = NULL;
    }
}


void
tile_cache_set_size (gulong cache_size)
{
  CACHE_LOCK;

  max_cache_size = cache_size;

  while (cur_cache_size > max_cache_size)
    {
      if (! tile_cache_zorch_next ())
        break;
    }

  CACHE_UNLOCK;
}


static gboolean
tile_cache_zorch_next (void)
{
  Tile *tile;

  if (clean_list.first)
    tile = clean_list.first;
  else if (dirty_list.first)
    tile = dirty_list.first;
  else
    return FALSE;

  TILE_MUTEX_LOCK (tile);

  tile_cache_flush_internal (tile);

  if (tile->dirty || tile->swap_offset == -1)
    {
      tile_swap_out (tile);
    }

  if (! tile->dirty)
    {
      g_free (tile->data);
      tile->data = NULL;
      TILE_MUTEX_UNLOCK (tile);

      return TRUE;
    }

  /* unable to swap out tile for some reason */
  TILE_MUTEX_UNLOCK (tile);

  return FALSE;
}


#ifdef ENABLE_THREADED_TILE_SWAPPER

static gpointer
tile_idle_thread (gpointer data)
{
  Tile     *tile;
  TileList *list;
  gint      count;

  g_printerr ("starting tile preswapper thread\n");

  count = 0;
  while (TRUE)
    {
      CACHE_LOCK;

      if (count > 5 || dirty_list.first == NULL)
        {
          CACHE_UNLOCK;

          count = 0;

          g_mutex_lock (dirty_mutex);
          g_cond_wait (dirty_signal, dirty_mutex);
          g_mutex_unlock (dirty_mutex);

          CACHE_LOCK;
        }

      if ((tile = dirty_list.first))
        {
          CACHE_UNLOCK;
          TILE_MUTEX_LOCK (tile);
          CACHE_LOCK;

          if (tile->dirty || tile->swap_offset == -1)
            {
              list = tile->listhead;

              if (list == &dirty_list)
                cur_cache_dirty -= tile->size;

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

              if (clean_list.last)
                clean_list.last->next = tile;
              else
                clean_list.first = tile;

              clean_list.last = tile;

              CACHE_UNLOCK;

              tile_swap_out (tile);
            }
          else
            {
              CACHE_UNLOCK;
            }

          TILE_MUTEX_UNLOCK (tile);
        }
      else
        {
          CACHE_UNLOCK;
        }

      count++;
    }
}

#else  /* !ENABLE_THREADED_TILE_SWAPPER */

static gboolean
tile_idle_preswap (gpointer data)
{
  Tile *tile;

  if (cur_cache_dirty * 2 < max_cache_size)
    {
      idle_swapper = 0;
      return FALSE;
    }

  CACHE_LOCK;

  if ((tile = dirty_list.first))
    {
      tile_swap_out (tile);

      dirty_list.first = tile->next;

      if (tile->next)
        tile->next->prev = NULL;
      else
        dirty_list.last = NULL;

      tile->next = NULL;
      tile->prev = clean_list.last;
      tile->listhead = &clean_list;

      if (clean_list.last)
        clean_list.last->next = tile;
      else
        clean_list.first = tile;

      clean_list.last = tile;
      cur_cache_dirty -= tile->size;
    }

  CACHE_UNLOCK;

  return TRUE;
}

#endif  /* !ENABLE_THREADED_TILE_SWAPPER */
