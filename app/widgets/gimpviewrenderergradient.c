/* The GIMP -- an image manipulation program
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

#ifdef __GNUC__
#warning FIXME #include "display/display-types.h"
#endif
#include "display/display-types.h"

#include "core/gimpgradient.h"

#include "display/gimpdisplayshell-render.h"

#include "gimpviewrenderergradient.h"


static void   gimp_view_renderer_gradient_class_init (GimpViewRendererGradientClass *klass);
static void   gimp_view_renderer_gradient_init       (GimpViewRendererGradient      *renderer);

static void   gimp_view_renderer_gradient_finalize   (GObject             *object);

static void   gimp_view_renderer_gradient_render     (GimpViewRenderer    *renderer,
                                                      GtkWidget           *widget);


static GimpViewRendererClass *parent_class = NULL;


GType
gimp_view_renderer_gradient_get_type (void)
{
  static GType renderer_type = 0;

  if (! renderer_type)
    {
      static const GTypeInfo renderer_info =
      {
        sizeof (GimpViewRendererGradientClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_view_renderer_gradient_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpViewRendererGradient),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_view_renderer_gradient_init,
      };

      renderer_type = g_type_register_static (GIMP_TYPE_VIEW_RENDERER,
                                              "GimpViewRendererGradient",
                                              &renderer_info, 0);
    }

  return renderer_type;
}

static void
gimp_view_renderer_gradient_class_init (GimpViewRendererGradientClass *klass)
{
  GObjectClass          *object_class   = G_OBJECT_CLASS (klass);
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_view_renderer_gradient_finalize;

  renderer_class->render = gimp_view_renderer_gradient_render;
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
gimp_view_renderer_gradient_render (GimpViewRenderer *renderer,
                                    GtkWidget        *widget)
{
  GimpViewRendererGradient *rendergrad;
  GimpGradient             *gradient;
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

      gimp_gradient_get_color_at (gradient, cur_x, rendergrad->reverse, &color);
      cur_x += dx;

      gimp_rgba_get_uchar (&color, &r, &g, &b, &a);

      if (x & 0x4)
        {
          *even++ = render_blend_dark_check[(a << 8) | r];
          *even++ = render_blend_dark_check[(a << 8) | g];
          *even++ = render_blend_dark_check[(a << 8) | b];

          *odd++ = render_blend_light_check[(a << 8) | r];
          *odd++ = render_blend_light_check[(a << 8) | g];
          *odd++ = render_blend_light_check[(a << 8) | b];
        }
      else
        {
          *even++ = render_blend_light_check[(a << 8) | r];
          *even++ = render_blend_light_check[(a << 8) | g];
          *even++ = render_blend_light_check[(a << 8) | b];

          *odd++ = render_blend_dark_check[(a << 8) | r];
          *odd++ = render_blend_dark_check[(a << 8) | g];
          *odd++ = render_blend_dark_check[(a << 8) | b];
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
                                         gboolean                     reverse)
{
  g_return_if_fail (GIMP_IS_VIEW_RENDERER_GRADIENT (renderer));

  if (reverse != renderer->reverse)
    {
      renderer->reverse = reverse ? TRUE : FALSE;

      gimp_view_renderer_invalidate (GIMP_VIEW_RENDERER (renderer));
      gimp_view_renderer_update (GIMP_VIEW_RENDERER (renderer));
    }
}
