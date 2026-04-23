/* GIMP - The GNU Image Manipulation Program Copyright (C) 1995
 * Spencer Kimball and Peter Mattis
 *
 * gimpcanvaspolygon.h
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


#define GIMP_TYPE_CANVAS_POLYGON (gimp_canvas_polygon_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpCanvasPolygon,
                          gimp_canvas_polygon,
                          GIMP, CANVAS_POLYGON,
                          GimpCanvasItem)


struct _GimpCanvasPolygonClass
{
  GimpCanvasItemClass  parent_class;
};


GimpCanvasItem * gimp_canvas_polygon_new             (GimpDisplayShell  *shell,
                                                      const GimpVector2 *points,
                                                      gint               n_points,
                                                      GimpMatrix3       *transform,
                                                      gboolean           filled);
GimpCanvasItem * gimp_canvas_polygon_new_from_coords (GimpDisplayShell  *shell,
                                                      const GimpCoords  *coords,
                                                      gint               n_coords,
                                                      GimpMatrix3       *transform,
                                                      gboolean           filled);

void             gimp_canvas_polygon_set_points      (GimpCanvasItem    *polygon,
                                                      const GimpVector2 *points,
                                                      gint               n_points);
