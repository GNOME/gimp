/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrenderergradient.c
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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmagradient.h"

#include "ligmaviewrenderergradient.h"


static void   ligma_view_renderer_gradient_set_context (LigmaViewRenderer *renderer,
                                                       LigmaContext      *context);
static void   ligma_view_renderer_gradient_invalidate  (LigmaViewRenderer *renderer);
static void   ligma_view_renderer_gradient_render      (LigmaViewRenderer *renderer,
                                                       GtkWidget        *widget);


G_DEFINE_TYPE (LigmaViewRendererGradient, ligma_view_renderer_gradient,
               LIGMA_TYPE_VIEW_RENDERER);

#define parent_class ligma_view_renderer_gradient_parent_class


static void
ligma_view_renderer_gradient_class_init (LigmaViewRendererGradientClass *klass)
{
  LigmaViewRendererClass *renderer_class = LIGMA_VIEW_RENDERER_CLASS (klass);

  renderer_class->set_context = ligma_view_renderer_gradient_set_context;
  renderer_class->invalidate  = ligma_view_renderer_gradient_invalidate;
  renderer_class->render      = ligma_view_renderer_gradient_render;
}

static void
ligma_view_renderer_gradient_init (LigmaViewRendererGradient *renderer)
{
  renderer->left  = 0.0;
  renderer->right = 1.0;
}

static void
ligma_view_renderer_gradient_fg_bg_changed (LigmaContext      *context,
                                           const LigmaRGB    *color,
                                           LigmaViewRenderer *renderer)
{
#if 0
  g_printerr ("%s: invalidating %s\n", G_STRFUNC,
              ligma_object_get_name (renderer->viewable));
#endif

  ligma_view_renderer_invalidate (renderer);
}

static void
ligma_view_renderer_gradient_set_context (LigmaViewRenderer *renderer,
                                         LigmaContext      *context)
{
  LigmaViewRendererGradient *rendergrad;

  rendergrad = LIGMA_VIEW_RENDERER_GRADIENT (renderer);

  if (renderer->context && rendergrad->has_fg_bg_segments)
    {
      g_signal_handlers_disconnect_by_func (renderer->context,
                                            ligma_view_renderer_gradient_fg_bg_changed,
                                            renderer);
    }

  LIGMA_VIEW_RENDERER_CLASS (parent_class)->set_context (renderer, context);

  if (renderer->context && rendergrad->has_fg_bg_segments)
    {
      g_signal_connect (renderer->context, "foreground-changed",
                        G_CALLBACK (ligma_view_renderer_gradient_fg_bg_changed),
                        renderer);
      g_signal_connect (renderer->context, "background-changed",
                        G_CALLBACK (ligma_view_renderer_gradient_fg_bg_changed),
                        renderer);

      ligma_view_renderer_gradient_fg_bg_changed (renderer->context,
                                                 NULL,
                                                 renderer);
    }
}

static void
ligma_view_renderer_gradient_invalidate (LigmaViewRenderer *renderer)
{
  LigmaViewRendererGradient *rendergrad;
  gboolean                  has_fg_bg_segments = FALSE;

  rendergrad = LIGMA_VIEW_RENDERER_GRADIENT (renderer);

  if (renderer->viewable)
    has_fg_bg_segments =
      ligma_gradient_has_fg_bg_segments (LIGMA_GRADIENT (renderer->viewable));

  if (rendergrad->has_fg_bg_segments != has_fg_bg_segments)
    {
      if (renderer->context)
        {
          if (rendergrad->has_fg_bg_segments)
            {
              g_signal_handlers_disconnect_by_func (renderer->context,
                                                    ligma_view_renderer_gradient_fg_bg_changed,
                                                    renderer);
            }
          else
            {
              g_signal_connect (renderer->context, "foreground-changed",
                                G_CALLBACK (ligma_view_renderer_gradient_fg_bg_changed),
                                renderer);
              g_signal_connect (renderer->context, "background-changed",
                                G_CALLBACK (ligma_view_renderer_gradient_fg_bg_changed),
                                renderer);
            }
        }

      rendergrad->has_fg_bg_segments = has_fg_bg_segments;
    }

  LIGMA_VIEW_RENDERER_CLASS (parent_class)->invalidate (renderer);
}

static void
ligma_view_renderer_gradient_render (LigmaViewRenderer *renderer,
                                    GtkWidget        *widget)
{
  LigmaViewRendererGradient *rendergrad = LIGMA_VIEW_RENDERER_GRADIENT (renderer);
  LigmaGradient             *gradient   = LIGMA_GRADIENT (renderer->viewable);
  LigmaGradientSegment      *seg        = NULL;
  LigmaColorTransform       *transform;
  guchar                   *buf;
  guchar                   *dest;
  gint                      dest_stride;
  gint                      x;
  gint                      y;
  gdouble                   dx, cur_x;
  LigmaRGB                   color;

  buf   = g_alloca (4 * renderer->width);
  dx    = (rendergrad->right - rendergrad->left) / (renderer->width - 1);
  cur_x = rendergrad->left;

  for (x = 0, dest = buf; x < renderer->width; x++, dest += 4)
    {
      guchar r, g, b, a;

      seg = ligma_gradient_get_color_at (gradient, renderer->context, seg,
                                        cur_x,
                                        rendergrad->reverse,
                                        rendergrad->blend_color_space,
                                        &color);
      cur_x += dx;

      ligma_rgba_get_uchar (&color, &r, &g, &b, &a);

      LIGMA_CAIRO_ARGB32_SET_PIXEL (dest, r, g, b, a);
    }

  if (! renderer->surface)
    renderer->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                    renderer->width,
                                                    renderer->height);

  cairo_surface_flush (renderer->surface);

  dest        = cairo_image_surface_get_data (renderer->surface);
  dest_stride = cairo_image_surface_get_stride (renderer->surface);

  transform = ligma_view_renderer_get_color_transform (renderer, widget,
                                                      babl_format ("cairo-ARGB32"),
                                                      babl_format ("cairo-ARGB32"));

  if (transform)
    ligma_color_transform_process_pixels (transform,
                                         babl_format ("cairo-ARGB32"), buf,
                                         babl_format ("cairo-ARGB32"), buf,
                                         renderer->width);

  for (y = 0; y < renderer->height; y++, dest += dest_stride)
    {
      memcpy (dest, buf, renderer->width * 4);
    }

  cairo_surface_mark_dirty (renderer->surface);
}

void
ligma_view_renderer_gradient_set_offsets (LigmaViewRendererGradient *renderer,
                                         gdouble                   left,
                                         gdouble                   right)
{
  g_return_if_fail (LIGMA_IS_VIEW_RENDERER_GRADIENT (renderer));

  left  = CLAMP (left, 0.0, 1.0);
  right = CLAMP (right, left, 1.0);

  if (left != renderer->left || right != renderer->right)
    {
      renderer->left  = left;
      renderer->right = right;

      ligma_view_renderer_invalidate (LIGMA_VIEW_RENDERER (renderer));
    }
}

void
ligma_view_renderer_gradient_set_reverse (LigmaViewRendererGradient *renderer,
                                         gboolean                  reverse)
{
  g_return_if_fail (LIGMA_IS_VIEW_RENDERER_GRADIENT (renderer));

  if (reverse != renderer->reverse)
    {
      renderer->reverse = reverse ? TRUE : FALSE;

      ligma_view_renderer_invalidate (LIGMA_VIEW_RENDERER (renderer));
      ligma_view_renderer_update (LIGMA_VIEW_RENDERER (renderer));
    }
}

void
ligma_view_renderer_gradient_set_blend_color_space (LigmaViewRendererGradient    *renderer,
                                                   LigmaGradientBlendColorSpace  blend_color_space)
{
  g_return_if_fail (LIGMA_IS_VIEW_RENDERER_GRADIENT (renderer));

  if (blend_color_space != renderer->blend_color_space)
    {
      renderer->blend_color_space = blend_color_space;

      ligma_view_renderer_invalidate (LIGMA_VIEW_RENDERER (renderer));
      ligma_view_renderer_update (LIGMA_VIEW_RENDERER (renderer));
    }
}
