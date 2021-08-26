/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptilebackendplugin.c
 * Copyright (C) 2011-2019 Øyvind Kolås <pippin@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "gimp.h"

#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "gimp-shm.h"
#include "gimpplugin-private.h"
#include "gimptilebackendplugin.h"


#define TILE_WIDTH  gimp_tile_width()
#define TILE_HEIGHT gimp_tile_height()


typedef struct _GimpTile GimpTile;

struct _GimpTile
{
  guint   tile_num; /* the number of this tile within the drawable */

  guint   ewidth;   /* the effective width of the tile */
  guint   eheight;  /* the effective height of the tile */

  guchar *data;     /* the pixel data for the tile */
};


struct _GimpTileBackendPluginPrivate
{
  gint32   drawable_id;
  gboolean shadow;
  gint     width;
  gint     height;
  gint     bpp;
  gint     ntile_rows;
  gint     ntile_cols;
};


static gpointer   gimp_tile_backend_plugin_command (GeglTileSource  *tile_store,
                                                    GeglTileCommand  command,
                                                    gint             x,
                                                    gint             y,
                                                    gint             z,
                                                    gpointer         data);

static gboolean   gimp_tile_write (GimpTileBackendPlugin *backend_plugin,
                                   gint                   x,
                                   gint                   y,
                                   GeglTile              *tile);
static GeglTile * gimp_tile_read  (GimpTileBackendPlugin *backend_plugin,
                                   gint                   x,
                                   gint                   y);

static gboolean   gimp_tile_init  (GimpTileBackendPlugin *backend_plugin,
                                   GimpTile              *tile,
                                   gint                   row,
                                   gint                   col);
static void       gimp_tile_unset (GimpTileBackendPlugin *backend_plugin,
                                   GimpTile              *tile);
static void       gimp_tile_get   (GimpTileBackendPlugin *backend_plugin,
                                   GimpTile              *tile);
static void       gimp_tile_put   (GimpTileBackendPlugin *backend_plugin,
                                   GimpTile              *tile);


G_DEFINE_TYPE_WITH_PRIVATE (GimpTileBackendPlugin, _gimp_tile_backend_plugin,
                            GEGL_TYPE_TILE_BACKEND)

#define parent_class _gimp_tile_backend_plugin_parent_class


static GMutex backend_plugin_mutex;


static void
_gimp_tile_backend_plugin_class_init (GimpTileBackendPluginClass *klass)
{
}

static void
_gimp_tile_backend_plugin_init (GimpTileBackendPlugin *backend)
{
  GeglTileSource *source = GEGL_TILE_SOURCE (backend);

  backend->priv = _gimp_tile_backend_plugin_get_instance_private (backend);

  source->command = gimp_tile_backend_plugin_command;
}

static gpointer
gimp_tile_backend_plugin_command (GeglTileSource  *tile_store,
                                  GeglTileCommand  command,
                                  gint             x,
                                  gint             y,
                                  gint             z,
                                  gpointer         data)
{
  GimpTileBackendPlugin *backend_plugin = GIMP_TILE_BACKEND_PLUGIN (tile_store);
  gpointer               result         = NULL;

  switch (command)
    {
    case GEGL_TILE_GET:
      /* TODO: fetch mipmapped tiles directly from gimp, instead of returning
       * NULL to render them locally
       */
      if (z == 0)
        {
          g_mutex_lock (&backend_plugin_mutex);

          result = gimp_tile_read (backend_plugin, x, y);

          g_mutex_unlock (&backend_plugin_mutex);
        }
      break;

    case GEGL_TILE_SET:
      /* TODO: actually store mipmapped tiles */
      if (z == 0)
        {
          g_mutex_lock (&backend_plugin_mutex);

          gimp_tile_write (backend_plugin, x, y, data);

          g_mutex_unlock (&backend_plugin_mutex);
        }

      gegl_tile_mark_as_stored (data);
      break;

    case GEGL_TILE_FLUSH:
      break;

    default:
      result = gegl_tile_backend_command (GEGL_TILE_BACKEND (tile_store),
                                          command, x, y, z, data);
      break;
    }

  return result;
}


/*  public functions  */

GeglTileBackend *
_gimp_tile_backend_plugin_new (GimpDrawable *drawable,
                               gint          shadow)
{
  GeglTileBackend       *backend;
  GimpTileBackendPlugin *backend_plugin;
  const Babl            *format = gimp_drawable_get_format (drawable);
  gint                   width  = gimp_drawable_get_width  (drawable);
  gint                   height = gimp_drawable_get_height (drawable);

  backend = g_object_new (GIMP_TYPE_TILE_BACKEND_PLUGIN,
                          "tile-width",  TILE_WIDTH,
                          "tile-height", TILE_HEIGHT,
                          "format",      format,
                          NULL);

  backend_plugin = GIMP_TILE_BACKEND_PLUGIN (backend);

  backend_plugin->priv->drawable_id = gimp_item_get_id (GIMP_ITEM (drawable));
  backend_plugin->priv->shadow      = shadow;
  backend_plugin->priv->width       = width;
  backend_plugin->priv->height      = height;
  backend_plugin->priv->bpp         = gimp_drawable_get_bpp (drawable);
  backend_plugin->priv->ntile_rows  = (height + TILE_HEIGHT - 1) / TILE_HEIGHT;
  backend_plugin->priv->ntile_cols  = (width  + TILE_WIDTH  - 1) / TILE_WIDTH;

  gegl_tile_backend_set_extent (backend,
                                GEGL_RECTANGLE (0, 0, width, height));

  return backend;
}


/*  private functions  */

static GeglTile *
gimp_tile_read (GimpTileBackendPlugin *backend_plugin,
                gint                   x,
                gint                   y)
{
  GimpTileBackendPluginPrivate *priv    = backend_plugin->priv;
  GeglTileBackend              *backend = GEGL_TILE_BACKEND (backend_plugin);
  GeglTile                     *tile;
  GimpTile                      gimp_tile = { 0, };
  gint                          tile_size;
  guchar                       *tile_data;

  if (! gimp_tile_init (backend_plugin, &gimp_tile, y, x))
    return NULL;

  tile_size  = gegl_tile_backend_get_tile_size (backend);
  tile       = gegl_tile_new (tile_size);
  tile_data  = gegl_tile_get_data (tile);

  gimp_tile_get (backend_plugin, &gimp_tile);

  if (gimp_tile.ewidth * gimp_tile.eheight * priv->bpp == tile_size)
    {
      memcpy (tile_data, gimp_tile.data, tile_size);
    }
  else
    {
      gint  tile_stride      = TILE_WIDTH * priv->bpp;
      gint  gimp_tile_stride = gimp_tile.ewidth * priv->bpp;
      guint row;

      for (row = 0; row < gimp_tile.eheight; row++)
        {
          memcpy (tile_data      + row * tile_stride,
                  gimp_tile.data + row * gimp_tile_stride,
                  gimp_tile_stride);
        }
    }

  gimp_tile_unset (backend_plugin, &gimp_tile);

  return tile;
}

static gboolean
gimp_tile_write (GimpTileBackendPlugin *backend_plugin,
                 gint                   x,
                 gint                   y,
                 GeglTile              *tile)
{
  GimpTileBackendPluginPrivate *priv      = backend_plugin->priv;
  GeglTileBackend              *backend   = GEGL_TILE_BACKEND (backend_plugin);
  GimpTile                      gimp_tile = { 0, };
  gint                          tile_size;
  guchar                       *tile_data;

  if (! gimp_tile_init (backend_plugin, &gimp_tile, y, x))
    return FALSE;

  tile_size = gegl_tile_backend_get_tile_size (backend);
  tile_data = gegl_tile_get_data (tile);

  gimp_tile.data = g_new (guchar,
                          gimp_tile.ewidth * gimp_tile.eheight *
                          priv->bpp);

  if (gimp_tile.ewidth * gimp_tile.eheight * priv->bpp == tile_size)
    {
      memcpy (gimp_tile.data, tile_data, tile_size);
    }
  else
    {
      gint  tile_stride      = TILE_WIDTH * priv->bpp;
      gint  gimp_tile_stride = gimp_tile.ewidth * priv->bpp;
      guint row;

      for (row = 0; row < gimp_tile.eheight; row++)
        {
          memcpy (gimp_tile.data + row * gimp_tile_stride,
                  tile_data      + row * tile_stride,
                  gimp_tile_stride);
        }
    }

  gimp_tile_put (backend_plugin, &gimp_tile);
  gimp_tile_unset (backend_plugin, &gimp_tile);

  return TRUE;
}

static gboolean
gimp_tile_init (GimpTileBackendPlugin *backend_plugin,
                GimpTile              *tile,
                gint                   row,
                gint                   col)
{
  GimpTileBackendPluginPrivate *priv = backend_plugin->priv;

  if (row > priv->ntile_rows - 1 ||
      col > priv->ntile_cols - 1)
    {
      return FALSE;
    }

  tile->tile_num = row * priv->ntile_cols + col;

  if (col == (priv->ntile_cols - 1))
    tile->ewidth  = priv->width  - ((priv->ntile_cols - 1) * TILE_WIDTH);
  else
    tile->ewidth  = TILE_WIDTH;

  if (row == (priv->ntile_rows - 1))
    tile->eheight = priv->height - ((priv->ntile_rows - 1) * TILE_HEIGHT);
  else
    tile->eheight = TILE_HEIGHT;

  tile->data = NULL;

  return TRUE;
}

static void
gimp_tile_unset (GimpTileBackendPlugin *backend_plugin,
                 GimpTile              *tile)
{
  g_clear_pointer (&tile->data, g_free);
}

static void
gimp_tile_get (GimpTileBackendPlugin *backend_plugin,
               GimpTile              *tile)
{
  GimpTileBackendPluginPrivate *priv    = backend_plugin->priv;
  GimpPlugIn                   *plug_in = gimp_get_plug_in ();
  GPTileReq                     tile_req;
  GPTileData                   *tile_data;
  GimpWireMessage               msg;

  tile_req.drawable_id = priv->drawable_id;
  tile_req.tile_num    = tile->tile_num;
  tile_req.shadow      = priv->shadow;

  if (! gp_tile_req_write (_gimp_plug_in_get_write_channel (plug_in),
                           &tile_req, plug_in))
    gimp_quit ();

  _gimp_plug_in_read_expect_msg (plug_in, &msg, GP_TILE_DATA);

  tile_data = msg.data;
  if (tile_data->drawable_id != priv->drawable_id ||
      tile_data->tile_num    != tile->tile_num    ||
      tile_data->shadow      != priv->shadow      ||
      tile_data->width       != tile->ewidth      ||
      tile_data->height      != tile->eheight     ||
      tile_data->bpp         != priv->bpp)
    {
#if 0
      g_printerr ("tile_data: %d %d %d %d %d %d\n"
                  "tile:      %d %d %d %d %d %d\n",
                  tile_data->drawable_id,
                  tile_data->tile_num,
                  tile_data->shadow,
                  tile_data->width,
                  tile_data->height,
                  tile_data->bpp,
                  priv->drawable_id,
                  tile->tile_num,
                  priv->shadow,
                  tile->ewidth,
                  tile->eheight,
                  priv->bpp);
#endif
      g_printerr ("received tile info did not match computed tile info");
      gimp_quit ();
    }

  if (tile_data->use_shm)
    {
      tile->data = g_memdup2 (_gimp_shm_addr (),
                              tile->ewidth * tile->eheight * priv->bpp);
    }
  else
    {
      tile->data = tile_data->data;
      tile_data->data = NULL;
    }

  if (! gp_tile_ack_write (_gimp_plug_in_get_write_channel (plug_in),
                           plug_in))
    gimp_quit ();

  gimp_wire_destroy (&msg);
}

static void
gimp_tile_put (GimpTileBackendPlugin *backend_plugin,
               GimpTile              *tile)
{
  GimpTileBackendPluginPrivate *priv    = backend_plugin->priv;
  GimpPlugIn                   *plug_in = gimp_get_plug_in ();
  GPTileReq                     tile_req;
  GPTileData                    tile_data;
  GPTileData                   *tile_info;
  GimpWireMessage               msg;

  tile_req.drawable_id = -1;
  tile_req.tile_num    = 0;
  tile_req.shadow      = 0;

  if (! gp_tile_req_write (_gimp_plug_in_get_write_channel (plug_in),
                           &tile_req, plug_in))
    gimp_quit ();

  _gimp_plug_in_read_expect_msg (plug_in, &msg, GP_TILE_DATA);

  tile_info = msg.data;

  tile_data.drawable_id = priv->drawable_id;
  tile_data.tile_num    = tile->tile_num;
  tile_data.shadow      = priv->shadow;
  tile_data.bpp         = priv->bpp;
  tile_data.width       = tile->ewidth;
  tile_data.height      = tile->eheight;
  tile_data.use_shm     = tile_info->use_shm;
  tile_data.data        = NULL;

  if (tile_info->use_shm)
    {
      memcpy (_gimp_shm_addr (),
              tile->data,
              tile->ewidth * tile->eheight * priv->bpp);
    }
  else
    {
      tile_data.data = tile->data;
    }

  if (! gp_tile_data_write (_gimp_plug_in_get_write_channel (plug_in),
                            &tile_data, plug_in))
    gimp_quit ();

  if (! tile_info->use_shm)
    tile_data.data = NULL;

  gimp_wire_destroy (&msg);

  _gimp_plug_in_read_expect_msg (plug_in, &msg, GP_TILE_ACK);

  gimp_wire_destroy (&msg);
}
