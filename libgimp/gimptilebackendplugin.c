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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <gegl.h>
#include "gimp.h"
#include "gimptilebackendplugin.h"

#define TILE_WIDTH  gimp_tile_width()
#define TILE_HEIGHT gimp_tile_width()

struct _GimpTileBackendPluginPrivate
{
  GimpDrawable *drawable;
  gboolean      shadow;
  int           mul;
};

static int gimp_gegl_tile_mul (void)
{
  static int mul = 2;
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

static const Babl *get_format (gint32 drawable_ID);
static const Babl *get_format (gint32 drawable_ID)
{
  switch (gimp_drawable_type (drawable_ID))
    {
      case GIMP_RGB_IMAGE:  return babl_format ("R'G'B' u8");
      case GIMP_RGBA_IMAGE: return babl_format ("R'G'B'A u8");
      case GIMP_GRAY_IMAGE:  return babl_format ("Y' u8");
      case GIMP_GRAYA_IMAGE: return babl_format ("Y'A u8");
      case GIMP_INDEXED_IMAGE:
      case GIMP_INDEXEDA_IMAGE:
        {
          gint32 image_ID = gimp_item_get_image (drawable_ID);
          const Babl *pala, *pal;
          gint    ncols;
          guchar *cmap = gimp_image_get_colormap (image_ID, &ncols);
          babl_new_palette (NULL, &pal, &pala);
          babl_palette_set_palette (pal, babl_format ("R'G'B' u8"),
                                    cmap, ncols);
          g_free (cmap);
          if (gimp_drawable_type (drawable_ID) == GIMP_INDEXEDA_IMAGE)
            return pala;
          return pal;
        }
    }
  return NULL;
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


G_DEFINE_TYPE (GimpTileBackendPlugin, _gimp_tile_backend_plugin,
               GEGL_TYPE_TILE_BACKEND)

#define parent_class _gimp_tile_backend_plugin_parent_class


static void
_gimp_tile_backend_plugin_class_init (GimpTileBackendPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_tile_backend_plugin_finalize;

  g_type_class_add_private (klass, sizeof (GimpTileBackendPluginPrivate));
  gimp_tile_cache_size (1024);
}

static void
_gimp_tile_backend_plugin_init (GimpTileBackendPlugin *backend)
{
  GeglTileSource *source = GEGL_TILE_SOURCE (backend);

  backend->priv = G_TYPE_INSTANCE_GET_PRIVATE (backend,
                                               GIMP_TYPE_TILE_BACKEND_PLUGIN,
                                               GimpTileBackendPluginPrivate);

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
  GimpTileBackendPlugin *backend_plugin;

  backend_plugin = GIMP_TILE_BACKEND_PLUGIN (tile_store);

  switch (command)
    {
    case GEGL_TILE_GET:
      return gimp_tile_read_mul (backend_plugin, x, y);
    case GEGL_TILE_SET:
      gimp_tile_write_mul (backend_plugin, x, y, gegl_tile_get_data (data));
      gegl_tile_mark_as_stored (data);
      break;
    case GEGL_TILE_FLUSH:
      gimp_drawable_flush (backend_plugin->priv->drawable);
      break;
    default:
      g_assert (command < GEGL_TILE_LAST_COMMAND && command >= 0);
    }

  return NULL;
}

static GeglTile *
gimp_tile_read_mul (GimpTileBackendPlugin *backend_plugin,
                    gint                   x,
                    gint                   y)
{
  GimpTileBackendPluginPrivate *priv = backend_plugin->priv;
  GeglTileBackend            *backend;
  GeglTile *tile;
  gint      tile_size;
  int       u, v;
  int       mul = priv->mul;
  unsigned char *tile_data;

  x *= mul;
  y *= mul;

  backend    = GEGL_TILE_BACKEND (backend_plugin);
  tile_size  = gegl_tile_backend_get_tile_size (backend);
  tile       = gegl_tile_new (tile_size);
  tile_data  = gegl_tile_get_data (tile);

  for (u = 0; u < mul; u++)
    for (v = 0; v < mul; v++)
      {
        GimpTile     *gimp_tile;

        if (x + u >= priv->drawable->ntile_cols ||
            y + v >= priv->drawable->ntile_rows)
          continue;

        gimp_tile = gimp_drawable_get_tile (priv->drawable,
                                            priv->shadow,
                                            y+v, x+u);
        gimp_tile_ref (gimp_tile);
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
                        ((char*)gimp_tile->data) + row * gimp_tile_stride,
                        gimp_tile_stride);
              }
          }
        gimp_tile_unref (gimp_tile, FALSE);
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
  int   u, v;
  int   mul = priv->mul;

  if (!priv->shadow)
    return;

  x *= mul;
  y *= mul;

  for (v = 0; v < mul; v++)
    for (u = 0; u < mul; u++)
    {
      GimpTile *gimp_tile;

      if (x + u >= priv->drawable->ntile_cols ||
          y + v >= priv->drawable->ntile_rows)
        continue;

      gimp_tile = gimp_drawable_get_tile (priv->drawable,
                                          priv->shadow,
                                          y+v, x+u);
      gimp_tile_ref (gimp_tile);
      {
        gint ewidth           = gimp_tile->ewidth;
        gint eheight          = gimp_tile->eheight;
        gint bpp              = gimp_tile->bpp;
        gint tile_stride      = mul * TILE_WIDTH * bpp;
        gint gimp_tile_stride = ewidth * bpp;
        gint row;

        for (row = 0; row < eheight; row++)
          memcpy (((char*)gimp_tile->data) + row * gimp_tile_stride,
                  source + (row + v * TILE_HEIGHT) *
                            tile_stride + u * TILE_WIDTH * bpp,
                  gimp_tile_stride);
      }
      gimp_tile_unref (gimp_tile, TRUE);
    }
}

GeglTileBackend *
_gimp_tile_backend_plugin_new (GimpDrawable *drawable,
                               gint          shadow)
{
  const Babl                   *format;
  GeglTileBackend              *ret;
  GimpTileBackendPlugin        *backend_plugin;
  GimpTileBackendPluginPrivate *priv;

  gint             width  = drawable->width;
  gint             height = drawable->height;
  gint             mul    = gimp_gegl_tile_mul ();
  GeglRectangle    rect   = { 0, 0, width, height};

  format = get_format (drawable->drawable_id);

  ret = g_object_new (GIMP_TYPE_TILE_BACKEND_PLUGIN,
                      "tile-width",  TILE_WIDTH  * mul,
                      "tile-height", TILE_HEIGHT * mul,
                      "format",      format,
                      NULL);

  backend_plugin = GIMP_TILE_BACKEND_PLUGIN (ret);
  priv = backend_plugin->priv;
  priv->drawable = drawable;
  priv->mul = mul;
  priv->shadow = shadow;

  gegl_tile_backend_set_extent (ret, &rect);

  return ret;
}
