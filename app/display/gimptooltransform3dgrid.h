/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatool3dtransformgrid.h
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

#ifndef __LIGMA_TOOL_TRANSFORM_3D_GRID_H__
#define __LIGMA_TOOL_TRANSFORM_3D_GRID_H__


#include "ligmatooltransformgrid.h"


#define LIGMA_TYPE_TOOL_TRANSFORM_3D_GRID            (ligma_tool_transform_3d_grid_get_type ())
#define LIGMA_TOOL_TRANSFORM_3D_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_TRANSFORM_3D_GRID, LigmaToolTransform3DGrid))
#define LIGMA_TOOL_TRANSFORM_3D_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_TRANSFORM_3D_GRID, LigmaToolTransform3DGridClass))
#define LIGMA_IS_TOOL_TRANSFORM_3D_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_TRANSFORM_3D_GRID))
#define LIGMA_IS_TOOL_TRANSFORM_3D_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_TRANSFORM_3D_GRID))
#define LIGMA_TOOL_TRANSFORM_3D_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_TRANSFORM_3D_GRID, LigmaToolTransform3DGridClass))


typedef struct _LigmaToolTransform3DGrid        LigmaToolTransform3DGrid;
typedef struct _LigmaToolTransform3DGridPrivate LigmaToolTransform3DGridPrivate;
typedef struct _LigmaToolTransform3DGridClass   LigmaToolTransform3DGridClass;

struct _LigmaToolTransform3DGrid
{
  LigmaToolTransformGrid     parent_instance;

  LigmaToolTransform3DGridPrivate *priv;
};

struct _LigmaToolTransform3DGridClass
{
  LigmaToolTransformGridClass  parent_class;
};


GType            ligma_tool_transform_3d_grid_get_type (void) G_GNUC_CONST;

LigmaToolWidget * ligma_tool_transform_3d_grid_new      (LigmaDisplayShell    *shell,
                                                       gdouble              x1,
                                                       gdouble              y1,
                                                       gdouble              x2,
                                                       gdouble              y2,
                                                       gdouble              camera_x,
                                                       gdouble              camera_y,
                                                       gdouble              camera_z);


#endif /* __LIGMA_TOOL_TRANSFORM_3D_GRID_H__ */
