/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasline.h
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

#ifndef __GIMP_CANVAS_LINE_H__
#define __GIMP_CANVAS_LINE_H__


#include "gimpcanvasitem.h"


#define GIMP_TYPE_CANVAS_LINE            (gimp_canvas_line_get_type ())
#define GIMP_CANVAS_LINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS_LINE, GimpCanvasLine))
#define GIMP_CANVAS_LINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS_LINE, GimpCanvasLineClass))
#define GIMP_IS_CANVAS_LINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS_LINE))
#define GIMP_IS_CANVAS_LINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS_LINE))
#define GIMP_CANVAS_LINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS_LINE, GimpCanvasLineClass))


typedef struct _GimpCanvasLine      GimpCanvasLine;
typedef struct _GimpCanvasLineClass GimpCanvasLineClass;

struct _GimpCanvasLine
{
  GimpCanvasItem  parent_instance;
};

struct _GimpCanvasLineClass
{
  GimpCanvasItemClass  parent_class;
};


GType            gimp_canvas_line_get_type (void) G_GNUC_CONST;

GimpCanvasItem * gimp_canvas_line_new      (GimpDisplayShell *shell,
                                            gdouble           x1,
                                            gdouble           y1,
                                            gdouble           x2,
                                            gdouble           y2);

void             gimp_canvas_line_set      (GimpCanvasItem   *line,
                                            gdouble           x1,
                                            gdouble           y1,
                                            gdouble           x2,
                                            gdouble           y2);


#endif /* __GIMP_CANVAS_LINE_H__ */
