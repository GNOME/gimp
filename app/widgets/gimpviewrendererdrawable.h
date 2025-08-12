/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererdrawable.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "gimpviewrenderer.h"


#define GIMP_TYPE_VIEW_RENDERER_DRAWABLE            (gimp_view_renderer_drawable_get_type ())
#define GIMP_VIEW_RENDERER_DRAWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VIEW_RENDERER_DRAWABLE, GimpViewRendererDrawable))
#define GIMP_VIEW_RENDERER_DRAWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VIEW_RENDERER_DRAWABLE, GimpViewRendererDrawableClass))
#define GIMP_IS_VIEW_RENDERER_DRAWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_VIEW_RENDERER_DRAWABLE))
#define GIMP_IS_VIEW_RENDERER_DRAWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VIEW_RENDERER_DRAWABLE))
#define GIMP_VIEW_RENDERER_DRAWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VIEW_RENDERER_DRAWABLE, GimpViewRendererDrawableClass))


typedef struct _GimpViewRendererDrawablePrivate GimpViewRendererDrawablePrivate;
typedef struct _GimpViewRendererDrawableClass   GimpViewRendererDrawableClass;

struct _GimpViewRendererDrawable
{
  GimpViewRenderer                 parent_instance;

  GimpViewRendererDrawablePrivate *priv;
};

struct _GimpViewRendererDrawableClass
{
  GimpViewRendererClass  parent_class;
};


GType   gimp_view_renderer_drawable_get_type (void) G_GNUC_CONST;
