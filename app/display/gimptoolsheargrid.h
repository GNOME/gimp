/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolsheargrid.h
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TOOL_SHEAR_GRID_H__
#define __LIGMA_TOOL_SHEAR_GRID_H__


#include "ligmatooltransformgrid.h"


#define LIGMA_TYPE_TOOL_SHEAR_GRID            (ligma_tool_shear_grid_get_type ())
#define LIGMA_TOOL_SHEAR_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_SHEAR_GRID, LigmaToolShearGrid))
#define LIGMA_TOOL_SHEAR_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_SHEAR_GRID, LigmaToolShearGridClass))
#define LIGMA_IS_TOOL_SHEAR_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_SHEAR_GRID))
#define LIGMA_IS_TOOL_SHEAR_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_SHEAR_GRID))
#define LIGMA_TOOL_SHEAR_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_SHEAR_GRID, LigmaToolShearGridClass))


typedef struct _LigmaToolShearGrid        LigmaToolShearGrid;
typedef struct _LigmaToolShearGridPrivate LigmaToolShearGridPrivate;
typedef struct _LigmaToolShearGridClass   LigmaToolShearGridClass;

struct _LigmaToolShearGrid
{
  LigmaToolTransformGrid     parent_instance;

  LigmaToolShearGridPrivate *private;
};

struct _LigmaToolShearGridClass
{
  LigmaToolTransformGridClass  parent_class;
};


GType            ligma_tool_shear_grid_get_type (void) G_GNUC_CONST;

LigmaToolWidget * ligma_tool_shear_grid_new      (LigmaDisplayShell    *shell,
                                                gdouble              x1,
                                                gdouble              y1,
                                                gdouble              x2,
                                                gdouble              y2,
                                                LigmaOrientationType  orientation,
                                                gdouble              shear_x,
                                                gdouble              shear_y);


#endif /* __LIGMA_TOOL_SHEAR_GRID_H__ */
