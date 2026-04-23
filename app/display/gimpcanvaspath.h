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


#define GIMP_TYPE_CANVAS_PATH (gimp_canvas_path_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpCanvasPath,
                          gimp_canvas_path,
                          GIMP, CANVAS_PATH,
                          GimpCanvasItem)


struct _GimpCanvasPathClass
{
  GimpCanvasItemClass  parent_class;
};


GimpCanvasItem * gimp_canvas_path_new (GimpDisplayShell     *shell,
                                       const GimpBezierDesc *bezier,
                                       gdouble               x,
                                       gdouble               y,
                                       gboolean              filled,
                                       GimpPathStyle         style);

void             gimp_canvas_path_set (GimpCanvasItem       *path,
                                       const GimpBezierDesc *bezier);
