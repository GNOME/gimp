/* The GIMP -- an image manipulation program
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "gimpviewrenderervectors.h"


static void   gimp_view_renderer_vectors_class_init (GimpViewRendererVectorsClass *klass);

static void   gimp_view_renderer_vectors_draw (GimpViewRenderer   *renderer,
                                               GdkWindow          *window,
                                               GtkWidget          *widget,
                                               const GdkRectangle *draw_area,
                                               const GdkRectangle *expose_area);


static GimpViewRendererClass *parent_class = NULL;


GType
gimp_view_renderer_vectors_get_type (void)
{
  static GType renderer_type = 0;

  if (! renderer_type)
    {
      static const GTypeInfo renderer_info =
      {
        sizeof (GimpViewRendererVectorsClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_view_renderer_vectors_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpViewRendererVectors),
        0,              /* n_preallocs */
        NULL,           /* instance_init */
      };

      renderer_type = g_type_register_static (GIMP_TYPE_VIEW_RENDERER,
                                              "GimpViewRendererVectors",
                                              &renderer_info, 0);
    }

  return renderer_type;
}

static void
gimp_view_renderer_vectors_class_init (GimpViewRendererVectorsClass *klass)
{
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  renderer_class->draw = gimp_view_renderer_vectors_draw;
}

static void
gimp_view_renderer_vectors_draw (GimpViewRenderer   *renderer,
                                 GdkWindow          *window,
                                 GtkWidget          *widget,
                                 const GdkRectangle *draw_area,
                                 const GdkRectangle *expose_area)
{
  GimpVectors  *vectors;
  GimpStroke   *stroke;
  GdkRectangle  rect, area;
  gdouble       xscale, yscale;

  rect.width  = renderer->width;
  rect.height = renderer->height;
  rect.x      = draw_area->x + (draw_area->width  - rect.width)  / 2;
  rect.y      = draw_area->y + (draw_area->height - rect.height) / 2;

  if (! gdk_rectangle_intersect (&rect, (GdkRectangle *) expose_area, &area))
    return;

  gdk_draw_rectangle (window, widget->style->white_gc, TRUE,
                      area.x, area.y, area.width, area.height);

  vectors = GIMP_VECTORS (renderer->viewable);

  xscale = (gdouble) GIMP_ITEM (vectors)->width  / (gdouble) rect.width;
  yscale = (gdouble) GIMP_ITEM (vectors)->height / (gdouble) rect.height;

  gdk_gc_set_clip_rectangle (widget->style->black_gc, &area);

  for (stroke = gimp_vectors_stroke_get_next (vectors, NULL);
       stroke != NULL;
       stroke = gimp_vectors_stroke_get_next (vectors, stroke))
    {
      GArray   *coordinates;
      GdkPoint *points;
      gint      i;

      coordinates = gimp_stroke_interpolate (stroke,
                                             MIN (xscale, yscale) / 2,
                                             NULL);
      if (!coordinates)
        continue;

      if (coordinates->len > 0)
        {
          points = g_new (GdkPoint, coordinates->len);

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

      g_array_free (coordinates, TRUE);
    }

  gdk_gc_set_clip_rectangle (widget->style->black_gc, NULL);
}
