/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrenderervectors.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *                    Simon Budig <simon@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "gimpviewrenderervectors.h"


static void   gimp_view_renderer_vectors_draw (GimpViewRenderer *renderer,
                                               GtkWidget        *widget,
                                               cairo_t          *cr,
                                               gint              available_width,
                                               gint              available_height);


G_DEFINE_TYPE (GimpViewRendererVectors, gimp_view_renderer_vectors,
               GIMP_TYPE_VIEW_RENDERER)

#define parent_class gimp_view_renderer_vectors_parent_class


static void
gimp_view_renderer_vectors_class_init (GimpViewRendererVectorsClass *klass)
{
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  renderer_class->draw = gimp_view_renderer_vectors_draw;
}

static void
gimp_view_renderer_vectors_init (GimpViewRendererVectors *renderer)
{
}

static void
gimp_view_renderer_vectors_draw (GimpViewRenderer *renderer,
                                 GtkWidget        *widget,
                                 cairo_t          *cr,
                                 gint              available_width,
                                 gint              available_height)
{
  GimpVectors          *vectors = GIMP_VECTORS (renderer->viewable);
  const GimpBezierDesc *desc;

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

  cairo_translate (cr,
                   (available_width  - renderer->width)  / 2,
                   (available_height - renderer->height) / 2);
  cairo_rectangle (cr, 0, 0, renderer->width, renderer->height);
  cairo_clip_preserve (cr);
  cairo_fill (cr);

  desc = gimp_vectors_get_bezier (vectors);

  if (desc)
    {
      gdouble xscale;
      gdouble yscale;

      xscale = ((gdouble) renderer->width /
                (gdouble) gimp_item_get_width  (GIMP_ITEM (vectors)));
      yscale = ((gdouble) renderer->height /
                (gdouble) gimp_item_get_height (GIMP_ITEM (vectors)));

      cairo_scale (cr, xscale, yscale);

      /* determine line width */
      xscale = yscale = 0.5;
      cairo_device_to_user_distance (cr, &xscale, &yscale);

      cairo_set_line_width (cr, MAX (xscale, yscale));
      cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

      cairo_append_path (cr, (cairo_path_t *) desc);
      cairo_stroke (cr);
    }
}
