/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvaslimit.h
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

#ifndef __LIGMA_CANVAS_LIMIT_H__
#define __LIGMA_CANVAS_LIMIT_H__


#include "ligmacanvasitem.h"


#define LIGMA_TYPE_CANVAS_LIMIT            (ligma_canvas_limit_get_type ())
#define LIGMA_CANVAS_LIMIT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_LIMIT, LigmaCanvasLimit))
#define LIGMA_CANVAS_LIMIT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_LIMIT, LigmaCanvasLimitClass))
#define LIGMA_IS_CANVAS_LIMIT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_LIMIT))
#define LIGMA_IS_CANVAS_LIMIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_LIMIT))
#define LIGMA_CANVAS_LIMIT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_LIMIT, LigmaCanvasLimitClass))


typedef struct _LigmaCanvasLimit      LigmaCanvasLimit;
typedef struct _LigmaCanvasLimitClass LigmaCanvasLimitClass;

struct _LigmaCanvasLimit
{
  LigmaCanvasItem  parent_instance;
};

struct _LigmaCanvasLimitClass
{
  LigmaCanvasItemClass  parent_class;
};


GType            ligma_canvas_limit_get_type        (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_limit_new             (LigmaDisplayShell *shell,
                                                    LigmaLimitType     type,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble           radius,
                                                    gdouble           aspect_ratio,
                                                    gdouble           angle,
                                                    gboolean          dashed);

void             ligma_canvas_limit_get_radii       (LigmaCanvasLimit  *limit,
                                                    gdouble          *rx,
                                                    gdouble          *ry);

gboolean         ligma_canvas_limit_is_inside       (LigmaCanvasLimit  *limit,
                                                    gdouble           x,
                                                    gdouble           y);
void             ligma_canvas_limit_boundary_point  (LigmaCanvasLimit  *limit,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble          *bx,
                                                    gdouble          *by);
gdouble          ligma_canvas_limit_boundary_radius (LigmaCanvasLimit  *limit,
                                                    gdouble           x,
                                                    gdouble           y);

void             ligma_canvas_limit_center_point    (LigmaCanvasLimit  *limit,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble          *cx,
                                                    gdouble          *cy);


#endif /* __LIGMA_CANVAS_LIMIT_H__ */
