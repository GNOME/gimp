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
#include "tile-private.h"
#include "tile-cache.h"
#include "tile-manager.h"
#include "tile-swap.h"


/*  Uncomment for verbose debugging on copy-on-write logic  */
/*  #define TILE_DEBUG  */

/*  Uncomment to enable global counters to profile the tile system. */
/*  #define TILE_PROFILING */

/*  Sanity checks on tile hinting code  */
/*  #define HINTS_SANITY */


/*  This is being used from tile-swap, but just for debugging purposes.  */
static gint tile_ref_count    = 0;


#ifdef TILE_PROFILING

static gint tile_count        = 0;
static gint tile_share_count  = 0;
static gint tile_active_count = 0;

#ifdef HINTS_SANITY
static gint tile_exist_peak   = 0;
static gint tile_exist_count  = 0;
#endif

#endif


static void tile_destroy (Tile *tile);


void
tile_allocate_rowhints (Tile *tile)
{
  if (! tile->rowhint)
    tile->rowhint = g_slice_alloc0 (sizeof (TileRowHint) * TILE_HEIGHT);
}

TileRowHint
tile_get_rowhint (Tile *tile,
                  gint  yoff)
{
#ifdef HINTS_SANITY
  if (yoff < tile_eheight(tile) && yoff >= 0)
    {
      return tile->rowhint[yoff];
    }
  else
    {
      g_error ("GET_ROWHINT OUT OF RANGE");
    }

  return TILEROWHINT_OUTOFRANGE;
#else
  return tile->rowhint[yoff];
#endif
}

void
tile_set_rowhint (Tile        *tile,
                  gint         yoff,
                  TileRowHint  rowhint)
{
#ifdef HINTS_SANITY
  if (yoff < tile_eheight(tile) && yoff >= 0)
    {
      tile->rowhint[yoff] = rowhint;
    }
  else
    {
      g_error("SET_ROWHINT OUT OF RANGE");
    }
#else

  tile->rowhint[yoff] = rowhint;
#endif
}

Tile *
tile_new (gint bpp)
{
  Tile *tile = g_slice_new0 (Tile);

  tile->ewidth      = TILE_WIDTH;
  tile->eheight     = TILE_HEIGHT;
  tile->bpp         = bpp;
  tile->swap_offset = -1;

#ifdef TILE_PROFILING
  tile_count++;
#endif

  return tile;
}

void
tile_lock (Tile *tile)
{
  /* Increment the global reference count.
   */
  tile_ref_count++;

  /* Increment this tile's reference count.
   */
  tile->ref_count++;

  if (tile->ref_count == 1)
    {
      if (tile->listhead)
        {
          /* remove from cache, move to main store */
          tile_cache_flush (tile);
        }

#ifdef TILE_PROFILING
      tile_active_count++;
#endif
    }

  if (tile->data == NULL)
    {
      /* There is no data, so the tile must be swapped out */
      tile_swap_in (tile);
    }

  /* Call 'tile_manager_validate' if the tile was invalid.
   */
  if (! tile->valid)
    {
      /* an invalid tile should never be shared, so this should work */
      tile_manager_validate (tile->tlink->tm, tile);
    }
}

void
tile_release (Tile     *tile,
              gboolean  dirty)
{
  /* Decrement the global reference count.
   */
  tile_ref_count--;

  /* Decrement this tile's reference count.
   */
  tile->ref_count--;

  /* Decrement write ref count if dirtying
   */
  if (dirty)
    {
      gint y;

      tile->write_count--;

      if (tile->rowhint)
        {
          for (y = 0; y < tile->eheight; y++)
            tile->rowhint[y] = TILEROWHINT_UNKNOWN;
        }
    }

  if (tile->ref_count == 0)
    {
#ifdef TILE_PROFILING
      tile_active_count--;
#endif

      if (tile->share_count == 0)
        {
          /* tile is truly dead */
          tile_destroy (tile);
          return;                        /* skip terminal unlock */
        }
      else
        {
          /* last reference was just released, so move the tile to the
             tile cache */
          tile_cache_insert (tile);
        }
    }
}

void
tile_alloc (Tile *tile)
{
  if (tile->data)
    return;

  /* Allocate the data for the tile.
   */
  tile->data = g_new (guchar, tile->size);

#ifdef TILE_PROFILING
#ifdef HINTS_SANITY
  tile_exist_count++;
  if (tile_exist_count > tile_exist_peak)
    tile_exist_peak = tile_exist_count;
#endif
#endif
}

static void
tile_destroy (Tile *tile)
{
  if (G_UNLIKELY (tile->ref_count))
    {
      g_warning ("tried to destroy a ref'd tile");
      return;
    }
  if (G_UNLIKELY (tile->share_count))
    {
      g_warning ("tried to destroy an attached tile");
      return;
    }
  if (tile->data)
    {
      g_free (tile->data);
      tile->data = NULL;
    }
  if (tile->rowhint)
    {
      g_slice_free1 (sizeof (TileRowHint) * TILE_HEIGHT, tile->rowhint);
      tile->rowhint = NULL;
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

  g_slice_free (Tile, tile);

#ifdef TILE_PROFILING
  tile_count--;

#ifdef HINTS_SANITY
  tile_exist_count--;
#endif

#endif
}

gint
tile_size (Tile *tile)
{
  /* Return the actual size of the tile data.
   *  (Based on its effective width and height).
   */
  return tile->size;
}

gint
tile_ewidth (Tile *tile)
{
  return tile->ewidth;
}

gint
tile_eheight (Tile *tile)
{
  return tile->eheight;
}

gint
tile_bpp (Tile *tile)
{
  return tile->bpp;
}

gboolean
tile_is_valid (Tile *tile)
{
  return tile->valid;
}

void
tile_attach (Tile *tile,
             void *tm,
             gint  tile_num)
{
  TileLink *tmp;

  if ((tile->share_count > 0) && (! tile->valid))
    {
      /* trying to share invalid tiles is problematic, not to mention silly */
      tile_manager_validate (tile->tlink->tm, tile);
    }

  tile->share_count++;

#ifdef TILE_PROFILING
  tile_share_count++;
#endif

#ifdef TILE_DEBUG
  g_printerr ("tile_attach: %p -> (%p,%d) *%d\n",
              tile, tm, tile_num, tile->share_count);
#endif

  /* link this tile into the tile's tilelink chain */
  tmp = g_slice_new (TileLink);

  tmp->tm       = tm;
  tmp->tile_num = tile_num;
  tmp->next     = tile->tlink;

  tile->tlink = tmp;
}

void
tile_detach (Tile *tile,
             void *tm,
             gint  tile_num)
{
  TileLink **link;
  TileLink  *tmp;

#ifdef TILE_DEBUG
  g_printerr ("tile_detach: %p ~> (%p,%d) r%d *%d\n",
              tile, tm, tile_num, tile->ref_count, tile->share_count);
#endif

  for (link = &tile->tlink;
       *link != NULL;
       link = &(*link)->next)
    {
      if (((*link)->tm == tm) && ((*link)->tile_num == tile_num))
        break;
    }

  if (G_UNLIKELY (*link == NULL))
    {
      g_warning ("Tried to detach a nonattached tile -- TILE BUG!");
      return;
    }

  tmp = *link;
  *link = tmp->next;

  g_slice_free (TileLink, tmp);

#ifdef TILE_PROFILING
  tile_share_count--;
#endif

  tile->share_count--;

  if (tile->share_count == 0 && tile->ref_count == 0)
    tile_destroy (tile);
}

gpointer
tile_data_pointer (Tile *tile,
                   gint  xoff,
                   gint  yoff)
{
  gsize offset = yoff * tile->ewidth + xoff;

  return (gpointer) (tile->data + offset * tile->bpp);
}

gint
tile_global_refcount (void)
{
  return tile_ref_count;
}
