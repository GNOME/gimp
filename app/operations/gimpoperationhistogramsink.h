/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationhistogramsink.h
 * Copyright (C) 2012 Øyvind Kolås
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

#pragma once

#include <gegl-plugin.h>
#include <operation/gegl-operation-sink.h>


#define GIMP_TYPE_OPERATION_HISTOGRAM_SINK            (gimp_operation_histogram_sink_get_type ())
#define GIMP_OPERATION_HISTOGRAM_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_HISTOGRAM_SINK, GimpOperationHistogramSink))
#define GIMP_OPERATION_HISTOGRAM_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_HISTOGRAM_SINK, GimpOperationHistogramSinkClass))
#define GEGL_IS_OPERATION_HISTOGRAM_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_HISTOGRAM_SINK))
#define GEGL_IS_OPERATION_HISTOGRAM_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_HISTOGRAM_SINK))
#define GIMP_OPERATION_HISTOGRAM_SINK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_HISTOGRAM_SINK, GimpOperationHistogramSinkClass))


typedef struct _GimpOperationHistogramSink      GimpOperationHistogramSink;
typedef struct _GimpOperationHistogramSinkClass GimpOperationHistogramSinkClass;

struct _GimpOperationHistogramSink
{
  GeglOperation  parent_instance;

  GimpHistogram *histogram;
};

struct _GimpOperationHistogramSinkClass
{
  GeglOperationSinkClass  parent_class;
};


GType   gimp_operation_histogram_sink_get_type (void) G_GNUC_CONST;

