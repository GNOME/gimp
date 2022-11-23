/* LIGMA - The GNU Image Manipulation Program Copyright (C) 1995
 * Spencer Kimball and Peter Mattis
 *
 * ligmacanvaspolygon.h
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CANVAS_POLYGON_H__
#define __LIGMA_CANVAS_POLYGON_H__


#include "ligmacanvasitem.h"


#define LIGMA_TYPE_CANVAS_POLYGON            (ligma_canvas_polygon_get_type ())
#define LIGMA_CANVAS_POLYGON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_POLYGON, LigmaCanvasPolygon))
#define LIGMA_CANVAS_POLYGON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_POLYGON, LigmaCanvasPolygonClass))
#define LIGMA_IS_CANVAS_POLYGON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_POLYGON))
#define LIGMA_IS_CANVAS_POLYGON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_POLYGON))
#define LIGMA_CANVAS_POLYGON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_POLYGON, LigmaCanvasPolygonClass))


typedef struct _LigmaCanvasPolygon      LigmaCanvasPolygon;
typedef struct _LigmaCanvasPolygonClass LigmaCanvasPolygonClass;

struct _LigmaCanvasPolygon
{
  LigmaCanvasItem  parent_instance;
};

struct _LigmaCanvasPolygonClass
{
  LigmaCanvasItemClass  parent_class;
};


GType            ligma_canvas_polygon_get_type        (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_polygon_new             (LigmaDisplayShell  *shell,
                                                      const LigmaVector2 *points,
                                                      gint               n_points,
                                                      LigmaMatrix3       *transform,
                                                      gboolean           filled);
LigmaCanvasItem * ligma_canvas_polygon_new_from_coords (LigmaDisplayShell  *shell,
                                                      const LigmaCoords  *coords,
                                                      gint               n_coords,
                                                      LigmaMatrix3       *transform,
                                                      gboolean           filled);

void             ligma_canvas_polygon_set_points      (LigmaCanvasItem    *polygon,
                                                      const LigmaVector2 *points,
                                                      gint               n_points);


#endif /* __LIGMA_CANVAS_POLYGON_H__ */
