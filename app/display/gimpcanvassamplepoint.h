/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvassamplepoint.h
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

#ifndef __LIGMA_CANVAS_SAMPLE_POINT_H__
#define __LIGMA_CANVAS_SAMPLE_POINT_H__


#include "ligmacanvasitem.h"


#define LIGMA_TYPE_CANVAS_SAMPLE_POINT            (ligma_canvas_sample_point_get_type ())
#define LIGMA_CANVAS_SAMPLE_POINT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_SAMPLE_POINT, LigmaCanvasSamplePoint))
#define LIGMA_CANVAS_SAMPLE_POINT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_SAMPLE_POINT, LigmaCanvasSamplePointClass))
#define LIGMA_IS_CANVAS_SAMPLE_POINT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_SAMPLE_POINT))
#define LIGMA_IS_CANVAS_SAMPLE_POINT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_SAMPLE_POINT))
#define LIGMA_CANVAS_SAMPLE_POINT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_SAMPLE_POINT, LigmaCanvasSamplePointClass))


typedef struct _LigmaCanvasSamplePoint      LigmaCanvasSamplePoint;
typedef struct _LigmaCanvasSamplePointClass LigmaCanvasSamplePointClass;

struct _LigmaCanvasSamplePoint
{
  LigmaCanvasItem  parent_instance;
};

struct _LigmaCanvasSamplePointClass
{
  LigmaCanvasItemClass  parent_class;
};


GType            ligma_canvas_sample_point_get_type (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_sample_point_new      (LigmaDisplayShell *shell,
                                                    gint              x,
                                                    gint              y,
                                                    gint              index,
                                                    gboolean          sample_point_style);

void             ligma_canvas_sample_point_set      (LigmaCanvasItem   *sample_point,
                                                    gint              x,
                                                    gint              y);


#endif /* __LIGMA_CANVAS_SAMPLE_POINT_H__ */
