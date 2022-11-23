/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvascorner.h
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

#ifndef __LIGMA_CANVAS_CORNER_H__
#define __LIGMA_CANVAS_CORNER_H__


#include "ligmacanvasitem.h"


#define LIGMA_TYPE_CANVAS_CORNER            (ligma_canvas_corner_get_type ())
#define LIGMA_CANVAS_CORNER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_CORNER, LigmaCanvasCorner))
#define LIGMA_CANVAS_CORNER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_CORNER, LigmaCanvasCornerClass))
#define LIGMA_IS_CANVAS_CORNER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_CORNER))
#define LIGMA_IS_CANVAS_CORNER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_CORNER))
#define LIGMA_CANVAS_CORNER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_CORNER, LigmaCanvasCornerClass))


typedef struct _LigmaCanvasCorner      LigmaCanvasCorner;
typedef struct _LigmaCanvasCornerClass LigmaCanvasCornerClass;

struct _LigmaCanvasCorner
{
  LigmaCanvasItem  parent_instance;
};

struct _LigmaCanvasCornerClass
{
  LigmaCanvasItemClass  parent_class;
};


GType            ligma_canvas_corner_get_type (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_corner_new      (LigmaDisplayShell *shell,
                                              gdouble           x,
                                              gdouble           y,
                                              gdouble           width,
                                              gdouble           height,
                                              LigmaHandleAnchor  anchor,
                                              gint              corner_width,
                                              gint              corner_height,
                                              gboolean          outside);

void             ligma_canvas_corner_set      (LigmaCanvasItem   *corner,
                                              gdouble           x,
                                              gdouble           y,
                                              gdouble           width,
                                              gdouble           height,
                                              gint              corner_width,
                                              gint              corner_height,
                                              gboolean          outside);


#endif /* __LIGMA_CANVAS_CORNER_H__ */
