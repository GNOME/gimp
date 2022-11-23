/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvastransformguides.h
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

#ifndef __LIGMA_CANVAS_TRANSFORM_GUIDES_H__
#define __LIGMA_CANVAS_TRANSFORM_GUIDES_H__


#include "ligmacanvasitem.h"


#define LIGMA_TYPE_CANVAS_TRANSFORM_GUIDES            (ligma_canvas_transform_guides_get_type ())
#define LIGMA_CANVAS_TRANSFORM_GUIDES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_TRANSFORM_GUIDES, LigmaCanvasTransformGuides))
#define LIGMA_CANVAS_TRANSFORM_GUIDES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_TRANSFORM_GUIDES, LigmaCanvasTransformGuidesClass))
#define LIGMA_IS_CANVAS_TRANSFORM_GUIDES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_TRANSFORM_GUIDES))
#define LIGMA_IS_CANVAS_TRANSFORM_GUIDES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_TRANSFORM_GUIDES))
#define LIGMA_CANVAS_TRANSFORM_GUIDES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_TRANSFORM_GUIDES, LigmaCanvasTransformGuidesClass))


typedef struct _LigmaCanvasTransformGuides      LigmaCanvasTransformGuides;
typedef struct _LigmaCanvasTransformGuidesClass LigmaCanvasTransformGuidesClass;

struct _LigmaCanvasTransformGuides
{
  LigmaCanvasItem  parent_instance;
};

struct _LigmaCanvasTransformGuidesClass
{
  LigmaCanvasItemClass  parent_class;
};


GType            ligma_canvas_transform_guides_get_type (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_transform_guides_new      (LigmaDisplayShell  *shell,
                                                        const LigmaMatrix3 *transform,
                                                        gdouble            x1,
                                                        gdouble            y1,
                                                        gdouble            x2,
                                                        gdouble            y2,
                                                        LigmaGuidesType     type,
                                                        gint               n_guides,
                                                        gboolean           clip);

void             ligma_canvas_transform_guides_set      (LigmaCanvasItem    *guides,
                                                        const LigmaMatrix3 *transform,
                                                        gdouble            x1,
                                                        gdouble            y1,
                                                        gdouble            x2,
                                                        gdouble            y2,
                                                        LigmaGuidesType     type,
                                                        gint               n_guides,
                                                        gboolean           clip);


#endif /* __LIGMA_CANVAS_TRANSFORM_GUIDES_H__ */
