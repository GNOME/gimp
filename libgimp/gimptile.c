/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimptile.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>

#include "gimp.h"
#include "gimpprotocol.h"
#include "gimpwire.h"


/*  This is the percentage of the maximum cache size that 
 *  should be cleared from the cache when an eviction is 
 *  necessary.
 */
#define FREE_QUANTUM 0.1

void         gimp_read_expect_msg   (WireMessage *msg,
				     gint         type);

static void  gimp_tile_get          (GimpTile    *tile);
static void  gimp_tile_put          (GimpTile    *tile);
static void  gimp_tile_cache_insert (GimpTile    *tile);
static void  gimp_tile_cache_flush  (GimpTile    *tile);
static void  gimp_tile_cache_zorch  (void);
static guint gimp_tile_hash         (GimpTile    *tile);


gint _gimp_tile_width  = -1;
gint _gimp_tile_height = -1;

static GHashTable *tile_hash_table = NULL;
static GList      *tile_list_head  = NULL;
static GList      *tile_list_tail  = NULL;
static gulong      max_tile_size   = 0;
static gulong      cur_cache_size  = 0;
static gulong      max_cache_size  = 0;


void
gimp_tile_ref (GimpTile *tile)
{
  if (tile)
    {
      tile->ref_count += 1;

      if (tile->ref_count == 1)
	{
	  gimp_tile_get (tile);
	  tile->dirty = FALSE;
	}

      gimp_tile_cache_insert (tile);
    }
}

void
gimp_tile_ref_zero (GimpTile *tile)
{
  if (tile)
    {
      tile->ref_count += 1;

      if (tile->ref_count == 1)
	{
	  tile->data = g_new (guchar, tile->ewidth * tile->eheight * tile->bpp);
	  memset (tile->data, 0, tile->ewidth * tile->eheight * tile->bpp);
	}

      gimp_tile_cache_insert (tile);
    }
}

void
gimp_tile_unref (GimpTile *tile,
		 gboolean  dirty)
{
  if (tile)
    {
      tile->ref_count -= 1;
      tile->dirty |= dirty;

      if (tile->ref_count == 0)
	{
	  gimp_tile_flush (tile);
	  g_free (tile->data);
	  tile->data = NULL;
	}
    }
}

void
gimp_tile_flush (GimpTile *tile)
{
  if (tile && tile->data && tile->dirty)
    {
      gimp_tile_put (tile);
      tile->dirty = FALSE;
    }
}

void
gimp_tile_cache_size (gulong kilobytes)
{
  max_cache_size = kilobytes * 1024;
}

void
gimp_tile_cache_ntiles (gulong ntiles)
{
  gimp_tile_cache_size ((gulong)(ntiles * _gimp_tile_width * _gimp_tile_height * 4 + 1023) / 1024);
}

guint
gimp_tile_width (void)
{
  return _gimp_tile_width;
}

guint
gimp_tile_height (void)
{
  return _gimp_tile_height;
}


static void
gimp_tile_get (GimpTile *tile)
{
  extern GIOChannel *_writechannel;
  extern guchar* _shm_addr;

  GPTileReq tile_req;
  GPTileData *tile_data;
  WireMessage msg;

  tile_req.drawable_ID = tile->drawable->id;
  tile_req.tile_num = tile->tile_num;
  tile_req.shadow = tile->shadow;
  if (!gp_tile_req_write (_writechannel, &tile_req))
    gimp_quit ();

  gimp_read_expect_msg(&msg,GP_TILE_DATA);

  tile_data = msg.data;
  if ((tile_data->drawable_ID != tile->drawable->id) ||
      (tile_data->tile_num != tile->tile_num) ||
      (tile_data->shadow != tile->shadow) ||
      (tile_data->width != tile->ewidth) ||
      (tile_data->height != tile->eheight) ||
      (tile_data->bpp != tile->bpp))
    {
      g_message ("received tile info did not match computed tile info\n");
      gimp_quit ();
    }

  if (tile_data->use_shm)
    {
      tile->data = g_new (guchar, tile->ewidth * tile->eheight * tile->bpp);
      memcpy (tile->data, _shm_addr, tile->ewidth * tile->eheight * tile->bpp);
    }
  else
    {
      tile->data = tile_data->data;
      tile_data->data = NULL;
    }

  if (!gp_tile_ack_write (_writechannel))
    gimp_quit ();

  wire_destroy (&msg);
}

static void
gimp_tile_put (GimpTile *tile)
{
  extern GIOChannel *_writechannel;
  extern guchar *_shm_addr;

  GPTileReq tile_req;
  GPTileData tile_data;
  GPTileData *tile_info;
  WireMessage msg;

  tile_req.drawable_ID = -1;
  tile_req.tile_num = 0;
  tile_req.shadow = 0;
  if (!gp_tile_req_write (_writechannel, &tile_req))
    gimp_quit ();

  gimp_read_expect_msg(&msg,GP_TILE_DATA);

  tile_info = msg.data;

  tile_data.drawable_ID = tile->drawable->id;
  tile_data.tile_num = tile->tile_num;
  tile_data.shadow = tile->shadow;
  tile_data.bpp = tile->bpp;
  tile_data.width = tile->ewidth;
  tile_data.height = tile->eheight;
  tile_data.use_shm = tile_info->use_shm;
  tile_data.data = NULL;

  if (tile_info->use_shm)
    memcpy (_shm_addr, tile->data, tile->ewidth * tile->eheight * tile->bpp);
  else
    tile_data.data = tile->data;

  if (!gp_tile_data_write (_writechannel, &tile_data))
    gimp_quit ();

  gimp_read_expect_msg(&msg,GP_TILE_ACK);

  wire_destroy (&msg);
}

/* This function is nearly identical to the function 'tile_cache_insert'
 *  in the file 'tile_cache.c' which is part of the main gimp application.
 */
static void
gimp_tile_cache_insert (GimpTile *tile)
{
  GList *tmp;

  if (!tile_hash_table)
    {
      tile_hash_table = g_hash_table_new ((GHashFunc) gimp_tile_hash, NULL);
      max_tile_size = gimp_tile_width () * gimp_tile_height () * 4;
    }

  /* First check and see if the tile is already
   *  in the cache. In that case we will simply place
   *  it at the end of the tile list to indicate that
   *  it was the most recently accessed tile.
   */
  tmp = g_hash_table_lookup (tile_hash_table, tile);

  if (tmp)
    {
      /* The tile was already in the cache. Place it at
       *  the end of the tile list.
       */
      if (tmp == tile_list_tail)
	tile_list_tail = tile_list_tail->prev;
      tile_list_head = g_list_remove_link (tile_list_head, tmp);
      if (!tile_list_head)
	tile_list_tail = NULL;
      g_list_free (tmp);

      /* Remove the old reference to the tiles list node
       *  in the tile hash table.
       */
      g_hash_table_remove (tile_hash_table, tile);

      tile_list_tail = g_list_append (tile_list_tail, tile);
      if (!tile_list_head)
	tile_list_head = tile_list_tail;
      tile_list_tail = g_list_last (tile_list_tail);

      /* Add the tiles list node to the tile hash table. The
       *  list node is indexed by the tile itself. This makes
       *  for a quick lookup of which list node the tile is in.
       */
      g_hash_table_insert (tile_hash_table, tile, tile_list_tail);
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
	  while (tile_list_head && (cur_cache_size + max_cache_size * FREE_QUANTUM) > max_cache_size)
	    gimp_tile_cache_zorch ();

	  if ((cur_cache_size + max_tile_size) > max_cache_size)
	    return;
	}

      /* Place the tile at the end of the tile list.
       */
      tile_list_tail = g_list_append (tile_list_tail, tile);
      if (!tile_list_head)
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
      tile->ref_count += 1;
      if (tile->ref_count == 1)
	{
	  gimp_tile_get (tile);
	  tile->dirty = FALSE;
	}
    }
}

static void
gimp_tile_cache_flush (GimpTile *tile)
{
  GList *tmp;

  if (!tile_hash_table)
    {
      tile_hash_table = g_hash_table_new ((GHashFunc) gimp_tile_hash, NULL);
      max_tile_size = gimp_tile_width () * gimp_tile_height () * 4;
    }

  /* Find where the tile is in the cache.
   */
  tmp = g_hash_table_lookup (tile_hash_table, tile);

  if (tmp)
    {
      /* If the tile is in the cache, then remove it from the
       *  tile list.
       */
      if (tmp == tile_list_tail)
        tile_list_tail = tile_list_tail->prev;
      tile_list_head = g_list_remove_link (tile_list_head, tmp);
      if (!tile_list_head)
        tile_list_tail = NULL;
      g_list_free (tmp);

      /* Remove the tile from the tile hash table.
       */
      g_hash_table_remove (tile_hash_table, tile);

      /* Note the decrease in the number of bytes the cache
       *  is referencing.
       */
      cur_cache_size -= max_tile_size;

      /* Unreference the tile.
       */
      gimp_tile_unref (tile, FALSE);
    }
}

static void
gimp_tile_cache_zorch (void)
{
  if (tile_list_head)
    gimp_tile_cache_flush ((GimpTile*) tile_list_head->data);
}

static guint
gimp_tile_hash (GimpTile *tile)
{
  return (gulong) tile;
}
