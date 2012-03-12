/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>

#include "base-types.h"

#include "tile.h"
#include "tile-cache.h"
#include "tile-swap.h"
#include "tile-rowhints.h"
#include "tile-private.h"


#define IDLE_SWAPPER_START              1000
#define IDLE_SWAPPER_INTERVAL_MS        20
#define IDLE_SWAPPER_TILES_PER_INTERVAL 10


typedef struct _TileList
{
  Tile *first;
  Tile *last;
} TileList;


static guint64       cur_cache_size   = 0;
static guint64       max_cache_size   = 0;
static guint64       cur_cache_dirty  = 0;
static TileList      tile_list        = { NULL, NULL };
static guint         idle_swapper     = 0;
static guint         idle_delay       = 0;
static Tile         *idle_scan_last   = NULL;

#ifdef TILE_PROFILING
extern gulong        tile_idle_swapout;
extern gulong        tile_total_zorched;
extern gulong        tile_total_zorched_swapout;
extern glong         tile_total_interactive_sec;
extern glong         tile_total_interactive_usec;
extern gint          tile_exist_count;
#endif

#ifdef ENABLE_MP

static GMutex       *tile_cache_mutex = NULL;

#define TILE_CACHE_LOCK    g_mutex_lock (tile_cache_mutex)
#define TILE_CACHE_UNLOCK  g_mutex_unlock (tile_cache_mutex)

#else

#define TILE_CACHE_LOCK   /* nothing */
#define TILE_CACHE_UNLOCK /* nothing */

#endif

#define PENDING_WRITE(t) ((t)->dirty || (t)->swap_offset == -1)


static gboolean  tile_cache_zorch_next     (void);
static void      tile_cache_flush_internal (Tile     *tile);
static gboolean  tile_idle_preswap         (gpointer  data);
#ifdef TILE_PROFILING
static void      tile_verify               (void);
#endif


void
tile_cache_init (guint64 tile_cache_size)
{
#ifdef ENABLE_MP
  g_return_if_fail (tile_cache_mutex == NULL);

  tile_cache_mutex = g_mutex_new ();
#endif

  tile_list.first = tile_list.last = NULL;
  idle_scan_last = NULL;

  max_cache_size = tile_cache_size;
}

void
tile_cache_exit (void)
{
  if (idle_swapper)
    {
      g_source_remove (idle_swapper);
      idle_swapper = 0;
    }

  if (cur_cache_size > 0)
    g_warning ("tile cache not empty (%"G_GUINT64_FORMAT" bytes left)",
               cur_cache_size);

  tile_cache_set_size (0);

#ifdef ENABLE_MP
  g_mutex_free (tile_cache_mutex);
  tile_cache_mutex = NULL;
#endif
}

void
tile_cache_suspend_idle_swapper(void)
{
  idle_delay = 1;
}

void
tile_cache_insert (Tile *tile)
{

  TILE_CACHE_LOCK;

  if (! tile->data)
    goto out;

  /* First check and see if the tile is already
   *  in the cache. In that case we will simply place
   *  it at the end of the tile list to indicate that
   *  it was the most recently accessed tile.
   */

  if (tile->cached)
    {
      /* Tile is in the cache.  Remove it from the list. */

      if (tile->next)
        tile->next->prev = tile->prev;
      else
        tile_list.last = tile->prev;

      if(tile->prev){
	tile->prev->next = tile->next;
      }else{
	tile_list.first = tile->next;
      }

      if (PENDING_WRITE(tile))
	cur_cache_dirty -= tile->size;

      if(tile == idle_scan_last)
	idle_scan_last = tile->next;

    }
  else
    {
      /* The tile was not in the cache. First check and see
       *  if there is room in the cache. If not then we'll have
       *  to make room first. Note: it might be the case that the
       *  cache is smaller than the size of a tile in which case
       *  it won't be possible to put it in the cache.
       */

#ifdef TILE_PROFILING
      if ((cur_cache_size + tile->size) > max_cache_size)
        {
          GTimeVal now;
          GTimeVal later;

          g_get_current_time(&now);
#endif
          while ((cur_cache_size + tile->size) > max_cache_size)
            {
              if (! tile_cache_zorch_next ())
                {
                  g_warning ("cache: unable to find room for a tile");
                  goto out;
                }
            }

#ifdef TILE_PROFILING
          g_get_current_time (&later);
          tile_total_interactive_usec += later.tv_usec - now.tv_usec;
          tile_total_interactive_sec += later.tv_sec - now.tv_sec;

          if (tile_total_interactive_usec < 0)
            {
              tile_total_interactive_usec += 1000000;
              tile_total_interactive_sec--;
            }

          if (tile_total_interactive_usec > 1000000)
            {
              tile_total_interactive_usec -= 1000000;
              tile_total_interactive_sec++;
            }
        }
#endif

      cur_cache_size += tile->size;
    }

  /* Put the tile at the end of the proper list */

  tile->next = NULL;
  tile->prev = tile_list.last;

  if (tile_list.last)
    tile_list.last->next = tile;
  else
    tile_list.first = tile;

  tile_list.last = tile;
  tile->cached = TRUE;
  idle_delay = 1;

  if (PENDING_WRITE(tile))
    {
      cur_cache_dirty += tile->size;

      if (! idle_scan_last)
	idle_scan_last=tile;

      if (! idle_swapper)
        {
#ifdef TILE_PROFILING
	  g_printerr("idle swapper -> started\n");
	  g_printerr("idle swapper -> waiting");
#endif
	  idle_delay = 0;
          idle_swapper = g_timeout_add_full (G_PRIORITY_LOW,
                                             IDLE_SWAPPER_START,
                                             tile_idle_preswap,
                                             NULL, NULL);
        }
    }

out:
  TILE_CACHE_UNLOCK;
}

void
tile_cache_flush (Tile *tile)
{
  TILE_CACHE_LOCK;

  if (tile->cached)
    tile_cache_flush_internal (tile);

  TILE_CACHE_UNLOCK;
}

void
tile_cache_set_size (guint64 cache_size)
{
  TILE_CACHE_LOCK;

  idle_delay = 1;
  max_cache_size = cache_size;

  while (cur_cache_size > max_cache_size)
    {
      if (! tile_cache_zorch_next ())
        break;
    }

  TILE_CACHE_UNLOCK;
}

static void
tile_cache_flush_internal (Tile *tile)
{

  tile->cached = FALSE;

  if (PENDING_WRITE(tile))
    cur_cache_dirty -= tile->size;

  cur_cache_size -= tile->size;

  if (tile->next)
    tile->next->prev = tile->prev;
  else
    tile_list.last = tile->prev;

  if (tile->prev)
    tile->prev->next = tile->next;
  else
    tile_list.first = tile->next;

  if (tile == idle_scan_last)
    idle_scan_last = tile->next;

  tile->next = tile->prev = NULL;
}

static gboolean
tile_cache_zorch_next (void)
{

  Tile *tile = tile_list.first;

  if (! tile)
    return FALSE;

#ifdef TILE_PROFILING
  tile_total_zorched++;
  tile->zorched = TRUE;

  if (PENDING_WRITE (tile))
    {
      tile_total_zorched_swapout++;
      tile->zorchout = TRUE;
    }
#endif

  tile_cache_flush_internal (tile);

  if (PENDING_WRITE (tile))
    {
      idle_delay = 1;
      tile_swap_out (tile);
    }

  if (! tile->dirty)
    {
      g_free (tile->data);
      tile->data = NULL;

#ifdef TILE_PROFILING
      tile_exist_count--;
#endif
      return TRUE;
    }

  /* unable to swap out tile for some reason */
  return FALSE;
}

static gboolean
tile_idle_preswap_run (gpointer data)
{
  Tile *tile;
  int count = 0;

  if (idle_delay)
    {
#ifdef TILE_PROFILING
      g_printerr("\nidle swapper -> waiting");
#endif

      idle_delay = 0;
      idle_swapper = g_timeout_add_full (G_PRIORITY_LOW,
                                         IDLE_SWAPPER_START,
                                         tile_idle_preswap,
                                         NULL, NULL);
      return FALSE;
    }

  TILE_CACHE_LOCK;

#ifdef TILE_PROFILING
  g_printerr(".");
#endif

  tile = idle_scan_last;

  while (tile)
    {
      if (PENDING_WRITE (tile))
        {
          idle_scan_last = tile->next;

#ifdef TILE_PROFILING
          tile_idle_swapout++;
#endif
          tile_swap_out (tile);

          if (! PENDING_WRITE(tile))
            cur_cache_dirty -= tile->size;

          count++;
          if (count >= IDLE_SWAPPER_TILES_PER_INTERVAL)
            {
              TILE_CACHE_UNLOCK;
              return TRUE;
            }
        }

      tile = tile->next;
    }

#ifdef TILE_PROFILING
  g_printerr ("\nidle swapper -> stopped\n");
#endif

  idle_scan_last = NULL;
  idle_swapper = 0;

#ifdef TILE_PROFILING
  tile_verify ();
#endif

  TILE_CACHE_UNLOCK;

  return FALSE;
}

static gboolean
tile_idle_preswap (gpointer data)
{

  if (idle_delay){
#ifdef TILE_PROFILING
    g_printerr(".");
#endif
    idle_delay = 0;
    return TRUE;
  }

#ifdef TILE_PROFILING
  tile_verify ();
  g_printerr("\nidle swapper -> running");
#endif

  idle_swapper = g_timeout_add_full (G_PRIORITY_LOW,
				     IDLE_SWAPPER_INTERVAL_MS,
				     tile_idle_preswap_run,
				     NULL, NULL);
  return FALSE;
}

#ifdef TILE_PROFILING
static void
tile_verify (void)
{
  /* scan list linearly, count metrics, compare to running totals */
  const Tile *t;
  guint64     local_size  = 0;
  guint64     local_dirty = 0;
  guint64     acc         = 0;

  for (t = tile_list.first; t; t = t->next)
    {
      local_size += t->size;

      if (PENDING_WRITE (t))
        local_dirty += t->size;
    }

  if (local_size != cur_cache_size)
    g_printerr ("\nCache size mismatch: running=%"G_GUINT64_FORMAT
                ", tested=%"G_GUINT64_FORMAT"\n",
                cur_cache_size,local_size);

  if (local_dirty != cur_cache_dirty)
    g_printerr ("\nCache dirty mismatch: running=%"G_GUINT64_FORMAT
                ", tested=%"G_GUINT64_FORMAT"\n",
                cur_cache_dirty,local_dirty);

  /* scan forward from scan list */
  for (t = idle_scan_last; t; t = t->next)
    {
      if (PENDING_WRITE (t))
        acc += t->size;
    }

  if (acc != local_dirty)
    g_printerr ("\nDirty scan follower mismatch: running=%"G_GUINT64_FORMAT
                ", tested=%"G_GUINT64_FORMAT"\n",
                acc,local_dirty);
}
#endif
