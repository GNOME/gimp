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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#define GIMP_DISABLE_DEPRECATION_WARNINGS

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "gimp.h"
#include "gimptile.h"


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


/*  public functions  */

void
_gimp_tile_ref_nocache (GimpTile *tile,
                        gboolean  init)
{
  g_return_if_fail (tile != NULL);

  tile->ref_count++;

  if (tile->ref_count == 1)
    {
      if (init)
        {
          gimp_tile_get (tile);
          tile->dirty = FALSE;
        }
      else
        {
          tile->data = g_new (guchar, tile->ewidth * tile->eheight * tile->bpp);
        }
    }
}

void
_gimp_tile_unref (GimpTile *tile,
                  gboolean  dirty)
{
  g_return_if_fail (tile != NULL);
  g_return_if_fail (tile->ref_count > 0);

  tile->ref_count--;
  tile->dirty |= dirty;

  if (tile->ref_count == 0)
    {
      _gimp_tile_flush (tile);
      g_free (tile->data);
      tile->data = NULL;
    }
}

void
_gimp_tile_flush (GimpTile *tile)
{
  g_return_if_fail (tile != NULL);

  if (tile->data && tile->dirty)
    {
      gimp_tile_put (tile);
      tile->dirty = FALSE;
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
