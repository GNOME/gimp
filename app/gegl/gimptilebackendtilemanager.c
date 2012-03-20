/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptilebackendtilemanager.c
 * Copyright (C) 2011,2012 Øyvind Kolås <pippin@gimp.org>
 *               2011      Martin Nordholts
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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gegl-buffer.h>
#include <gegl-tile.h>

#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "base/tile.h"
#include "base/tile-manager.h"
#include "base/tile-manager-private.h"

#include "gimptilebackendtilemanager.h"
#include "gimp-gegl-utils.h"


struct _GimpTileBackendTileManagerPrivate
{
  TileManager *tile_manager;
  int          mul;
};

static int gimp_gegl_tile_mul (void)
{
  static int mul = 8;
  static gboolean inited = 0;
  if (G_LIKELY (inited))
    return mul;
  inited = 1;
  if (g_getenv ("GIMP_GEGL_TILE_MUL"))
    mul = atoi (g_getenv ("GIMP_GEGL_TILE_MUL"));
  if (mul < 1)
    mul = 1;
  return mul;
}

static void     gimp_tile_backend_tile_manager_finalize (GObject         *object);
static void     gimp_tile_backend_tile_manager_dispose  (GObject         *object);
static gpointer gimp_tile_backend_tile_manager_command  (GeglTileSource  *tile_store,
                                                         GeglTileCommand  command,
                                                         gint             x,
                                                         gint             y,
                                                         gint             z,
                                                         gpointer         data);

static void       gimp_tile_write   (GimpTileBackendTileManager *backend_tm,
                                     gint                        x,
                                     gint                        y,
                                     guchar                     *source);

static GeglTile * gimp_tile_read (GimpTileBackendTileManager *backend_tm,
                                  gint                        x,
                                  gint                        y);


static void       gimp_tile_write_mul (GimpTileBackendTileManager *backend_tm,
                                       gint                        x,
                                       gint                        y,
                                       guchar                     *source);

static GeglTile * gimp_tile_read_mul (GimpTileBackendTileManager *backend_tm,
                                      gint                        x,
                                      gint                        y);

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
tile_done_writing (void *data)
{
  tile_release (data, TRUE);
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

  backend_tm = GIMP_TILE_BACKEND_TILE_MANAGER (tile_store);

  switch (command)
    {
    case GEGL_TILE_GET:
      if (backend_tm->priv->mul > 1)
        return gimp_tile_read_mul (backend_tm, x, y);
      return gimp_tile_read (backend_tm, x, y);
    case GEGL_TILE_SET:
      if (backend_tm->priv->mul > 1)
        gimp_tile_write_mul (backend_tm, x, y, gegl_tile_get_data (data));
      else
        gimp_tile_write (backend_tm, x, y, gegl_tile_get_data (data));

      gegl_tile_mark_as_stored (data);
      return NULL;

    case GEGL_TILE_IDLE:
    case GEGL_TILE_VOID:
    case GEGL_TILE_EXIST:
      return NULL;

    default:
      g_assert (command < GEGL_TILE_LAST_COMMAND && command >= 0);
    }

  return NULL;
}

static GeglTile *
gimp_tile_read (GimpTileBackendTileManager *backend_tm,
                gint                        x,
                gint                        y)
{
  GeglTileBackend            *backend;
  GeglTile *tile;
  gint      tile_size;
  Tile     *gimp_tile;
  gint      tile_stride;
  gint      gimp_tile_stride;
  int       row;

  backend    = GEGL_TILE_BACKEND (backend_tm);
  gimp_tile = tile_manager_get_at (backend_tm->priv->tile_manager,
                                   x, y, TRUE, TRUE);
  if (!gimp_tile)
    return NULL;
  g_return_val_if_fail (gimp_tile != NULL, NULL);

  tile_size        = gegl_tile_backend_get_tile_size (backend);
  tile_stride      = TILE_WIDTH * tile_bpp (gimp_tile);
  gimp_tile_stride = tile_ewidth (gimp_tile) * tile_bpp (gimp_tile);

  if (tile_stride == gimp_tile_stride && 
      TILE_HEIGHT == tile_eheight (gimp_tile))
    {
      /* use the GimpTile directly as GEGL tile */
      tile = gegl_tile_new_bare ();
      gegl_tile_set_data_full (tile, tile_data_pointer (gimp_tile, 0, 0),
                               tile_size, tile_done_writing, gimp_tile);
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
      tile_release (gimp_tile, TRUE);
    }
  return tile;
}

static void
gimp_tile_write (GimpTileBackendTileManager *backend_tm,
                 gint                        x,
                 gint                        y,
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

  /* if the memory pointer already points to the data, there is
   * no point in copying it
   */
  if (source != tile_data_pointer (gimp_tile, 0, 0))
    {
      tile_stride      = TILE_WIDTH * tile_bpp (gimp_tile);
      gimp_tile_stride = tile_ewidth (gimp_tile) * tile_bpp (gimp_tile);

      for (row = 0; row < tile_eheight (gimp_tile); row++)
        {
          memcpy (tile_data_pointer (gimp_tile, 0, row),
                  source + row * tile_stride,
                  gimp_tile_stride);
        }
    }

  tile_release (gimp_tile, TRUE);
}


static GeglTile *
gimp_tile_read_mul (GimpTileBackendTileManager *backend_tm,
                    gint                        x,
                    gint                        y)
{
  GeglTileBackend            *backend;
  GeglTile *tile;
  gint      tile_size;
  int       u, v;
  int       mul = backend_tm->priv->mul;
  unsigned char *tile_data;
  void *validate_proc;

  x *= mul;
  y *= mul;

  validate_proc = backend_tm->priv->tile_manager->validate_proc;
  backend_tm->priv->tile_manager->validate_proc = NULL;

  backend    = GEGL_TILE_BACKEND (backend_tm);
  tile_size  = gegl_tile_backend_get_tile_size (backend);
  tile       = gegl_tile_new (tile_size);
  tile_data  = gegl_tile_get_data (tile);

  for (u = 0; u < mul; u++)
  for (v = 0; v < mul; v++)
  {
    Tile     *gimp_tile;
    
    gimp_tile = tile_manager_get_at (backend_tm->priv->tile_manager,
                                               x+u, y+v, TRUE, FALSE);
    if (gimp_tile)
      {
        gint ewidth           = tile_ewidth (gimp_tile);
        gint eheight          = tile_eheight (gimp_tile);
        gint bpp              = tile_bpp (gimp_tile);
        gint tile_stride      = mul * TILE_WIDTH * bpp;
        gint gimp_tile_stride = ewidth * bpp;
        gint row;

        for (row = 0; row < eheight; row++)
          {
            memcpy (tile_data + (row + TILE_HEIGHT * v) * tile_stride + u * TILE_WIDTH * bpp,
                    tile_data_pointer (gimp_tile, 0, row),
                    gimp_tile_stride);
          }
        tile_release (gimp_tile, FALSE);
      }
  }

  backend_tm->priv->tile_manager->validate_proc = validate_proc;
  return tile;
}

static void
gimp_tile_write_mul (GimpTileBackendTileManager *backend_tm,
                     gint                        x,
                     gint                        y,
                     guchar                     *source)
{
  int   u, v;
  int   mul = backend_tm->priv->mul;
  void *validate_proc;

  x *= mul;
  y *= mul;

  validate_proc = backend_tm->priv->tile_manager->validate_proc;
  backend_tm->priv->tile_manager->validate_proc = NULL;

  for (v = 0; v < mul; v++)
    for (u = 0; u < mul; u++)
    {
      Tile *gimp_tile = tile_manager_get_at (backend_tm->priv->tile_manager,
                                             x+u, y+v, TRUE, TRUE);
      if (gimp_tile)
        {
          gint  ewidth      = tile_ewidth (gimp_tile);
          gint  eheight     = tile_eheight (gimp_tile);
          gint  bpp         = tile_bpp (gimp_tile);

          gint  tile_stride = mul * TILE_WIDTH * bpp;
          gint  gimp_tile_stride = ewidth * bpp;
          gint  row;

          for (row = 0; row < eheight; row++)
              memcpy (tile_data_pointer (gimp_tile, 0, row),
                      source + (row + v * TILE_HEIGHT) *
                              tile_stride + u * TILE_WIDTH * bpp,
                      gimp_tile_stride);

          tile_release (gimp_tile, TRUE);
        }
    }
  backend_tm->priv->tile_manager->validate_proc = validate_proc;
}

GeglTileBackend *
gimp_tile_backend_tile_manager_new (TileManager *tm,
                                    const Babl  *format)
{
  GeglTileBackend            *ret;
  GimpTileBackendTileManager *backend_tm;

  gint             width  = tile_manager_width (tm);
  gint             height = tile_manager_height (tm);
  gint             bpp    = tile_manager_bpp (tm);
  gint             mul    = gimp_gegl_tile_mul ();
  GeglRectangle    rect   = { 0, 0, width, height };

  g_return_val_if_fail (format == NULL ||
                        babl_format_get_bytes_per_pixel (format) ==
                        babl_format_get_bytes_per_pixel (gimp_bpp_to_babl_format (bpp, TRUE)),
                        NULL);

  if (tm->validate_proc)
    mul = 1;

  if (! format)
    format = gimp_bpp_to_babl_format (bpp, TRUE);

  ret = g_object_new (GIMP_TYPE_TILE_BACKEND_TILE_MANAGER,
                      "tile-width",  TILE_WIDTH  * mul,
                      "tile-height", TILE_HEIGHT * mul,
                      "format",      format,
                      NULL);

  backend_tm = GIMP_TILE_BACKEND_TILE_MANAGER (ret);
  backend_tm->priv->mul = mul;

  backend_tm->priv->tile_manager = tile_manager_ref (tm);

  gegl_tile_backend_set_extent (ret, &rect);

  return ret;
}

TileManager *
gimp_tile_backend_tile_manager_get_tiles (GeglTileBackend *backend)
{
  g_return_val_if_fail (GIMP_IS_TILE_BACKEND_TILE_MANAGER (backend), NULL);

  return GIMP_TILE_BACKEND_TILE_MANAGER (backend)->priv->tile_manager;
}
