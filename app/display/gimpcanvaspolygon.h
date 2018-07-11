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

#ifndef __GIMP_CANVAS_POLYGON_H__
#define __GIMP_CANVAS_POLYGON_H__


#include "gimpcanvasitem.h"


#define GIMP_TYPE_CANVAS_POLYGON            (gimp_canvas_polygon_get_type ())
#define GIMP_CANVAS_POLYGON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS_POLYGON, GimpCanvasPolygon))
#define GIMP_CANVAS_POLYGON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS_POLYGON, GimpCanvasPolygonClass))
#define GIMP_IS_CANVAS_POLYGON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS_POLYGON))
#define GIMP_IS_CANVAS_POLYGON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS_POLYGON))
#define GIMP_CANVAS_POLYGON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS_POLYGON, GimpCanvasPolygonClass))


typedef struct _GimpCanvasPolygon      GimpCanvasPolygon;
typedef struct _GimpCanvasPolygonClass GimpCanvasPolygonClass;

struct _GimpCanvasPolygon
{
  GimpCanvasItem  parent_instance;
};

struct _GimpCanvasPolygonClass
{
  GimpCanvasItemClass  parent_class;
};


GType            gimp_canvas_polygon_get_type        (void) G_GNUC_CONST;

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


#endif /* __GIMP_CANVAS_POLYGON_H__ */
