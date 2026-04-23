/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvashandle.h
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


#define GIMP_CANVAS_HANDLE_SIZE_CIRCLE    13
#define GIMP_CANVAS_HANDLE_SIZE_CROSS     15
#define GIMP_CANVAS_HANDLE_SIZE_CROSSHAIR 43
#define GIMP_CANVAS_HANDLE_SIZE_LARGE     25
#define GIMP_CANVAS_HANDLE_SIZE_SMALL     7


#define GIMP_TYPE_CANVAS_HANDLE (gimp_canvas_handle_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpCanvasHandle,
                          gimp_canvas_handle,
                          GIMP, CANVAS_HANDLE,
                          GimpCanvasItem)


struct _GimpCanvasHandleClass
{
  GimpCanvasItemClass  parent_class;
};


GimpCanvasItem * gimp_canvas_handle_new          (GimpDisplayShell *shell,
                                                  GimpHandleType    type,
                                                  GimpHandleAnchor  anchor,
                                                  gdouble           x,
                                                  gdouble           y,
                                                  gint              width,
                                                  gint              height);

void             gimp_canvas_handle_get_position (GimpCanvasItem   *handle,
                                                  gdouble          *x,
                                                  gdouble          *y);
void             gimp_canvas_handle_set_position (GimpCanvasItem   *handle,
                                                  gdouble           x,
                                                  gdouble           y);

gint             gimp_canvas_handle_calc_size    (GimpCanvasItem   *item,
                                                  gdouble           mouse_x,
                                                  gdouble           mouse_y,
                                                  gint              normal_size,
                                                  gint              hover_size);

void             gimp_canvas_handle_get_size     (GimpCanvasItem   *handle,
                                                  gint             *width,
                                                  gint             *height);
void             gimp_canvas_handle_set_size     (GimpCanvasItem   *handle,
                                                  gint              width,
                                                  gint              height);

void             gimp_canvas_handle_set_angles   (GimpCanvasItem   *handle,
                                                  gdouble           start_handle,
                                                  gdouble           slice_handle);
