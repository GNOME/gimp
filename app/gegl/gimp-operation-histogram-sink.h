/* 
 * Copyright 2012 Øyvind Kolås
 */

#ifndef __GIMP_OPERATION_HISTOGRAM_SINK_H__
#define __GIMP_OPERATION_HISTOGRAM_SINK_H__

#include "gegl-operation-sink.h"

G_BEGIN_DECLS

#define GIMP_TYPE_HISTOGRAM_SINK            (gimp_operation_histogram_sink_get_type ())
#define GIMP_OPERATION_HISTOGRAM_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HISTOGRAM_SINK, GimpOperationHistogramSink))
#define GIMP_OPERATION_HISTOGRAM_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_HISTOGRAM_SINK, GimpOperationHistogramSinkClass))
#define GEGL_IS_OPERATION_HISTOGRAM_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_HISTOGRAM_SINK))
#define GEGL_IS_OPERATION_HISTOGRAM_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_HISTOGRAM_SINK))
#define GIMP_OPERATION_HISTOGRAM_SINK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_HISTOGRAM_SINK, GimpOperationHistogramSinkClass))

typedef struct _GimpOperationHistogramSink  GimpOperationHistogramSink;
struct _GimpOperationHistogramSink
{
  GeglOperation parent_instance;
};

typedef struct _GimpOperationHistogramSinkClass GimpOperationHistogramSinkClass;
struct _GimpOperationHistogramSinkClass
{
  GeglOperationSinkClass parent_class;

  gboolean (* process) (GeglOperation       *self,
                        GeglBuffer          *input,
                        GeglBuffer          *aux,
                        GeglBuffer          *output,
                        const GeglRectangle *result,
                        gint                 level);
  gpointer              pad[4];
};

GType gimp_operation_histogram_sink_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
