/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrenderergradient.h
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

#ifndef __GIMP_VIEW_RENDERER_GRADIENT_H__
#define __GIMP_VIEW_RENDERER_GRADIENT_H__

#include "gimpviewrenderer.h"

#define GIMP_TYPE_VIEW_RENDERER_GRADIENT            (gimp_view_renderer_gradient_get_type ())
#define GIMP_VIEW_RENDERER_GRADIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VIEW_RENDERER_GRADIENT, GimpViewRendererGradient))
#define GIMP_VIEW_RENDERER_GRADIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VIEW_RENDERER_GRADIENT, GimpViewRendererGradientClass))
#define GIMP_IS_VIEW_RENDERER_GRADIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_VIEW_RENDERER_GRADIENT))
#define GIMP_IS_VIEW_RENDERER_GRADIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VIEW_RENDERER_GRADIENT))
#define GIMP_VIEW_RENDERER_GRADIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VIEW_RENDERER_GRADIENT, GimpViewRendererGradientClass))


typedef struct _GimpViewRendererGradientClass  GimpViewRendererGradientClass;

struct _GimpViewRendererGradient
{
  GimpViewRenderer            parent_instance;

  gdouble                     left;
  gdouble                     right;
  gboolean                    reverse;
  GimpGradientBlendColorSpace blend_color_space;
  gboolean                    has_fg_bg_segments;
};

struct _GimpViewRendererGradientClass
{
  GimpViewRendererClass  parent_class;
};


GType   gimp_view_renderer_gradient_get_type    (void) G_GNUC_CONST;

void    gimp_view_renderer_gradient_set_offsets (GimpViewRendererGradient    *renderer,
                                                 gdouble                      left,
                                                 gdouble                      right);
void    gimp_view_renderer_gradient_set_reverse (GimpViewRendererGradient    *renderer,
                                                 gboolean                     reverse);
void    gimp_view_renderer_gradient_set_blend_color_space
                                                (GimpViewRendererGradient    *renderer,
                                                 GimpGradientBlendColorSpace  blend_color_space);


#endif /* __GIMP_VIEW_RENDERER_GRADIENT_H__ */
