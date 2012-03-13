/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptilebackendtilemanager.c
 * Copyright (C) 2011 Øyvind Kolås <pippin@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gegl-buffer.h>
#include <gegl-tile.h>

#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "base/tile.h"
#include "base/tile-manager.h"

#include "gimptilebackendtilemanager.h"
#include "gimp-gegl-utils.h"


struct _GimpTileBackendTileManagerPrivate
{
  TileManager *tile_manager;
  int          write;
};



static void     gimp_tile_backend_tile_manager_finalize (GObject         *object);
static void     gimp_tile_backend_tile_manager_dispose  (GObject         *object);
static gpointer gimp_tile_backend_tile_manager_command  (GeglTileSource  *tile_store,
                                                         GeglTileCommand  command,
                                                         gint             x,
                                                         gint             y,
                                                         gint             z,
                                                         gpointer         data);

static void       gimp_tile_write   (GimpTileBackendTileManager *ram,
                                     gint                        x,
                                     gint                        y,
                                     gint                        z,
                                     guchar                     *source);


G_DEFINE_TYPE (GimpTileBackendTileManager, gimp_tile_backend_tile_manager,
               GEGL_TYPE_TILE_BACKEND)

#define parent_class gimp_tile_backend_tile_manager_parent_class


static void
gimp_tile_backend_tile_manager_class_init (GimpTileBackendTileManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_tile_backend_tile_manager_finalize;
  object_class->dispose  = gimp_tile_backend_tile_manager_dispose;

  g_type_class_add_private (klass, sizeof (GimpTileBackendTileManagerPrivate));
}

static void
gimp_tile_backend_tile_manager_init (GimpTileBackendTileManager *backend)
{
  GeglTileSource *source = GEGL_TILE_SOURCE (backend);

  backend->priv = G_TYPE_INSTANCE_GET_PRIVATE (backend,
                                               GIMP_TYPE_TILE_BACKEND_TILE_MANAGER,
                                               GimpTileBackendTileManagerPrivate);

  source->command = gimp_tile_backend_tile_manager_command;
}

static void
gimp_tile_backend_tile_manager_dispose (GObject *object)
{
  GimpTileBackendTileManager *backend = GIMP_TILE_BACKEND_TILE_MANAGER (object);

  if (backend->priv->tile_manager)
    {
      tile_manager_unref (backend->priv->tile_manager);
      backend->priv->tile_manager = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tile_backend_tile_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
tile_done (Tile *tile,
           void *data)
{
  tile_release (data, FALSE);
}

static gpointer
gimp_tile_backend_tile_manager_command (GeglTileSource  *tile_store,
                                        GeglTileCommand  command,
                                        gint             x,
                                        gint             y,
                                        gint             z,
                                        gpointer         data)
{
  GimpTileBackendTileManager *backend_tm;
  GeglTileBackend            *backend;

  backend_tm = GIMP_TILE_BACKEND_TILE_MANAGER (tile_store);
  backend    = GEGL_TILE_BACKEND (tile_store);

  switch (command)
    {
    case GEGL_TILE_GET:
      {
        GeglTile *tile;
        gint      tile_size;
        Tile     *gimp_tile;
        gint      tile_stride;
        gint      gimp_tile_stride;
        int       row;

        gimp_tile = tile_manager_get_at (backend_tm->priv->tile_manager,
                                         x, y, TRUE, backend_tm->priv->write);
        if (!gimp_tile)
          return NULL;
        g_return_val_if_fail (gimp_tile != NULL, NULL);

        tile_size        = gegl_tile_backend_get_tile_size (backend);
        tile_stride      = TILE_WIDTH * tile_bpp (gimp_tile);
        gimp_tile_stride = tile_ewidth (gimp_tile) * tile_bpp (gimp_tile);

        if (tile_stride == gimp_tile_stride)
          {
            /* use the GimpTile directly as GEGL tile */
            tile = gegl_tile_new_bare ();
            gegl_tile_set_data_full (tile, tile_data_pointer (gimp_tile, 0, 0),
                                     tile_size, (void*)tile_done, gimp_tile);
          }
        else
          {
            /* create a copy of the tile */
            tile = gegl_tile_new (tile_size);
            for (row = 0; row < tile_eheight (gimp_tile); row++)
              {
                memcpy (gegl_tile_get_data (tile) + row * tile_stride,
                        tile_data_pointer (gimp_tile, 0, row),
                        gimp_tile_stride);
              }
            tile_release (gimp_tile, FALSE);
          }

        return tile;
      }

    case GEGL_TILE_SET:
      {
        GeglTile *tile  = data;
        if (backend_tm->priv->write == FALSE)
          return NULL;
        gimp_tile_write (backend_tm, x, y, z, gegl_tile_get_data (tile));
        gegl_tile_mark_as_stored (tile);
        return NULL;
      }

    case GEGL_TILE_IDLE:
    case GEGL_TILE_VOID:
    case GEGL_TILE_EXIST:
      return NULL;

    default:
      g_assert (command < GEGL_TILE_LAST_COMMAND && command >= 0);
    }

  return NULL;
}

static void
gimp_tile_write (GimpTileBackendTileManager *backend_tm,
                 gint                        x,
                 gint                        y,
                 gint                        z,
                 guchar                     *source)
{
  Tile *gimp_tile;
  gint  tile_stride;
  gint  gimp_tile_stride;
  int   row;

  gimp_tile = tile_manager_get_at (backend_tm->priv->tile_manager,
                                   x, y, TRUE, TRUE);

  if (!gimp_tile)
    return;

  if (source != tile_data_pointer (gimp_tile, 0, 0))
    {
      /* only copy when we are not 0 copy */
      tile_stride      = TILE_WIDTH * tile_bpp (gimp_tile);
      gimp_tile_stride = tile_ewidth (gimp_tile) * tile_bpp (gimp_tile);

      for (row = 0; row < tile_eheight (gimp_tile); row++)
        {
          memcpy (tile_data_pointer (gimp_tile, 0, row),
                  source + row * tile_stride,
                  gimp_tile_stride);
        }
    }

  tile_release (gimp_tile, FALSE);
}

GeglTileBackend *
gimp_tile_backend_tile_manager_new (TileManager *tm,
                                    gboolean     write)
{
  GeglTileBackend            *ret;
  GimpTileBackendTileManager *backend_tm;

  gint             width  = tile_manager_width (tm);
  gint             height = tile_manager_height (tm);
  gint             bpp    = tile_manager_bpp (tm);
  GeglRectangle    rect   = { 0, 0, width, height };

  ret = g_object_new (GIMP_TYPE_TILE_BACKEND_TILE_MANAGER,
                      "tile-width",  TILE_WIDTH,
                      "tile-height", TILE_HEIGHT,
                      "format",      gimp_bpp_to_babl_format (bpp, FALSE),
                      NULL);
  backend_tm = GIMP_TILE_BACKEND_TILE_MANAGER (ret);
  backend_tm->priv->write = write;

  backend_tm->priv->tile_manager = tile_manager_ref (tm);

  gegl_tile_backend_set_extent (ret, &rect);

  return ret;
}

GeglBuffer *
gimp_tile_manager_get_gegl_buffer (TileManager *tm,
                                   gboolean     write)
{
  GeglTileBackend *backend;
  GeglBuffer      *buffer;

  backend = gimp_tile_backend_tile_manager_new (tm, write);
  buffer = gegl_buffer_new_for_backend (NULL, backend);
  g_object_unref (backend);

  return buffer;
}
