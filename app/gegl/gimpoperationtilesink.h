/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationtilesink.h
 * Copyright (C) 2007 Øyvind Kolås <pippin@gimp.org>
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

#ifndef __GIMP_OPERATION_TILE_SINK_H__
#define __GIMP_OPERATION_TILE_SINK_H__

#include <gegl-plugin.h>
#include <operation/gegl-operation-sink.h>


#define GIMP_TYPE_OPERATION_TILE_SINK           (gimp_operation_tile_sink_get_type ())
#define GIMP_OPERATION_TILE_SINK(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_TILE_SINK, GimpOperationTileSink))
#define GIMP_OPERATION_TILE_SINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_TILE_SINK, GimpOperationTileSinkClass))
#define GIMP_OPERATION_TILE_SINK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_TILE_SINK, GimpOperationTileSinkClass))


typedef struct _GimpOperationTileSinkClass GimpOperationTileSinkClass;

struct _GimpOperationTileSink
{
  GeglOperationSink  parent_instance;

  TileManager       *tile_manager;
  gboolean           linear; /* should linear data be assumed */
};

struct _GimpOperationTileSinkClass
{
  GeglOperationSinkClass  parent_class;

  void (* data_written) (GimpOperationTileSink *sink,
                         const GeglRectangle   *extent);
};


GType gimp_operation_tile_sink_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_OPERATION_TILE_SINK_H__ */
