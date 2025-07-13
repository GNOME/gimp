/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptool3dtransformgrid.h
 * Copyright (C) 2019 Ell
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

#include "gimptooltransformgrid.h"


#define GIMP_TYPE_TOOL_TRANSFORM_3D_GRID            (gimp_tool_transform_3d_grid_get_type ())
#define GIMP_TOOL_TRANSFORM_3D_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_TRANSFORM_3D_GRID, GimpToolTransform3DGrid))
#define GIMP_TOOL_TRANSFORM_3D_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_TRANSFORM_3D_GRID, GimpToolTransform3DGridClass))
#define GIMP_IS_TOOL_TRANSFORM_3D_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_TRANSFORM_3D_GRID))
#define GIMP_IS_TOOL_TRANSFORM_3D_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_TRANSFORM_3D_GRID))
#define GIMP_TOOL_TRANSFORM_3D_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_TRANSFORM_3D_GRID, GimpToolTransform3DGridClass))


typedef struct _GimpToolTransform3DGrid        GimpToolTransform3DGrid;
typedef struct _GimpToolTransform3DGridPrivate GimpToolTransform3DGridPrivate;
typedef struct _GimpToolTransform3DGridClass   GimpToolTransform3DGridClass;

struct _GimpToolTransform3DGrid
{
  GimpToolTransformGrid     parent_instance;

  GimpToolTransform3DGridPrivate *priv;
};

struct _GimpToolTransform3DGridClass
{
  GimpToolTransformGridClass  parent_class;
};


GType            gimp_tool_transform_3d_grid_get_type (void) G_GNUC_CONST;

GimpToolWidget * gimp_tool_transform_3d_grid_new      (GimpDisplayShell    *shell,
                                                       gdouble              x1,
                                                       gdouble              y1,
                                                       gdouble              x2,
                                                       gdouble              y2,
                                                       gdouble              camera_x,
                                                       gdouble              camera_y,
                                                       gdouble              camera_z);
