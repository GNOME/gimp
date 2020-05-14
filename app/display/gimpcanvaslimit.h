/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvaslimit.h
 * Copyright (C) 2020 Ell
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

#ifndef __GIMP_CANVAS_LIMIT_H__
#define __GIMP_CANVAS_LIMIT_H__


#include "gimpcanvasitem.h"


#define GIMP_TYPE_CANVAS_LIMIT            (gimp_canvas_limit_get_type ())
#define GIMP_CANVAS_LIMIT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS_LIMIT, GimpCanvasLimit))
#define GIMP_CANVAS_LIMIT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS_LIMIT, GimpCanvasLimitClass))
#define GIMP_IS_CANVAS_LIMIT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS_LIMIT))
#define GIMP_IS_CANVAS_LIMIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS_LIMIT))
#define GIMP_CANVAS_LIMIT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS_LIMIT, GimpCanvasLimitClass))


typedef struct _GimpCanvasLimit      GimpCanvasLimit;
typedef struct _GimpCanvasLimitClass GimpCanvasLimitClass;

struct _GimpCanvasLimit
{
  GimpCanvasItem  parent_instance;
};

struct _GimpCanvasLimitClass
{
  GimpCanvasItemClass  parent_class;
};


GType            gimp_canvas_limit_get_type        (void) G_GNUC_CONST;

GimpCanvasItem * gimp_canvas_limit_new             (GimpDisplayShell *shell,
                                                    GimpLimitType     type,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble           radius,
                                                    gdouble           aspect_ratio,
                                                    gdouble           angle,
                                                    gboolean          dashed);

void             gimp_canvas_limit_get_radii       (GimpCanvasLimit  *limit,
                                                    gdouble          *rx,
                                                    gdouble          *ry);

gboolean         gimp_canvas_limit_is_inside       (GimpCanvasLimit  *limit,
                                                    gdouble           x,
                                                    gdouble           y);
void             gimp_canvas_limit_boundary_point  (GimpCanvasLimit  *limit,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble          *bx,
                                                    gdouble          *by);
gdouble          gimp_canvas_limit_boundary_radius (GimpCanvasLimit  *limit,
                                                    gdouble           x,
                                                    gdouble           y);

void             gimp_canvas_limit_center_point    (GimpCanvasLimit  *limit,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble          *cx,
                                                    gdouble          *cy);


#endif /* __GIMP_CANVAS_LIMIT_H__ */
