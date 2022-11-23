/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabezierstroke.h
 * Copyright (C) 2002 Simon Budig  <simon@ligma.org>
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

#ifndef __LIGMA_BEZIER_STROKE_H__
#define __LIGMA_BEZIER_STROKE_H__

#include "ligmastroke.h"


#define LIGMA_TYPE_BEZIER_STROKE            (ligma_bezier_stroke_get_type ())
#define LIGMA_BEZIER_STROKE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BEZIER_STROKE, LigmaBezierStroke))
#define LIGMA_BEZIER_STROKE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BEZIER_STROKE, LigmaBezierStrokeClass))
#define LIGMA_IS_BEZIER_STROKE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BEZIER_STROKE))
#define LIGMA_IS_BEZIER_STROKE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BEZIER_STROKE))
#define LIGMA_BEZIER_STROKE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BEZIER_STROKE, LigmaBezierStrokeClass))


typedef struct _LigmaBezierStrokeClass LigmaBezierStrokeClass;

struct _LigmaBezierStroke
{
  LigmaStroke  parent_instance;
};

struct _LigmaBezierStrokeClass
{
  LigmaStrokeClass  parent_class;
};


GType        ligma_bezier_stroke_get_type        (void) G_GNUC_CONST;

LigmaStroke * ligma_bezier_stroke_new             (void);
LigmaStroke * ligma_bezier_stroke_new_from_coords (const LigmaCoords *coords,
                                                 gint              n_coords,
                                                 gboolean          closed);

LigmaStroke * ligma_bezier_stroke_new_moveto      (const LigmaCoords *start);
void         ligma_bezier_stroke_lineto          (LigmaStroke       *bez_stroke,
                                                 const LigmaCoords *end);
void         ligma_bezier_stroke_conicto         (LigmaStroke       *bez_stroke,
                                                 const LigmaCoords *control,
                                                 const LigmaCoords *end);
void         ligma_bezier_stroke_cubicto         (LigmaStroke       *bez_stroke,
                                                 const LigmaCoords *control1,
                                                 const LigmaCoords *control2,
                                                 const LigmaCoords *end);
void         ligma_bezier_stroke_arcto           (LigmaStroke       *bez_stroke,
                                                 gdouble           radius_x,
                                                 gdouble           radius_y,
                                                 gdouble           angle_rad,
                                                 gboolean          large_arc,
                                                 gboolean          sweep,
                                                 const LigmaCoords *end);
LigmaStroke * ligma_bezier_stroke_new_ellipse     (const LigmaCoords *center,
                                                 gdouble           radius_x,
                                                 gdouble           radius_y,
                                                 gdouble           angle);


LigmaAnchor * ligma_bezier_stroke_extend      (LigmaStroke           *stroke,
                                             const LigmaCoords     *coords,
                                             LigmaAnchor           *neighbor,
                                             LigmaVectorExtendMode  extend_mode);


#endif /* __LIGMA_BEZIER_STROKE_H__ */
