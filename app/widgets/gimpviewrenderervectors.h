/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrenderervectors.h
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
 *                    Simon Budig <simon@ligma.org>
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

#ifndef __LIGMA_VIEW_RENDERER_VECTORS_H__
#define __LIGMA_VIEW_RENDERER_VECTORS_H__

#include "ligmaviewrenderer.h"

#define LIGMA_TYPE_VIEW_RENDERER_VECTORS            (ligma_view_renderer_vectors_get_type ())
#define LIGMA_VIEW_RENDERER_VECTORS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VIEW_RENDERER_VECTORS, LigmaViewRendererVectors))
#define LIGMA_VIEW_RENDERER_VECTORS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VIEW_RENDERER_VECTORS, LigmaViewRendererVectorsClass))
#define LIGMA_IS_VIEW_RENDERER_VECTORS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_VIEW_RENDERER_VECTORS))
#define LIGMA_IS_VIEW_RENDERER_VECTORS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VIEW_RENDERER_VECTORS))
#define LIGMA_VIEW_RENDERER_VECTORS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VIEW_RENDERER_VECTORS, LigmaViewRendererVectorsClass))


typedef struct _LigmaViewRendererVectorsClass  LigmaViewRendererVectorsClass;

struct _LigmaViewRendererVectors
{
  LigmaViewRenderer  parent_instance;
};

struct _LigmaViewRendererVectorsClass
{
  LigmaViewRendererClass  parent_class;
};


GType   ligma_view_renderer_vectors_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_VIEW_RENDERER_VECTORS_H__ */
