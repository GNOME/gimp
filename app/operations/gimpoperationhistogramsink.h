/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationhistogramsink.h
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

#ifndef __LIGMA_OPERATION_HISTOGRAM_SINK_H__
#define __LIGMA_OPERATION_HISTOGRAM_SINK_H__


#include <gegl-plugin.h>
#include <operation/gegl-operation-sink.h>


#define LIGMA_TYPE_OPERATION_HISTOGRAM_SINK            (ligma_operation_histogram_sink_get_type ())
#define LIGMA_OPERATION_HISTOGRAM_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_HISTOGRAM_SINK, LigmaOperationHistogramSink))
#define LIGMA_OPERATION_HISTOGRAM_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_HISTOGRAM_SINK, LigmaOperationHistogramSinkClass))
#define GEGL_IS_OPERATION_HISTOGRAM_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_HISTOGRAM_SINK))
#define GEGL_IS_OPERATION_HISTOGRAM_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_HISTOGRAM_SINK))
#define LIGMA_OPERATION_HISTOGRAM_SINK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_HISTOGRAM_SINK, LigmaOperationHistogramSinkClass))


typedef struct _LigmaOperationHistogramSink      LigmaOperationHistogramSink;
typedef struct _LigmaOperationHistogramSinkClass LigmaOperationHistogramSinkClass;

struct _LigmaOperationHistogramSink
{
  GeglOperation  parent_instance;

  LigmaHistogram *histogram;
};

struct _LigmaOperationHistogramSinkClass
{
  GeglOperationSinkClass  parent_class;
};


GType   ligma_operation_histogram_sink_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_HISTOGRAM_SINK_C__ */
