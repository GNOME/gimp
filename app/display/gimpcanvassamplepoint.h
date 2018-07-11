/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvassamplepoint.h
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

#ifndef __GIMP_CANVAS_SAMPLE_POINT_H__
#define __GIMP_CANVAS_SAMPLE_POINT_H__


#include "gimpcanvasitem.h"


#define GIMP_TYPE_CANVAS_SAMPLE_POINT            (gimp_canvas_sample_point_get_type ())
#define GIMP_CANVAS_SAMPLE_POINT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS_SAMPLE_POINT, GimpCanvasSamplePoint))
#define GIMP_CANVAS_SAMPLE_POINT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS_SAMPLE_POINT, GimpCanvasSamplePointClass))
#define GIMP_IS_CANVAS_SAMPLE_POINT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS_SAMPLE_POINT))
#define GIMP_IS_CANVAS_SAMPLE_POINT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS_SAMPLE_POINT))
#define GIMP_CANVAS_SAMPLE_POINT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS_SAMPLE_POINT, GimpCanvasSamplePointClass))


typedef struct _GimpCanvasSamplePoint      GimpCanvasSamplePoint;
typedef struct _GimpCanvasSamplePointClass GimpCanvasSamplePointClass;

struct _GimpCanvasSamplePoint
{
  GimpCanvasItem  parent_instance;
};

struct _GimpCanvasSamplePointClass
{
  GimpCanvasItemClass  parent_class;
};


GType            gimp_canvas_sample_point_get_type (void) G_GNUC_CONST;

GimpCanvasItem * gimp_canvas_sample_point_new      (GimpDisplayShell *shell,
                                                    gint              x,
                                                    gint              y,
                                                    gint              index,
                                                    gboolean          sample_point_style);

void             gimp_canvas_sample_point_set      (GimpCanvasItem   *sample_point,
                                                    gint              x,
                                                    gint              y);


#endif /* __GIMP_CANVAS_SAMPLE_POINT_H__ */
