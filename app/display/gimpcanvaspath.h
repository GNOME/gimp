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

#ifndef __LIGMA_CANVAS_PATH_H__
#define __LIGMA_CANVAS_PATH_H__


#include "ligmacanvasitem.h"


#define LIGMA_TYPE_CANVAS_PATH            (ligma_canvas_path_get_type ())
#define LIGMA_CANVAS_PATH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_PATH, LigmaCanvasPath))
#define LIGMA_CANVAS_PATH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_PATH, LigmaCanvasPathClass))
#define LIGMA_IS_CANVAS_PATH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_PATH))
#define LIGMA_IS_CANVAS_PATH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_PATH))
#define LIGMA_CANVAS_PATH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_PATH, LigmaCanvasPathClass))


typedef struct _LigmaCanvasPath      LigmaCanvasPath;
typedef struct _LigmaCanvasPathClass LigmaCanvasPathClass;

struct _LigmaCanvasPath
{
  LigmaCanvasItem  parent_instance;
};

struct _LigmaCanvasPathClass
{
  LigmaCanvasItemClass  parent_class;
};


GType            ligma_canvas_path_get_type (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_path_new      (LigmaDisplayShell     *shell,
                                            const LigmaBezierDesc *bezier,
                                            gdouble               x,
                                            gdouble               y,
                                            gboolean              filled,
                                            LigmaPathStyle         style);

void             ligma_canvas_path_set      (LigmaCanvasItem       *path,
                                            const LigmaBezierDesc *bezier);


#endif /* __LIGMA_CANVAS_PATH_H__ */
