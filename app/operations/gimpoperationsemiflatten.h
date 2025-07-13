/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationsemiflatten.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_OPERATION_SEMI_FLATTEN            (gimp_operation_semi_flatten_get_type ())
#define GIMP_OPERATION_SEMI_FLATTEN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_SEMI_FLATTEN, GimpOperationSemiFlatten))
#define GIMP_OPERATION_SEMI_FLATTEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_SEMI_FLATTEN, GimpOperationSemiFlattenClass))
#define GIMP_IS_OPERATION_SEMI_FLATTEN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_SEMI_FLATTEN))
#define GIMP_IS_OPERATION_SEMI_FLATTEN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_SEMI_FLATTEN))
#define GIMP_OPERATION_SEMI_FLATTEN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_SEMI_FLATTEN, GimpOperationSemiFlattenClass))


typedef struct _GimpOperationSemiFlatten      GimpOperationSemiFlatten;
typedef struct _GimpOperationSemiFlattenClass GimpOperationSemiFlattenClass;

struct _GimpOperationSemiFlatten
{
  GeglOperationPointFilter  parent_instance;

  GeglColor                *color;
};

struct _GimpOperationSemiFlattenClass
{
  GeglOperationPointFilterClass  parent_class;
};


GType   gimp_operation_semi_flatten_get_type (void) G_GNUC_CONST;
