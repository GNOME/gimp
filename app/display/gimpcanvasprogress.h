/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasprogress.h
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

#ifndef __LIGMA_CANVAS_PROGRESS_H__
#define __LIGMA_CANVAS_PROGRESS_H__


#include "ligmacanvasitem.h"


#define LIGMA_TYPE_CANVAS_PROGRESS            (ligma_canvas_progress_get_type ())
#define LIGMA_CANVAS_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_PROGRESS, LigmaCanvasProgress))
#define LIGMA_CANVAS_PROGRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_PROGRESS, LigmaCanvasProgressClass))
#define LIGMA_IS_CANVAS_PROGRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_PROGRESS))
#define LIGMA_IS_CANVAS_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_PROGRESS))
#define LIGMA_CANVAS_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_PROGRESS, LigmaCanvasProgressClass))


typedef struct _LigmaCanvasProgress      LigmaCanvasProgress;
typedef struct _LigmaCanvasProgressClass LigmaCanvasProgressClass;

struct _LigmaCanvasProgress
{
  LigmaCanvasItem  parent_instance;
};

struct _LigmaCanvasProgressClass
{
  LigmaCanvasItemClass  parent_class;
};


GType            ligma_canvas_progress_get_type (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_progress_new      (LigmaDisplayShell *shell,
                                                LigmaHandleAnchor  anchor,
                                                gdouble           x,
                                                gdouble           y);


#endif /* __LIGMA_CANVAS_PROGRESS_H__ */
