/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolrotategrid.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_TOOL_ROTATE_GRID_H__
#define __GIMP_TOOL_ROTATE_GRID_H__


#include "gimptooltransformgrid.h"


#define GIMP_TYPE_TOOL_ROTATE_GRID            (gimp_tool_rotate_grid_get_type ())
#define GIMP_TOOL_ROTATE_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_ROTATE_GRID, GimpToolRotateGrid))
#define GIMP_TOOL_ROTATE_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_ROTATE_GRID, GimpToolRotateGridClass))
#define GIMP_IS_TOOL_ROTATE_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_ROTATE_GRID))
#define GIMP_IS_TOOL_ROTATE_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_ROTATE_GRID))
#define GIMP_TOOL_ROTATE_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_ROTATE_GRID, GimpToolRotateGridClass))


typedef struct _GimpToolRotateGrid        GimpToolRotateGrid;
typedef struct _GimpToolRotateGridPrivate GimpToolRotateGridPrivate;
typedef struct _GimpToolRotateGridClass   GimpToolRotateGridClass;

struct _GimpToolRotateGrid
{
  GimpToolTransformGrid      parent_instance;

  GimpToolRotateGridPrivate *private;
};

struct _GimpToolRotateGridClass
{
  GimpToolTransformGridClass  parent_class;
};


GType            gimp_tool_rotate_grid_get_type (void) G_GNUC_CONST;

GimpToolWidget * gimp_tool_rotate_grid_new      (GimpDisplayShell  *shell,
                                                 gdouble            x1,
                                                 gdouble            y1,
                                                 gdouble            x2,
                                                 gdouble            y2,
                                                 gdouble            pivot_x,
                                                 gdouble            pivot_y,
                                                 gdouble            angle);


#endif /* __GIMP_TOOL_ROTATE_GRID_H__ */
