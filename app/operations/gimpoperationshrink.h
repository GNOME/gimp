/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationshrink.h
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


#define GIMP_TYPE_OPERATION_SHRINK            (gimp_operation_shrink_get_type ())
#define GIMP_OPERATION_SHRINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_SHRINK, GimpOperationShrink))
#define GIMP_OPERATION_SHRINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_SHRINK, GimpOperationShrinkClass))
#define GIMP_IS_OPERATION_SHRINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_SHRINK))
#define GIMP_IS_OPERATION_SHRINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_SHRINK))
#define GIMP_OPERATION_SHRINK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_SHRINK, GimpOperationShrinkClass))


typedef struct _GimpOperationShrink      GimpOperationShrink;
typedef struct _GimpOperationShrinkClass GimpOperationShrinkClass;

struct _GimpOperationShrink
{
  GeglOperationFilter  parent_instance;

  gint                 radius_x;
  gint                 radius_y;
  gboolean             edge_lock;
};

struct _GimpOperationShrinkClass
{
  GeglOperationFilterClass  parent_class;
};


GType   gimp_operation_shrink_get_type (void) G_GNUC_CONST;
