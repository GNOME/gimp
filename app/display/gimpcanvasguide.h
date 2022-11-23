/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasguide.h
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

#ifndef __LIGMA_CANVAS_GUIDE_H__
#define __LIGMA_CANVAS_GUIDE_H__


#include "ligmacanvasitem.h"


#define LIGMA_TYPE_CANVAS_GUIDE            (ligma_canvas_guide_get_type ())
#define LIGMA_CANVAS_GUIDE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_GUIDE, LigmaCanvasGuide))
#define LIGMA_CANVAS_GUIDE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_GUIDE, LigmaCanvasGuideClass))
#define LIGMA_IS_CANVAS_GUIDE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_GUIDE))
#define LIGMA_IS_CANVAS_GUIDE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_GUIDE))
#define LIGMA_CANVAS_GUIDE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_GUIDE, LigmaCanvasGuideClass))


typedef struct _LigmaCanvasGuide      LigmaCanvasGuide;
typedef struct _LigmaCanvasGuideClass LigmaCanvasGuideClass;

struct _LigmaCanvasGuide
{
  LigmaCanvasItem  parent_instance;
};

struct _LigmaCanvasGuideClass
{
  LigmaCanvasItemClass  parent_class;
};


GType            ligma_canvas_guide_get_type (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_guide_new      (LigmaDisplayShell    *shell,
                                             LigmaOrientationType  orientation,
                                             gint                 position,
                                             LigmaGuideStyle       style);

void             ligma_canvas_guide_set      (LigmaCanvasItem      *guide,
                                             LigmaOrientationType  orientation,
                                             gint                 position);


#endif /* __LIGMA_CANVAS_GUIDE_H__ */
