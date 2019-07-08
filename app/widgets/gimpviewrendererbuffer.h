/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererbuffer.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_VIEW_RENDERER_BUFFER_H__
#define __GIMP_VIEW_RENDERER_BUFFER_H__

#include "gimpviewrenderer.h"

#define GIMP_TYPE_VIEW_RENDERER_BUFFER            (gimp_view_renderer_buffer_get_type ())
#define GIMP_VIEW_RENDERER_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VIEW_RENDERER_BUFFER, GimpViewRendererBuffer))
#define GIMP_VIEW_RENDERER_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VIEW_RENDERER_BUFFER, GimpViewRendererBufferClass))
#define GIMP_IS_VIEW_RENDERER_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_VIEW_RENDERER_BUFFER))
#define GIMP_IS_VIEW_RENDERER_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VIEW_RENDERER_BUFFER))
#define GIMP_VIEW_RENDERER_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VIEW_RENDERER_BUFFER, GimpViewRendererBufferClass))


typedef struct _GimpViewRendererBufferClass  GimpViewRendererBufferClass;

struct _GimpViewRendererBuffer
{
  GimpViewRenderer  parent_instance;
};

struct _GimpViewRendererBufferClass
{
  GimpViewRendererClass  parent_class;
};


GType   gimp_view_renderer_buffer_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_VIEW_RENDERER_BUFFER_H__ */
