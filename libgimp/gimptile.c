/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimptile.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#define GIMP_DISABLE_DEPRECATION_WARNINGS

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "gimp.h"


/**
 * SECTION: gimptile
 * @title: gimptile
 * @short_description: Functions for working with tiles.
 *
 * Functions for working with tiles.
 **/


/*  This is the percentage of the maximum cache size that
 *  should be cleared from the cache when an eviction is
 *  necessary.
 */
#define FREE_QUANTUM 0.1


void         gimp_read_expect_msg   (GimpWireMessage *msg,
                                     gint             type);

static void  gimp_tile_get          (GimpTile        *tile);
static void  gimp_tile_put          (GimpTile        *tile);
static void  gimp_tile_cache_insert (GimpTile        *tile);
static void  gimp_tile_cache_flush  (GimpTile        *tile);


/*  private variables  */

static GHashTable * tile_hash_table = NULL;
static GList      * tile_list_head  = NULL;
static GList      * tile_list_tail  = NULL;
static gulong       max_tile_size   = 0;
static gulong       cur_cache_size  = 0;
static gulong       max_cache_size  = 0;


/*  public functions  */

void
gimp_tile_ref (GimpTile *tile)
{
  g_return_if_fail (tile != NULL);

  tile->ref_count++;

  if (tile->ref_count == 1)
    {
      gimp_tile_get (tile);
      tile->dirty = FALSE;
    }

  gimp_tile_cache_insert (tile);
}

void
gimp_tile_unref (GimpTile *tile,
                 gboolean  dirty)
{
  g_return_if_fail (tile != NULL);
  g_return_if_fail (tile->ref_count > 0);

  tile->ref_count--;
  tile->dirty |= dirty;

  if (tile->ref_count == 0)
    {
      gimp_tile_flush (tile);
      g_free (tile->data);
      tile->data = NULL;
    }
}

void
gimp_tile_flush (GimpTile *tile)
{
  g_return_if_fail (tile != NULL);

  if (tile->data && tile->dirty)
    {
      gimp_tile_put (tile);
      tile->dirty = FALSE;
    }
}

/**
 * gimp_tile_cache_size:
 * @kilobytes: new cache size in kilobytes
 *
 * Sets the size of the tile cache on the plug-in side. The tile cache
 * is used to reduce the number of tiles exchanged between the GIMP core
 * and the plug-in. See also gimp_tile_cache_ntiles().
 **/
void
gimp_tile_cache_size (gulong kilobytes)
{
  max_cache_size = kilobytes * 1024;
}

/**
 * gimp_tile_cache_ntiles:
 * @ntiles: number of tiles that should fit into the cache
 *
 * Sets the size of the tile cache on the plug-in side. This function
 * is similar to gimp_tile_cache_size() but supports specifying the
 * number of tiles directly.
 *
 * If your plug-in access pixels tile-by-tile, it doesn't need a tile
 * cache at all. If however the plug-in accesses drawable pixel data
 * row-by-row, it should set the tile cache large enough to hold the
 * number of tiles per row. Double this size if your plug-in uses
 * shadow tiles.
 **/
void
gimp_tile_cache_ntiles (gulong ntiles)
{
  gimp_tile_cache_size ((ntiles *
                         gimp_tile_width () *
                         gimp_tile_height () * 4 + 1023) / 1024);
}

void
_gimp_tile_cache_flush_drawable (GimpDrawable *drawable)
{
  GList *list;

  g_return_if_fail (drawable != NULL);

  list = tile_list_head;
  while (list)
    {
      GimpTile *tile = list->data;

      list = list->next;

      if (tile->drawable == drawable)
        gimp_tile_cache_flush (tile);
    }
}


/*  private functions  */

static void
gimp_tile_get (GimpTile *tile)
{
  extern GIOChannel *_writechannel;

  GPTileReq        tile_req;
  GPTileData      *tile_data;
  GimpWireMessage  msg;

  tile_req.drawable_ID = tile->drawable->drawable_id;
  tile_req.tile_num    = tile->tile_num;
  tile_req.shadow      = tile->shadow;

  gp_lock ();
  if (! gp_tile_req_write (_writechannel, &tile_req, NULL))
    gimp_quit ();

  gimp_read_expect_msg (&msg, GP_TILE_DATA);

  tile_data = msg.data;
  if (tile_data->drawable_ID != tile->drawable->drawable_id ||
      tile_data->tile_num    != tile->tile_num              ||
      tile_data->shadow      != tile->shadow                ||
      tile_data->width       != tile->ewidth                ||
      tile_data->height      != tile->eheight               ||
      tile_data->bpp         != tile->bpp)
    {
      g_message ("received tile info did not match computed tile info");
      gimp_quit ();
    }

  if (tile_data->use_shm)
    {
      tile->data = g_memdup (gimp_shm_addr (),
                             tile->ewidth * tile->eheight * tile->bpp);
    }
  else
    {
      tile->data = tile_data->data;
      tile_data->data = NULL;
    }

  if (! gp_tile_ack_write (_writechannel, NULL))
    gimp_quit ();
  gp_unlock ();

  gimp_wire_destroy (&msg);
}

static void
gimp_tile_put (GimpTile *tile)
{
  extern GIOChannel *_writechannel;

  GPTileReq        tile_req;
  GPTileData       tile_data;
  GPTileData      *tile_info;
  GimpWireMessage  msg;

  tile_req.drawable_ID = -1;
  tile_req.tile_num    = 0;
  tile_req.shadow      = 0;

  gp_lock ();
  if (! gp_tile_req_write (_writechannel, &tile_req, NULL))
    gimp_quit ();

  gimp_read_expect_msg (&msg, GP_TILE_DATA);

  tile_info = msg.data;

  tile_data.drawable_ID = tile->drawable->drawable_id;
  tile_data.tile_num    = tile->tile_num;
  tile_data.shadow      = tile->shadow;
  tile_data.bpp         = tile->bpp;
  tile_data.width       = tile->ewidth;
  tile_data.height      = tile->eheight;
  tile_data.use_shm     = tile_info->use_shm;
  tile_data.data        = NULL;

  if (tile_info->use_shm)
    memcpy (gimp_shm_addr (),
            tile->data,
            tile->ewidth * tile->eheight * tile->bpp);
  else
    tile_data.data = tile->data;

  if (! gp_tile_data_write (_writechannel, &tile_data, NULL))
    gimp_quit ();

  if (! tile_info->use_shm)
    tile_data.data = NULL;

  gimp_wire_destroy (&msg);

  gimp_read_expect_msg (&msg, GP_TILE_ACK);
  gp_unlock ();
  gimp_wire_destroy (&msg);
}

/* This function is nearly identical to the function 'tile_cache_insert'
 *  in the file 'tile_cache.c' which is part of the main gimp application.
 */
static void
gimp_tile_cache_insert (GimpTile *tile)
{
  GList *list;

  if (!tile_hash_table)
    {
      tile_hash_table = g_hash_table_new (g_direct_hash, NULL);
      max_tile_size = gimp_tile_width () * gimp_tile_height () * 4;
    }

  /* First check and see if the tile is already
   *  in the cache. In that case we will simply place
   *  it at the end of the tile list to indicate that
   *  it was the most recently accessed tile.
   */
  list = g_hash_table_lookup (tile_hash_table, tile);

  if (list)
    {
      /* The tile was already in the cache. Place it at
       *  the end of the tile list.
       */

      /* If the tile is already at the end of the list, we are done */
      if (list == tile_list_tail)
        return;

      /* At this point we have at least two elements in our list */
      g_assert (tile_list_head != tile_list_tail);

      tile_list_head = g_list_remove_link (tile_list_head, list);

      tile_list_tail = g_list_last (g_list_concat (tile_list_tail, list));
    }
  else
    {
      /* The tile was not in the cache. First check and see
       *  if there is room in the cache. If not then we'll have
       *  to make room first. Note: it might be the case that the
       *  cache is smaller than the size of a tile in which case
       *  it won't be possible to put it in the cache.
       */

      if ((cur_cache_size + max_tile_size) > max_cache_size)
        {
          while (tile_list_head &&
                 (cur_cache_size +
                  max_cache_size * FREE_QUANTUM) > max_cache_size)
            {
              gimp_tile_cache_flush ((GimpTile *) tile_list_head->data);
            }

          if ((cur_cache_size + max_tile_size) > max_cache_size)
            return;
        }

      /* Place the tile at the end of the tile list.
       */
      tile_list_tail = g_list_append (tile_list_tail, tile);

      if (! tile_list_head)
        tile_list_head = tile_list_tail;

      tile_list_tail = g_list_last (tile_list_tail);

      /* Add the tiles list node to the tile hash table.
       */
      g_hash_table_insert (tile_hash_table, tile, tile_list_tail);

      /* Note the increase in the number of bytes the cache
       *  is referencing.
       */
      cur_cache_size += max_tile_size;

      /* Reference the tile so that it won't be returned to
       *  the main gimp application immediately.
       */
      tile->ref_count++;
    }
}

static void
gimp_tile_cache_flush (GimpTile *tile)
{
  GList *list;

  if (! tile_hash_table)
    return;

  /* Find where the tile is in the cache.
   */
  list = g_hash_table_lookup (tile_hash_table, tile);

  if (list)
    {
      /* If the tile is in the cache, then remove it from the
       *  tile list.
       */
      if (list == tile_list_tail)
        tile_list_tail = tile_list_tail->prev;

      tile_list_head = g_list_remove_link (tile_list_head, list);

      if (! tile_list_head)
        tile_list_tail = NULL;

      /* Remove the tile from the tile hash table.
       */
      g_hash_table_remove (tile_hash_table, tile);
      g_list_free (list);

      /* Note the decrease in the number of bytes the cache
       *  is referencing.
       */
      cur_cache_size -= max_tile_size;

      /* Unreference the tile.
       */
      gimp_tile_unref (tile, FALSE);
    }
}
