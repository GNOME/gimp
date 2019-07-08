/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererbrush.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_VIEW_RENDERER_BRUSH_H__
#define __GIMP_VIEW_RENDERER_BRUSH_H__

#include "gimpviewrenderer.h"

#define GIMP_TYPE_VIEW_RENDERER_BRUSH            (gimp_view_renderer_brush_get_type ())
#define GIMP_VIEW_RENDERER_BRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VIEW_RENDERER_BRUSH, GimpViewRendererBrush))
#define GIMP_VIEW_RENDERER_BRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VIEW_RENDERER_BRUSH, GimpViewRendererBrushClass))
#define GIMP_IS_VIEW_RENDERER_BRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_VIEW_RENDERER_BRUSH))
#define GIMP_IS_VIEW_RENDERER_BRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VIEW_RENDERER_BRUSH))
#define GIMP_VIEW_RENDERER_BRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VIEW_RENDERER_BRUSH, GimpViewRendererBrushClass))


typedef struct _GimpViewRendererBrushClass  GimpViewRendererBrushClass;

struct _GimpViewRendererBrush
{
  GimpViewRenderer parent_instance;

  guint            pipe_timeout_id;
  gint             pipe_animation_index;

  /* for the pipe render animation timeout */
  GtkWidget       *widget;
};

struct _GimpViewRendererBrushClass
{
  GimpViewRendererClass  parent_class;
};


GType   gimp_view_renderer_brush_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_VIEW_RENDERER_BRUSH_H__ */
