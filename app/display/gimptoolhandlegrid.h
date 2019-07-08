/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolhandlegrid.h
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

#ifndef __GIMP_TOOL_HANDLE_GRID_H__
#define __GIMP_TOOL_HANDLE_GRID_H__


#include "gimptooltransformgrid.h"


#define GIMP_TYPE_TOOL_HANDLE_GRID            (gimp_tool_handle_grid_get_type ())
#define GIMP_TOOL_HANDLE_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_HANDLE_GRID, GimpToolHandleGrid))
#define GIMP_TOOL_HANDLE_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_HANDLE_GRID, GimpToolHandleGridClass))
#define GIMP_IS_TOOL_HANDLE_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_HANDLE_GRID))
#define GIMP_IS_TOOL_HANDLE_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_HANDLE_GRID))
#define GIMP_TOOL_HANDLE_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_HANDLE_GRID, GimpToolHandleGridClass))


typedef struct _GimpToolHandleGrid        GimpToolHandleGrid;
typedef struct _GimpToolHandleGridPrivate GimpToolHandleGridPrivate;
typedef struct _GimpToolHandleGridClass   GimpToolHandleGridClass;

struct _GimpToolHandleGrid
{
  GimpToolTransformGrid      parent_instance;

  GimpToolHandleGridPrivate *private;
};

struct _GimpToolHandleGridClass
{
  GimpToolTransformGridClass  parent_class;
};


GType            gimp_tool_handle_grid_get_type (void) G_GNUC_CONST;

GimpToolWidget * gimp_tool_handle_grid_new      (GimpDisplayShell  *shell,
                                                 gdouble            x1,
                                                 gdouble            y1,
                                                 gdouble            x2,
                                                 gdouble            y2);


#endif /* __GIMP_TOOL_HANDLE_GRID_H__ */
