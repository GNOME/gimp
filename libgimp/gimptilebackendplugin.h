/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptilebackendplugin.h
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

#ifndef __GIMP_TILE_BACKEND_PLUGIN_H__
#define __GIMP_TILE_BACKEND_PLUGIN_H__

#include <gegl-buffer-backend.h>

G_BEGIN_DECLS

#define GIMP_TYPE_TILE_BACKEND_PLUGIN            (_gimp_tile_backend_plugin_get_type ())
#define GIMP_TILE_BACKEND_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TILE_BACKEND_PLUGIN, GimpTileBackendPlugin))
#define GIMP_TILE_BACKEND_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_TILE_BACKEND_PLUGIN, GimpTileBackendPluginClass))
#define GIMP_IS_TILE_BACKEND_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TILE_BACKEND_PLUGIN))
#define GIMP_IS_TILE_BACKEND_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_TILE_BACKEND_PLUGIN))
#define GIMP_TILE_BACKEND_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_TILE_BACKEND_PLUGIN, GimpTileBackendPluginClass))


typedef struct _GimpTileBackendPlugin        GimpTileBackendPlugin;
typedef struct _GimpTileBackendPluginClass   GimpTileBackendPluginClass;
typedef struct _GimpTileBackendPluginPrivate GimpTileBackendPluginPrivate;

struct _GimpTileBackendPlugin
{
  GeglTileBackend               parent_instance;

  GimpTileBackendPluginPrivate *priv;
};

struct _GimpTileBackendPluginClass
{
  GeglTileBackendClass parent_class;
};

GType             _gimp_tile_backend_plugin_get_type (void) G_GNUC_CONST;

GeglTileBackend * _gimp_tile_backend_plugin_new      (GimpDrawable *drawable,
                                                      gint          shadow);

G_END_DECLS

#endif /* __GIMP_TILE_BACKEND_PLUGIN_H__ */
