/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererfont.h
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_VIEW_RENDERER_FONT            (gimp_view_renderer_font_get_type ())
#define GIMP_VIEW_RENDERER_FONT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VIEW_RENDERER_FONT, GimpViewRendererFont))
#define GIMP_VIEW_RENDERER_FONT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VIEW_RENDERER_FONT, GimpViewRendererFontClass))
#define GIMP_IS_VIEW_RENDERER_FONT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_VIEW_RENDERER_FONT))
#define GIMP_IS_VIEW_RENDERER_FONT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VIEW_RENDERER_FONT))
#define GIMP_VIEW_RENDERER_FONT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VIEW_RENDERER_FONT, GimpViewRendererFontClass))


typedef struct _GimpViewRendererFontClass  GimpViewRendererFontClass;

struct _GimpViewRendererFont
{
  GimpViewRenderer  parent_instance;
};

struct _GimpViewRendererFontClass
{
  GimpViewRendererClass  parent_class;
};


GType   gimp_view_renderer_font_get_type (void) G_GNUC_CONST;
