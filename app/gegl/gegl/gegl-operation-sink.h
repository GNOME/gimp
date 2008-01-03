/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås
 */
#ifndef _GEGL_OPERATION_SINK_H
#define _GEGL_OPERATION_SINK_H

#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-operation.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_SINK            (gegl_operation_sink_get_type ())
#define GEGL_OPERATION_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_SINK, GeglOperationSink))
#define GEGL_OPERATION_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_SINK, GeglOperationSinkClass))
#define GEGL_OPERATION_SINK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_SINK, GeglOperationSinkClass))
#define GEGL_IS_OPERATION_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_SINK))
#define GEGL_IS_OPERATION_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_SINK))

typedef struct _GeglOperationSink  GeglOperationSink;
struct _GeglOperationSink
{
  GeglOperation parent_instance;
};

typedef struct _GeglOperationSinkClass GeglOperationSinkClass;
struct _GeglOperationSinkClass
{
  GeglOperationClass parent_class;

  gboolean           needs_full;

  gboolean (* process) (GeglOperation *self,
                        gpointer       context_id);
};

GType    gegl_operation_sink_get_type   (void) G_GNUC_CONST;

gboolean gegl_operation_sink_needs_full (GeglOperation *operation);

G_END_DECLS

#endif
