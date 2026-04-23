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


#define GIMP_TYPE_CANVAS_CANVAS_BOUNDARY (gimp_canvas_canvas_boundary_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpCanvasCanvasBoundary,
                          gimp_canvas_canvas_boundary,
                          GIMP, CANVAS_CANVAS_BOUNDARY,
                          GimpCanvasRectangle)


struct _GimpCanvasCanvasBoundaryClass
{
  GimpCanvasRectangleClass  parent_class;
};


GimpCanvasItem * gimp_canvas_canvas_boundary_new       (GimpDisplayShell        *shell);

void             gimp_canvas_canvas_boundary_set_image (GimpCanvasCanvasBoundary *boundary,
                                                        GimpImage                *image);
