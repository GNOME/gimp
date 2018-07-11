/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasrectangle.h
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

#ifndef __GIMP_CANVAS_RECTANGLE_H__
#define __GIMP_CANVAS_RECTANGLE_H__


#include "gimpcanvasitem.h"


#define GIMP_TYPE_CANVAS_RECTANGLE            (gimp_canvas_rectangle_get_type ())
#define GIMP_CANVAS_RECTANGLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS_RECTANGLE, GimpCanvasRectangle))
#define GIMP_CANVAS_RECTANGLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS_RECTANGLE, GimpCanvasRectangleClass))
#define GIMP_IS_CANVAS_RECTANGLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS_RECTANGLE))
#define GIMP_IS_CANVAS_RECTANGLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS_RECTANGLE))
#define GIMP_CANVAS_RECTANGLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS_RECTANGLE, GimpCanvasRectangleClass))


typedef struct _GimpCanvasRectangle      GimpCanvasRectangle;
typedef struct _GimpCanvasRectangleClass GimpCanvasRectangleClass;

struct _GimpCanvasRectangle
{
  GimpCanvasItem  parent_instance;
};

struct _GimpCanvasRectangleClass
{
  GimpCanvasItemClass  parent_class;
};


GType            gimp_canvas_rectangle_get_type (void) G_GNUC_CONST;

GimpCanvasItem * gimp_canvas_rectangle_new      (GimpDisplayShell *shell,
                                                 gdouble           x,
                                                 gdouble           y,
                                                 gdouble           width,
                                                 gdouble           height,
                                                 gboolean          filled);

void             gimp_canvas_rectangle_set      (GimpCanvasItem   *rectangle,
                                                 gdouble           x,
                                                 gdouble           y,
                                                 gdouble           width,
                                                 gdouble           height);


#endif /* __GIMP_CANVAS_RECTANGLE_H__ */
