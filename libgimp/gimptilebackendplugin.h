/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatilebackendplugin.h
 * Copyright (C) 2011-2019 Øyvind Kolås <pippin@ligma.org>
 *                         Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TILE_BACKEND_PLUGIN_H__
#define __LIGMA_TILE_BACKEND_PLUGIN_H__

#include <gegl-buffer-backend.h>

G_BEGIN_DECLS

#define LIGMA_TYPE_TILE_BACKEND_PLUGIN            (_ligma_tile_backend_plugin_get_type ())
#define LIGMA_TILE_BACKEND_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TILE_BACKEND_PLUGIN, LigmaTileBackendPlugin))
#define LIGMA_TILE_BACKEND_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_TILE_BACKEND_PLUGIN, LigmaTileBackendPluginClass))
#define LIGMA_IS_TILE_BACKEND_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TILE_BACKEND_PLUGIN))
#define LIGMA_IS_TILE_BACKEND_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_TILE_BACKEND_PLUGIN))
#define LIGMA_TILE_BACKEND_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_TILE_BACKEND_PLUGIN, LigmaTileBackendPluginClass))


typedef struct _LigmaTileBackendPlugin        LigmaTileBackendPlugin;
typedef struct _LigmaTileBackendPluginClass   LigmaTileBackendPluginClass;
typedef struct _LigmaTileBackendPluginPrivate LigmaTileBackendPluginPrivate;

struct _LigmaTileBackendPlugin
{
  GeglTileBackend               parent_instance;

  LigmaTileBackendPluginPrivate *priv;
};

struct _LigmaTileBackendPluginClass
{
  GeglTileBackendClass parent_class;
};

GType             _ligma_tile_backend_plugin_get_type (void) G_GNUC_CONST;

GeglTileBackend * _ligma_tile_backend_plugin_new      (LigmaDrawable *drawable,
                                                      gint          shadow);

G_END_DECLS

#endif /* __LIGMA_TILE_BACKEND_PLUGIN_H__ */
