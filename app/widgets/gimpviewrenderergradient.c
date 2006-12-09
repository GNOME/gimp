/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrenderergradient.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "core/gimpgradient.h"

#include "gimprender.h"
#include "gimpviewrenderergradient.h"


static void   gimp_view_renderer_gradient_finalize    (GObject          *object);

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
  GObjectClass          *object_class   = G_OBJECT_CLASS (klass);
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  object_class->finalize      = gimp_view_renderer_gradient_finalize;

  renderer_class->set_context = gimp_view_renderer_gradient_set_context;
  renderer_class->invalidate  = gimp_view_renderer_gradient_invalidate;
  renderer_class->render      = gimp_view_renderer_gradient_render;
}

static void
gimp_view_renderer_gradient_init (GimpViewRendererGradient *renderer)
{
  renderer->even    = NULL;
  renderer->odd     = NULL;
  renderer->width   = -1;
  renderer->left    = 0.0;
  renderer->right   = 1.0;
  renderer->reverse = FALSE;
}

static void
gimp_view_renderer_gradient_finalize (GObject *object)
{
  GimpViewRendererGradient *renderer = GIMP_VIEW_RENDERER_GRADIENT (object);

  if (renderer->even)
    {
      g_free (renderer->even);
      renderer->even = NULL;
    }

  if (renderer->odd)
    {
      g_free (renderer->odd);
      renderer->odd = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_view_renderer_gradient_fg_bg_changed (GimpContext      *context,
                                           const GimpRGB    *color,
                                           GimpViewRenderer *renderer)
{
#if 0
  g_printerr ("%s: invalidating %s\n", G_STRFUNC,
              gimp_object_get_name (GIMP_OBJECT (renderer->viewable)));
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
  GimpViewRendererGradient *rendergrad;
  GimpGradient             *gradient;
  GimpGradientSegment      *seg = NULL;
  guchar                   *even;
  guchar                   *odd;
  guchar                   *buf;
  gint                      x;
  gint                      y;
  gdouble                   dx, cur_x;
  GimpRGB                   color;

  rendergrad = GIMP_VIEW_RENDERER_GRADIENT (renderer);

  gradient = GIMP_GRADIENT (renderer->viewable);

  if (renderer->width != rendergrad->width)
    {
      if (rendergrad->even)
        g_free (rendergrad->even);

      if (rendergrad->odd)
        g_free (rendergrad->odd);

      rendergrad->even = g_new (guchar, renderer->rowstride);
      rendergrad->odd  = g_new (guchar, renderer->rowstride);

      rendergrad->width = renderer->width;
    }

  even = rendergrad->even;
  odd  = rendergrad->odd;

  dx    = (rendergrad->right - rendergrad->left) / (renderer->width - 1);
  cur_x = rendergrad->left;

  for (x = 0; x < renderer->width; x++)
    {
      guchar r, g, b, a;

      seg = gimp_gradient_get_color_at (gradient, renderer->context, seg,
                                        cur_x, rendergrad->reverse, &color);
      cur_x += dx;

      gimp_rgba_get_uchar (&color, &r, &g, &b, &a);

      if (x & 0x4)
        {
          *even++ = gimp_render_blend_dark_check[(a << 8) | r];
          *even++ = gimp_render_blend_dark_check[(a << 8) | g];
          *even++ = gimp_render_blend_dark_check[(a << 8) | b];

          *odd++ = gimp_render_blend_light_check[(a << 8) | r];
          *odd++ = gimp_render_blend_light_check[(a << 8) | g];
          *odd++ = gimp_render_blend_light_check[(a << 8) | b];
        }
      else
        {
          *even++ = gimp_render_blend_light_check[(a << 8) | r];
          *even++ = gimp_render_blend_light_check[(a << 8) | g];
          *even++ = gimp_render_blend_light_check[(a << 8) | b];

          *odd++ = gimp_render_blend_dark_check[(a << 8) | r];
          *odd++ = gimp_render_blend_dark_check[(a << 8) | g];
          *odd++ = gimp_render_blend_dark_check[(a << 8) | b];
        }
    }

  if (! renderer->buffer)
    renderer->buffer = g_new (guchar, renderer->height * renderer->rowstride);

  buf = renderer->buffer;

  for (y = 0; y < renderer->height; y++)
    {
      if (y & 0x4)
        memcpy (buf, rendergrad->even, renderer->rowstride);
      else
        memcpy (buf, rendergrad->odd,  renderer->rowstride);

      buf += renderer->rowstride;
    }

  renderer->needs_render = FALSE;
}

void
gimp_view_renderer_gradient_set_offsets (GimpViewRendererGradient *renderer,
                                         gdouble                   left,
                                         gdouble                   right,
                                         gboolean                  instant_update)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER_GRADIENT (renderer));

  left  = CLAMP (left, 0.0, 1.0);
  right = CLAMP (right, left, 1.0);

  if (left != renderer->left || right != renderer->right)
    {
      renderer->left  = left;
      renderer->right = right;

      gimp_view_renderer_invalidate (GIMP_VIEW_RENDERER (renderer));

      if (instant_update)
        gimp_view_renderer_update (GIMP_VIEW_RENDERER (renderer));
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
