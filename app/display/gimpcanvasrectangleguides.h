/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasrectangleguides.h
 * Copyright (C) 2011 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CANVAS_RECTANGLE_GUIDES_H__
#define __LIGMA_CANVAS_RECTANGLE_GUIDES_H__


#include "ligmacanvasitem.h"


#define LIGMA_TYPE_CANVAS_RECTANGLE_GUIDES            (ligma_canvas_rectangle_guides_get_type ())
#define LIGMA_CANVAS_RECTANGLE_GUIDES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_RECTANGLE_GUIDES, LigmaCanvasRectangleGuides))
#define LIGMA_CANVAS_RECTANGLE_GUIDES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_RECTANGLE_GUIDES, LigmaCanvasRectangleGuidesClass))
#define LIGMA_IS_CANVAS_RECTANGLE_GUIDES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_RECTANGLE_GUIDES))
#define LIGMA_IS_CANVAS_RECTANGLE_GUIDES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_RECTANGLE_GUIDES))
#define LIGMA_CANVAS_RECTANGLE_GUIDES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_RECTANGLE_GUIDES, LigmaCanvasRectangleGuidesClass))


typedef struct _LigmaCanvasRectangleGuides      LigmaCanvasRectangleGuides;
typedef struct _LigmaCanvasRectangleGuidesClass LigmaCanvasRectangleGuidesClass;

struct _LigmaCanvasRectangleGuides
{
  LigmaCanvasItem  parent_instance;
};

struct _LigmaCanvasRectangleGuidesClass
{
  LigmaCanvasItemClass  parent_class;
};


GType            ligma_canvas_rectangle_guides_get_type (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_rectangle_guides_new      (LigmaDisplayShell *shell,
                                                        gdouble           x,
                                                        gdouble           y,
                                                        gdouble           width,
                                                        gdouble           height,
                                                        LigmaGuidesType    type,
                                                        gint              n_guides);

void             ligma_canvas_rectangle_guides_set      (LigmaCanvasItem   *rectangle,
                                                        gdouble           x,
                                                        gdouble           y,
                                                        gdouble           width,
                                                        gdouble           height,
                                                        LigmaGuidesType    type,
                                                        gint              n_guides);


#endif /* __LIGMA_CANVAS_RECTANGLE_GUIDES_H__ */
