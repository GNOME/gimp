/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvascanvasvboundary.h
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

#include "gimpcanvasrectangle.h"


#define GIMP_TYPE_CANVAS_CANVAS_BOUNDARY            (gimp_canvas_canvas_boundary_get_type ())
#define GIMP_CANVAS_CANVAS_BOUNDARY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS_CANVAS_BOUNDARY, GimpCanvasCanvasBoundary))
#define GIMP_CANVAS_CANVAS_BOUNDARY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS_CANVAS_BOUNDARY, GimpCanvasCanvasBoundaryClass))
#define GIMP_IS_CANVAS_CANVAS_BOUNDARY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS_CANVAS_BOUNDARY))
#define GIMP_IS_CANVAS_CANVAS_BOUNDARY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS_CANVAS_BOUNDARY))
#define GIMP_CANVAS_CANVAS_BOUNDARY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS_CANVAS_BOUNDARY, GimpCanvasCanvasBoundaryClass))


typedef struct _GimpCanvasCanvasBoundary      GimpCanvasCanvasBoundary;
typedef struct _GimpCanvasCanvasBoundaryClass GimpCanvasCanvasBoundaryClass;

struct _GimpCanvasCanvasBoundary
{
  GimpCanvasRectangle  parent_instance;
};

struct _GimpCanvasCanvasBoundaryClass
{
  GimpCanvasRectangleClass  parent_class;
};


GType            gimp_canvas_canvas_boundary_get_type  (void) G_GNUC_CONST;

GimpCanvasItem * gimp_canvas_canvas_boundary_new       (GimpDisplayShell        *shell);

void             gimp_canvas_canvas_boundary_set_image (GimpCanvasCanvasBoundary *boundary,
                                                        GimpImage                *image);
