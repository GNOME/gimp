/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationgrow.h
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


#define GIMP_TYPE_OPERATION_GROW            (gimp_operation_grow_get_type ())
#define GIMP_OPERATION_GROW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_GROW, GimpOperationGrow))
#define GIMP_OPERATION_GROW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_GROW, GimpOperationGrowClass))
#define GIMP_IS_OPERATION_GROW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_GROW))
#define GIMP_IS_OPERATION_GROW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_GROW))
#define GIMP_OPERATION_GROW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_GROW, GimpOperationGrowClass))


typedef struct _GimpOperationGrow      GimpOperationGrow;
typedef struct _GimpOperationGrowClass GimpOperationGrowClass;

struct _GimpOperationGrow
{
  GeglOperationFilter  parent_instance;

  gint                 radius_x;
  gint                 radius_y;
};

struct _GimpOperationGrowClass
{
  GeglOperationFilterClass  parent_class;
};


GType   gimp_operation_grow_get_type (void) G_GNUC_CONST;

