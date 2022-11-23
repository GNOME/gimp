/* LIGMA - The GNU Image Manipulation Program Copyright (C) 1995
 * Spencer Kimball and Peter Mattis
 *
 * ligmacanvaspen.h
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

#ifndef __LIGMA_CANVAS_PEN_H__
#define __LIGMA_CANVAS_PEN_H__


#include "ligmacanvaspolygon.h"


#define LIGMA_TYPE_CANVAS_PEN            (ligma_canvas_pen_get_type ())
#define LIGMA_CANVAS_PEN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_PEN, LigmaCanvasPen))
#define LIGMA_CANVAS_PEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_PEN, LigmaCanvasPenClass))
#define LIGMA_IS_CANVAS_PEN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_PEN))
#define LIGMA_IS_CANVAS_PEN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_PEN))
#define LIGMA_CANVAS_PEN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_PEN, LigmaCanvasPenClass))


typedef struct _LigmaCanvasPen      LigmaCanvasPen;
typedef struct _LigmaCanvasPenClass LigmaCanvasPenClass;

struct _LigmaCanvasPen
{
  LigmaCanvasPolygon  parent_instance;
};

struct _LigmaCanvasPenClass
{
  LigmaCanvasPolygonClass  parent_class;
};


GType            ligma_canvas_pen_get_type (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_pen_new      (LigmaDisplayShell  *shell,
                                           const LigmaVector2 *points,
                                           gint               n_points,
                                           LigmaContext       *context,
                                           LigmaActiveColor    color,
                                           gint               width);


#endif /* __LIGMA_CANVAS_PEN_H__ */
