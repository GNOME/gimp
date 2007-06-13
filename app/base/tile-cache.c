/* GIMP - The GNU Image Manipulation Program
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

static gboolean  tile_idle_preswap         (gpointer  data);


static gboolean initialize = TRUE;

typedef struct _TileList
{
  Tile *first;
  Tile *last;
} TileList;

static const gulong  max_tile_size   = TILE_WIDTH * TILE_HEIGHT * 4;
static gulong        cur_cache_size  = 0;
static gulong        max_cache_size  = 0;
static gulong        cur_cache_dirty = 0;
static TileList      clean_list      = { NULL, NULL };
static TileList      dirty_list      = { NULL, NULL };
static guint         idle_swapper    = 0;


#ifdef ENABLE_MP

static GStaticMutex   tile_cache_mutex = G_STATIC_MUTEX_INIT;

#define CACHE_LOCK    g_static_mutex_lock (&tile_cache_mutex)
#define CACHE_UNLOCK  g_static_mutex_unlock (&tile_cache_mutex)

#else

#define CACHE_LOCK   /* nothing */
#define CACHE_UNLOCK /* nothing */

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
    }
}

void
tile_cache_exit (void)
{
  if (idle_swapper)
    {
      g_source_remove (idle_swapper);
      idle_swapper = 0;
    }

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

  list = tile->listhead;

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

  if (tile->dirty || (tile->swap_offset == -1))
    {
      cur_cache_dirty += tile->size;

      if (! idle_swapper &&
          cur_cache_dirty * 2 > max_cache_size)
        {
          idle_swapper = g_timeout_add_full (G_PRIORITY_LOW,
                                             IDLE_SWAPPER_TIMEOUT,
                                             tile_idle_preswap,
                                             NULL, NULL);
        }
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

static void
tile_cache_flush_internal (Tile *tile)
{
  TileList *list = tile->listhead;

  /* Find where the tile is in the cache.
   */

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

  tile_cache_flush_internal (tile);

  if (tile->dirty || tile->swap_offset == -1)
    {
      tile_swap_out (tile);
    }

  if (! tile->dirty)
    {
      g_free (tile->data);
      tile->data = NULL;

      return TRUE;
    }

  /* unable to swap out tile for some reason */
  return FALSE;
}

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
