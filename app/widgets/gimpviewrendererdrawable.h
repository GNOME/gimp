/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrendererdrawable.h
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_VIEW_RENDERER_DRAWABLE_H__
#define __LIGMA_VIEW_RENDERER_DRAWABLE_H__

#include "ligmaviewrenderer.h"

#define LIGMA_TYPE_VIEW_RENDERER_DRAWABLE            (ligma_view_renderer_drawable_get_type ())
#define LIGMA_VIEW_RENDERER_DRAWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VIEW_RENDERER_DRAWABLE, LigmaViewRendererDrawable))
#define LIGMA_VIEW_RENDERER_DRAWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VIEW_RENDERER_DRAWABLE, LigmaViewRendererDrawableClass))
#define LIGMA_IS_VIEW_RENDERER_DRAWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_VIEW_RENDERER_DRAWABLE))
#define LIGMA_IS_VIEW_RENDERER_DRAWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VIEW_RENDERER_DRAWABLE))
#define LIGMA_VIEW_RENDERER_DRAWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VIEW_RENDERER_DRAWABLE, LigmaViewRendererDrawableClass))


typedef struct _LigmaViewRendererDrawablePrivate LigmaViewRendererDrawablePrivate;
typedef struct _LigmaViewRendererDrawableClass   LigmaViewRendererDrawableClass;

struct _LigmaViewRendererDrawable
{
  LigmaViewRenderer                 parent_instance;

  LigmaViewRendererDrawablePrivate *priv;
};

struct _LigmaViewRendererDrawableClass
{
  LigmaViewRendererClass  parent_class;
};


GType   ligma_view_renderer_drawable_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_VIEW_RENDERER_DRAWABLE_H__ */
