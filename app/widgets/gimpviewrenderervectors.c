/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrenderervectors.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *                    Simon Budig <simon@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "gimpviewrenderervectors.h"


static void   gimp_view_renderer_vectors_draw (GimpViewRenderer   *renderer,
                                               GtkWidget          *widget,
                                               cairo_t            *cr,
                                               const GdkRectangle *draw_area);


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
gimp_view_renderer_vectors_draw (GimpViewRenderer   *renderer,
                                 GtkWidget          *widget,
                                 cairo_t            *cr,
                                 const GdkRectangle *draw_area)
{
  GimpVectors  *vectors = GIMP_VECTORS (renderer->viewable);
  GimpStroke   *stroke;
  gdouble       xscale;
  gdouble       yscale;
  gint          x, y;

  gdk_cairo_set_source_color (cr, &widget->style->white);

  x = draw_area->x + (draw_area->width  - renderer->width)  / 2;
  y = draw_area->y + (draw_area->height - renderer->height) / 2;

  cairo_rectangle (cr, x, y, renderer->width, renderer->height);
  cairo_fill (cr);

  /*  FIXME: port vector previews to Cairo  */

  xscale = (gdouble) GIMP_ITEM (vectors)->width  / (gdouble) renderer->width;
  yscale = (gdouble) GIMP_ITEM (vectors)->height / (gdouble) renderer->height;

  for (stroke = gimp_vectors_stroke_get_next (vectors, NULL);
       stroke != NULL;
       stroke = gimp_vectors_stroke_get_next (vectors, stroke))
    {
      GArray *coordinates;

      coordinates = gimp_stroke_interpolate (stroke,
                                             MIN (xscale, yscale) / 2,
                                             NULL);
      if (! coordinates)
        continue;

#if 0
      if (coordinates->len > 0)
        {
          GdkPoint *points = g_new (GdkPoint, coordinates->len);
          gint      i;

          for (i = 0; i < coordinates->len; i++)
            {
              GimpCoords *coords;

              coords = &(g_array_index (coordinates, GimpCoords, i));

              points[i].x = rect.x + ROUND (coords->x / xscale);
              points[i].y = rect.y + ROUND (coords->y / yscale);
            }

          gdk_draw_lines (window, widget->style->black_gc,
                          points, coordinates->len);

          g_free (points);
        }
#endif

      g_array_free (coordinates, TRUE);
    }
}
