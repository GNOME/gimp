/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationsubtract.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2017 Øyvind Kolås <pippin@gimp.org>
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

#ifndef __GIMP_OPERATION_SUBTRACT_H__
#define __GIMP_OPERATION_SUBTRACT_H__


#include "gimpoperationpointlayermode.h"


#define GIMP_TYPE_OPERATION_SUBTRACT            (gimp_operation_subtract_get_type ())
#define GIMP_OPERATION_SUBTRACT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_SUBTRACT, GimpOperationSubtract))
#define GIMP_OPERATION_SUBTRACT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_SUBTRACT, GimpOperationSubtractClass))
#define GIMP_IS_OPERATION_SUBTRACT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_SUBTRACT))
#define GIMP_IS_OPERATION_SUBTRACT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_SUBTRACT))
#define GIMP_OPERATION_SUBTRACT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_SUBTRACT, GimpOperationSubtractClass))


typedef struct _GimpOperationSubtract      GimpOperationSubtract;
typedef struct _GimpOperationSubtractClass GimpOperationSubtractClass;

struct _GimpOperationSubtract
{
  GimpOperationPointLayerMode  parent_instance;
};

struct _GimpOperationSubtractClass
{
  GimpOperationPointLayerModeClass  parent_class;
};


GType    gimp_operation_subtract_get_type       (void) G_GNUC_CONST;

gboolean gimp_operation_subtract_process_pixels (gfloat              *in,
                                                 gfloat              *layer,
                                                 gfloat              *mask,
                                                 gfloat              *out,
                                                 gfloat               opacity,
                                                 glong                samples,
                                                 const GeglRectangle *roi,
                                                 gint                 level);


#endif /* __GIMP_OPERATION_SUBTRACT_H__ */
