/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrenderergradient.h
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_VIEW_RENDERER_GRADIENT_H__
#define __LIGMA_VIEW_RENDERER_GRADIENT_H__

#include "ligmaviewrenderer.h"

#define LIGMA_TYPE_VIEW_RENDERER_GRADIENT            (ligma_view_renderer_gradient_get_type ())
#define LIGMA_VIEW_RENDERER_GRADIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VIEW_RENDERER_GRADIENT, LigmaViewRendererGradient))
#define LIGMA_VIEW_RENDERER_GRADIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VIEW_RENDERER_GRADIENT, LigmaViewRendererGradientClass))
#define LIGMA_IS_VIEW_RENDERER_GRADIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_VIEW_RENDERER_GRADIENT))
#define LIGMA_IS_VIEW_RENDERER_GRADIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VIEW_RENDERER_GRADIENT))
#define LIGMA_VIEW_RENDERER_GRADIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VIEW_RENDERER_GRADIENT, LigmaViewRendererGradientClass))


typedef struct _LigmaViewRendererGradientClass  LigmaViewRendererGradientClass;

struct _LigmaViewRendererGradient
{
  LigmaViewRenderer            parent_instance;

  gdouble                     left;
  gdouble                     right;
  gboolean                    reverse;
  LigmaGradientBlendColorSpace blend_color_space;
  gboolean                    has_fg_bg_segments;
};

struct _LigmaViewRendererGradientClass
{
  LigmaViewRendererClass  parent_class;
};


GType   ligma_view_renderer_gradient_get_type    (void) G_GNUC_CONST;

void    ligma_view_renderer_gradient_set_offsets (LigmaViewRendererGradient    *renderer,
                                                 gdouble                      left,
                                                 gdouble                      right);
void    ligma_view_renderer_gradient_set_reverse (LigmaViewRendererGradient    *renderer,
                                                 gboolean                     reverse);
void    ligma_view_renderer_gradient_set_blend_color_space
                                                (LigmaViewRendererGradient    *renderer,
                                                 LigmaGradientBlendColorSpace  blend_color_space);


#endif /* __LIGMA_VIEW_RENDERER_GRADIENT_H__ */
