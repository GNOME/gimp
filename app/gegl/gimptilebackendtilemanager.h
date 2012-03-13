/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptilebackendtilemanager.h
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

#ifndef __GIMP_TILE_BACKEND_TILE_MANAGER_H__
#define __GIMP_TILE_BACKEND_TILE_MANAGER_H__

#include <gegl-buffer-backend.h>

G_BEGIN_DECLS

#define GIMP_TYPE_TILE_BACKEND_TILE_MANAGER            (gimp_tile_backend_tile_manager_get_type ())
#define GIMP_TILE_BACKEND_TILE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TILE_BACKEND_TILE_MANAGER, GimpTileBackendTileManager))
#define GIMP_TILE_BACKEND_TILE_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_TILE_BACKEND_TILE_MANAGER, GimpTileBackendTileManagerClass))
#define GIMP_IS_TILE_BACKEND_TILE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TILE_BACKEND_TILE_MANAGER))
#define GIMP_IS_TILE_BACKEND_TILE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_TILE_BACKEND_TILE_MANAGER))
#define GIMP_TILE_BACKEND_TILE_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_TILE_BACKEND_TILE_MANAGER, GimpTileBackendTileManagerClass))

typedef struct _GimpTileBackendTileManagerClass   GimpTileBackendTileManagerClass;
typedef struct _GimpTileBackendTileManagerPrivate GimpTileBackendTileManagerPrivate;

struct _GimpTileBackendTileManager
{
  GeglTileBackend  parent_instance;

  GimpTileBackendTileManagerPrivate *priv;
};

struct _GimpTileBackendTileManagerClass
{
  GeglTileBackendClass parent_class;
};

GType             gimp_tile_backend_tile_manager_get_type (void) G_GNUC_CONST;

GeglTileBackend * gimp_tile_backend_tile_manager_new (TileManager *tm,
                                                      gboolean     write);

GeglBuffer      * gimp_tile_manager_get_gegl_buffer  (TileManager *tm,
                                                      gboolean     write);

G_END_DECLS

#endif /* __GIMP_TILE_BACKEND_TILE_MANAGER_H__ */
