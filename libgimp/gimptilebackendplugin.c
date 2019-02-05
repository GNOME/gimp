/* gimptilebackendtilemanager.c
 * Copyright (C) 2012 Øyvind Kolås <pippin@gimp.org>
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

#include <gegl.h>

#define GIMP_DISABLE_DEPRECATION_WARNINGS

#include "gimp.h"
#include "gimptilebackendplugin.h"


#define TILE_WIDTH  gimp_tile_width()
#define TILE_HEIGHT gimp_tile_height()


struct _GimpTileBackendPluginPrivate
{
  GimpDrawable *drawable;
  gboolean      shadow;
  gint          mul;
};


static gint
gimp_gegl_tile_mul (void)
{
  static gint     mul    = 1;
  static gboolean inited = FALSE;

  if (G_LIKELY (inited))
    return mul;

  inited = TRUE;

  if (g_getenv ("GIMP_GEGL_TILE_MUL"))
    mul = atoi (g_getenv ("GIMP_GEGL_TILE_MUL"));

  if (mul < 1)
    mul = 1;

  return mul;
}

static void     gimp_tile_backend_plugin_finalize (GObject         *object);
static gpointer gimp_tile_backend_plugin_command  (GeglTileSource  *tile_store,
                                                   GeglTileCommand  command,
                                                   gint             x,
                                                   gint             y,
                                                   gint             z,
                                                   gpointer         data);

static void       gimp_tile_write_mul (GimpTileBackendPlugin *backend_plugin,
                                       gint                   x,
                                       gint                   y,
                                       guchar                *source);

static GeglTile * gimp_tile_read_mul (GimpTileBackendPlugin *backend_plugin,
                                      gint                   x,
                                      gint                   y);


G_DEFINE_TYPE_WITH_PRIVATE (GimpTileBackendPlugin, _gimp_tile_backend_plugin,
                            GEGL_TYPE_TILE_BACKEND)

#define parent_class _gimp_tile_backend_plugin_parent_class


static GMutex backend_plugin_mutex;


static void
_gimp_tile_backend_plugin_class_init (GimpTileBackendPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_tile_backend_plugin_finalize;
}

static void
_gimp_tile_backend_plugin_init (GimpTileBackendPlugin *backend)
{
  GeglTileSource *source = GEGL_TILE_SOURCE (backend);

  backend->priv = _gimp_tile_backend_plugin_get_instance_private (backend);

  source->command = gimp_tile_backend_plugin_command;
}

static void
gimp_tile_backend_plugin_finalize (GObject *object)
{
  GimpTileBackendPlugin *backend = GIMP_TILE_BACKEND_PLUGIN (object);

  if (backend->priv->drawable) /* This also causes a flush */
    gimp_drawable_detach (backend->priv->drawable);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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

          result = gimp_tile_read_mul (backend_plugin, x, y);

          g_mutex_unlock (&backend_plugin_mutex);
        }
      break;

    case GEGL_TILE_SET:
      /* TODO: actually store mipmapped tiles */
      if (z == 0)
        {
          g_mutex_lock (&backend_plugin_mutex);

          gimp_tile_write_mul (backend_plugin, x, y, gegl_tile_get_data (data));

          g_mutex_unlock (&backend_plugin_mutex);
        }

      gegl_tile_mark_as_stored (data);
      break;

    case GEGL_TILE_FLUSH:
      g_mutex_lock (&backend_plugin_mutex);

      gimp_drawable_flush (backend_plugin->priv->drawable);

      g_mutex_unlock (&backend_plugin_mutex);
      break;

    default:
      result = gegl_tile_backend_command (GEGL_TILE_BACKEND (tile_store),
                                          command, x, y, z, data);
      break;
    }

  return result;
}

static GeglTile *
gimp_tile_read_mul (GimpTileBackendPlugin *backend_plugin,
                    gint                   x,
                    gint                   y)
{
  GimpTileBackendPluginPrivate *priv    = backend_plugin->priv;
  GeglTileBackend              *backend = GEGL_TILE_BACKEND (backend_plugin);
  GeglTile                     *tile;
  gint                          tile_size;
  gint                          u, v;
  gint                          mul = priv->mul;
  guchar                       *tile_data;

  x *= mul;
  y *= mul;

  tile_size  = gegl_tile_backend_get_tile_size (backend);
  tile       = gegl_tile_new (tile_size);
  tile_data  = gegl_tile_get_data (tile);

  for (u = 0; u < mul; u++)
    {
      for (v = 0; v < mul; v++)
        {
          GimpTile *gimp_tile;

          if (x + u >= priv->drawable->ntile_cols ||
              y + v >= priv->drawable->ntile_rows)
            continue;

          gimp_tile = gimp_drawable_get_tile (priv->drawable,
                                              priv->shadow,
                                              y + v, x + u);
          _gimp_tile_ref_nocache (gimp_tile, TRUE);

          {
            gint ewidth           = gimp_tile->ewidth;
            gint eheight          = gimp_tile->eheight;
            gint bpp              = gimp_tile->bpp;
            gint tile_stride      = mul * TILE_WIDTH * bpp;
            gint gimp_tile_stride = ewidth * bpp;
            gint row;

            for (row = 0; row < eheight; row++)
              {
                memcpy (tile_data + (row + TILE_HEIGHT * v) *
                        tile_stride + u * TILE_WIDTH * bpp,
                        ((gchar *) gimp_tile->data) + row * gimp_tile_stride,
                        gimp_tile_stride);
              }
          }

          gimp_tile_unref (gimp_tile, FALSE);
        }
    }

  return tile;
}

static void
gimp_tile_write_mul (GimpTileBackendPlugin *backend_plugin,
                     gint                   x,
                     gint                   y,
                     guchar                *source)
{
  GimpTileBackendPluginPrivate *priv = backend_plugin->priv;
  gint                          u, v;
  gint                          mul = priv->mul;

  x *= mul;
  y *= mul;

  for (v = 0; v < mul; v++)
    {
      for (u = 0; u < mul; u++)
        {
          GimpTile *gimp_tile;

          if (x + u >= priv->drawable->ntile_cols ||
              y + v >= priv->drawable->ntile_rows)
            continue;

          gimp_tile = gimp_drawable_get_tile (priv->drawable,
                                              priv->shadow,
                                              y+v, x+u);
          _gimp_tile_ref_nocache (gimp_tile, FALSE);

          {
            gint ewidth           = gimp_tile->ewidth;
            gint eheight          = gimp_tile->eheight;
            gint bpp              = gimp_tile->bpp;
            gint tile_stride      = mul * TILE_WIDTH * bpp;
            gint gimp_tile_stride = ewidth * bpp;
            gint row;

            for (row = 0; row < eheight; row++)
              memcpy (((gchar *)gimp_tile->data) + row * gimp_tile_stride,
                      source + (row + v * TILE_HEIGHT) *
                      tile_stride + u * TILE_WIDTH * bpp,
                      gimp_tile_stride);
          }

          gimp_tile_unref (gimp_tile, TRUE);
        }
    }
}

GeglTileBackend *
_gimp_tile_backend_plugin_new (GimpDrawable *drawable,
                               gint          shadow)
{
  GeglTileBackend       *backend;
  GimpTileBackendPlugin *backend_plugin;
  const Babl            *format;
  gint                   width  = gimp_drawable_width (drawable->drawable_id);
  gint                   height = gimp_drawable_height (drawable->drawable_id);
  gint                   mul    = gimp_gegl_tile_mul ();

  format = gimp_drawable_get_format (drawable->drawable_id);

  backend = g_object_new (GIMP_TYPE_TILE_BACKEND_PLUGIN,
                          "tile-width",  TILE_WIDTH  * mul,
                          "tile-height", TILE_HEIGHT * mul,
                          "format",      format,
                          NULL);

  backend_plugin = GIMP_TILE_BACKEND_PLUGIN (backend);

  backend_plugin->priv->drawable = drawable;
  backend_plugin->priv->mul      = mul;
  backend_plugin->priv->shadow   = shadow;

  gegl_tile_backend_set_extent (backend,
                                GEGL_RECTANGLE (0, 0, width, height));

  return backend;
}
