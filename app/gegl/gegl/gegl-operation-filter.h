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
#ifndef _GEGL_OPERATION_FILTER_H
#define _GEGL_OPERATION_FILTER_H

#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-operation.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_FILTER           (gegl_operation_filter_get_type ())
#define GEGL_OPERATION_FILTER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_FILTER, GeglOperationFilter))
#define GEGL_OPERATION_FILTER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_FILTER, GeglOperationFilterClass))
#define GEGL_OPERATION_FILTER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_FILTER, GeglOperationFilterClass))

typedef struct _GeglOperationFilter  GeglOperationFilter;
struct _GeglOperationFilter
{
  GeglOperation parent_instance;
};

typedef struct _GeglOperationFilterClass GeglOperationFilterClass;
struct _GeglOperationFilterClass
{
  GeglOperationClass parent_class;

  gboolean (* process) (GeglOperation *self,
                        gpointer       context_id);
};

GType gegl_operation_filter_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
