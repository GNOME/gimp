/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrendererbuffer.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_VIEW_RENDERER_BUFFER_H__
#define __LIGMA_VIEW_RENDERER_BUFFER_H__

#include "ligmaviewrenderer.h"

#define LIGMA_TYPE_VIEW_RENDERER_BUFFER            (ligma_view_renderer_buffer_get_type ())
#define LIGMA_VIEW_RENDERER_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VIEW_RENDERER_BUFFER, LigmaViewRendererBuffer))
#define LIGMA_VIEW_RENDERER_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VIEW_RENDERER_BUFFER, LigmaViewRendererBufferClass))
#define LIGMA_IS_VIEW_RENDERER_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_VIEW_RENDERER_BUFFER))
#define LIGMA_IS_VIEW_RENDERER_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VIEW_RENDERER_BUFFER))
#define LIGMA_VIEW_RENDERER_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VIEW_RENDERER_BUFFER, LigmaViewRendererBufferClass))


typedef struct _LigmaViewRendererBufferClass  LigmaViewRendererBufferClass;

struct _LigmaViewRendererBuffer
{
  LigmaViewRenderer  parent_instance;
};

struct _LigmaViewRendererBufferClass
{
  LigmaViewRendererClass  parent_class;
};


GType   ligma_view_renderer_buffer_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_VIEW_RENDERER_BUFFER_H__ */
