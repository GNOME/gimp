/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrenderergradient.c
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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpgradient.h"

#include "gimpviewrenderergradient.h"


static void   gimp_view_renderer_gradient_set_context (GimpViewRenderer *renderer,
                                                       GimpContext      *context);
static void   gimp_view_renderer_gradient_invalidate  (GimpViewRenderer *renderer);
static void   gimp_view_renderer_gradient_render      (GimpViewRenderer *renderer,
                                                       GtkWidget        *widget);


G_DEFINE_TYPE (GimpViewRendererGradient, gimp_view_renderer_gradient,
               GIMP_TYPE_VIEW_RENDERER);

#define parent_class gimp_view_renderer_gradient_parent_class


static void
gimp_view_renderer_gradient_class_init (GimpViewRendererGradientClass *klass)
{
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  renderer_class->set_context = gimp_view_renderer_gradient_set_context;
  renderer_class->invalidate  = gimp_view_renderer_gradient_invalidate;
  renderer_class->render      = gimp_view_renderer_gradient_render;
}

static void
gimp_view_renderer_gradient_init (GimpViewRendererGradient *renderer)
{
  renderer->left  = 0.0;
  renderer->right = 1.0;
}

static void
gimp_view_renderer_gradient_fg_bg_changed (GimpContext      *context,
                                           const GimpRGB    *color,
                                           GimpViewRenderer *renderer)
{
#if 0
  g_printerr ("%s: invalidating %s\n", G_STRFUNC,
              gimp_object_get_name (renderer->viewable));
#endif

  gimp_view_renderer_invalidate (renderer);
}

static void
gimp_view_renderer_gradient_set_context (GimpViewRenderer *renderer,
                                         GimpContext      *context)
{
  GimpViewRendererGradient *rendergrad;

  rendergrad = GIMP_VIEW_RENDERER_GRADIENT (renderer);

  if (renderer->context && rendergrad->has_fg_bg_segments)
    {
      g_signal_handlers_disconnect_by_func (renderer->context,
                                            gimp_view_renderer_gradient_fg_bg_changed,
                                            renderer);
    }

  GIMP_VIEW_RENDERER_CLASS (parent_class)->set_context (renderer, context);

  if (renderer->context && rendergrad->has_fg_bg_segments)
    {
      g_signal_connect (renderer->context, "foreground-changed",
                        G_CALLBACK (gimp_view_renderer_gradient_fg_bg_changed),
                        renderer);
      g_signal_connect (renderer->context, "background-changed",
                        G_CALLBACK (gimp_view_renderer_gradient_fg_bg_changed),
                        renderer);

      gimp_view_renderer_gradient_fg_bg_changed (renderer->context,
                                                 NULL,
                                                 renderer);
    }
}

static void
gimp_view_renderer_gradient_invalidate (GimpViewRenderer *renderer)
{
  GimpViewRendererGradient *rendergrad;
  gboolean                  has_fg_bg_segments = FALSE;

  rendergrad = GIMP_VIEW_RENDERER_GRADIENT (renderer);

  if (renderer->viewable)
    has_fg_bg_segments =
      gimp_gradient_has_fg_bg_segments (GIMP_GRADIENT (renderer->viewable));

  if (rendergrad->has_fg_bg_segments != has_fg_bg_segments)
    {
      if (renderer->context)
        {
          if (rendergrad->has_fg_bg_segments)
            {
              g_signal_handlers_disconnect_by_func (renderer->context,
                                                    gimp_view_renderer_gradient_fg_bg_changed,
                                                    renderer);
            }
          else
            {
              g_signal_connect (renderer->context, "foreground-changed",
                                G_CALLBACK (gimp_view_renderer_gradient_fg_bg_changed),
                                renderer);
              g_signal_connect (renderer->context, "background-changed",
                                G_CALLBACK (gimp_view_renderer_gradient_fg_bg_changed),
                                renderer);
            }
        }

      rendergrad->has_fg_bg_segments = has_fg_bg_segments;
    }

  GIMP_VIEW_RENDERER_CLASS (parent_class)->invalidate (renderer);
}

static void
gimp_view_renderer_gradient_render (GimpViewRenderer *renderer,
                                    GtkWidget        *widget)
{
  GimpViewRendererGradient *rendergrad = GIMP_VIEW_RENDERER_GRADIENT (renderer);
  GimpGradient             *gradient   = GIMP_GRADIENT (renderer->viewable);
  GimpGradientSegment      *seg        = NULL;
  GimpColorTransform       *transform;
  guchar                   *buf;
  guchar                   *dest;
  gint                      dest_stride;
  gint                      x;
  gint                      y;
  gdouble                   dx, cur_x;
  GimpRGB                   color;

  buf   = g_alloca (4 * renderer->width);
  dx    = (rendergrad->right - rendergrad->left) / (renderer->width - 1);
  cur_x = rendergrad->left;

  for (x = 0, dest = buf; x < renderer->width; x++, dest += 4)
    {
      guchar r, g, b, a;

      seg = gimp_gradient_get_color_at (gradient, renderer->context, seg,
                                        cur_x,
                                        rendergrad->reverse,
                                        rendergrad->blend_color_space,
                                        &color);
      cur_x += dx;

      gimp_rgba_get_uchar (&color, &r, &g, &b, &a);

      GIMP_CAIRO_ARGB32_SET_PIXEL (dest, r, g, b, a);
    }

  if (! renderer->surface)
    renderer->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                    renderer->width,
                                                    renderer->height);

  cairo_surface_flush (renderer->surface);

  dest        = cairo_image_surface_get_data (renderer->surface);
  dest_stride = cairo_image_surface_get_stride (renderer->surface);

  transform = gimp_view_renderer_get_color_transform (renderer, widget,
                                                      babl_format ("cairo-ARGB32"),
                                                      babl_format ("cairo-ARGB32"));

  if (transform)
    gimp_color_transform_process_pixels (transform,
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
gimp_view_renderer_gradient_set_offsets (GimpViewRendererGradient *renderer,
                                         gdouble                   left,
                                         gdouble                   right)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER_GRADIENT (renderer));

  left  = CLAMP (left, 0.0, 1.0);
  right = CLAMP (right, left, 1.0);

  if (left != renderer->left || right != renderer->right)
    {
      renderer->left  = left;
      renderer->right = right;

      gimp_view_renderer_invalidate (GIMP_VIEW_RENDERER (renderer));
    }
}

void
gimp_view_renderer_gradient_set_reverse (GimpViewRendererGradient *renderer,
                                         gboolean                  reverse)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER_GRADIENT (renderer));

  if (reverse != renderer->reverse)
    {
      renderer->reverse = reverse ? TRUE : FALSE;

      gimp_view_renderer_invalidate (GIMP_VIEW_RENDERER (renderer));
      gimp_view_renderer_update (GIMP_VIEW_RENDERER (renderer));
    }
}

void
gimp_view_renderer_gradient_set_blend_color_space (GimpViewRendererGradient    *renderer,
                                                   GimpGradientBlendColorSpace  blend_color_space)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER_GRADIENT (renderer));

  if (blend_color_space != renderer->blend_color_space)
    {
      renderer->blend_color_space = blend_color_space;

      gimp_view_renderer_invalidate (GIMP_VIEW_RENDERER (renderer));
      gimp_view_renderer_update (GIMP_VIEW_RENDERER (renderer));
    }
}
