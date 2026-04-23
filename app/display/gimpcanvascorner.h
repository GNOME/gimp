/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvascorner.h
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


#define GIMP_TYPE_CANVAS_CORNER (gimp_canvas_corner_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpCanvasCorner,
                          gimp_canvas_corner,
                          GIMP, CANVAS_CORNER,
                          GimpCanvasItem)


struct _GimpCanvasCornerClass
{
  GimpCanvasItemClass  parent_class;
};


GimpCanvasItem * gimp_canvas_corner_new (GimpDisplayShell *shell,
                                         gdouble           x,
                                         gdouble           y,
                                         gdouble           width,
                                         gdouble           height,
                                         GimpHandleAnchor  anchor,
                                         gint              corner_width,
                                         gint              corner_height,
                                         gboolean          outside);

void             gimp_canvas_corner_set (GimpCanvasItem   *corner,
                                         gdouble           x,
                                         gdouble           y,
                                         gdouble           width,
                                         gdouble           height,
                                         gint              corner_width,
                                         gint              corner_height,
                                         gboolean          outside);
