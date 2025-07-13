/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasgrid.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "gimpcanvasitem.h"


#define GIMP_TYPE_CANVAS_GRID            (gimp_canvas_grid_get_type ())
#define GIMP_CANVAS_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS_GRID, GimpCanvasGrid))
#define GIMP_CANVAS_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS_GRID, GimpCanvasGridClass))
#define GIMP_IS_CANVAS_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS_GRID))
#define GIMP_IS_CANVAS_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS_GRID))
#define GIMP_CANVAS_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS_GRID, GimpCanvasGridClass))


typedef struct _GimpCanvasGrid      GimpCanvasGrid;
typedef struct _GimpCanvasGridClass GimpCanvasGridClass;

struct _GimpCanvasGrid
{
  GimpCanvasItem  parent_instance;
};

struct _GimpCanvasGridClass
{
  GimpCanvasItemClass  parent_class;
};


GType            gimp_canvas_grid_get_type (void) G_GNUC_CONST;

GimpCanvasItem * gimp_canvas_grid_new      (GimpDisplayShell *shell,
                                            GimpGrid         *grid);
