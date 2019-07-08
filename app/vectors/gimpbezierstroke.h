/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbezierstroke.h
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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

#ifndef __GIMP_BEZIER_STROKE_H__
#define __GIMP_BEZIER_STROKE_H__

#include "gimpstroke.h"


#define GIMP_TYPE_BEZIER_STROKE            (gimp_bezier_stroke_get_type ())
#define GIMP_BEZIER_STROKE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BEZIER_STROKE, GimpBezierStroke))
#define GIMP_BEZIER_STROKE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BEZIER_STROKE, GimpBezierStrokeClass))
#define GIMP_IS_BEZIER_STROKE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BEZIER_STROKE))
#define GIMP_IS_BEZIER_STROKE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BEZIER_STROKE))
#define GIMP_BEZIER_STROKE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BEZIER_STROKE, GimpBezierStrokeClass))


typedef struct _GimpBezierStrokeClass GimpBezierStrokeClass;

struct _GimpBezierStroke
{
  GimpStroke  parent_instance;
};

struct _GimpBezierStrokeClass
{
  GimpStrokeClass  parent_class;
};


GType        gimp_bezier_stroke_get_type        (void) G_GNUC_CONST;

GimpStroke * gimp_bezier_stroke_new             (void);
GimpStroke * gimp_bezier_stroke_new_from_coords (const GimpCoords *coords,
                                                 gint              n_coords,
                                                 gboolean          closed);

GimpStroke * gimp_bezier_stroke_new_moveto      (const GimpCoords *start);
void         gimp_bezier_stroke_lineto          (GimpStroke       *bez_stroke,
                                                 const GimpCoords *end);
void         gimp_bezier_stroke_conicto         (GimpStroke       *bez_stroke,
                                                 const GimpCoords *control,
                                                 const GimpCoords *end);
void         gimp_bezier_stroke_cubicto         (GimpStroke       *bez_stroke,
                                                 const GimpCoords *control1,
                                                 const GimpCoords *control2,
                                                 const GimpCoords *end);
void         gimp_bezier_stroke_arcto           (GimpStroke       *bez_stroke,
                                                 gdouble           radius_x,
                                                 gdouble           radius_y,
                                                 gdouble           angle_rad,
                                                 gboolean          large_arc,
                                                 gboolean          sweep,
                                                 const GimpCoords *end);
GimpStroke * gimp_bezier_stroke_new_ellipse     (const GimpCoords *center,
                                                 gdouble           radius_x,
                                                 gdouble           radius_y,
                                                 gdouble           angle);


GimpAnchor * gimp_bezier_stroke_extend      (GimpStroke           *stroke,
                                             const GimpCoords     *coords,
                                             GimpAnchor           *neighbor,
                                             GimpVectorExtendMode  extend_mode);


#endif /* __GIMP_BEZIER_STROKE_H__ */
