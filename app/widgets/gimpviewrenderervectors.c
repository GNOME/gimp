/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppreviewrenderervectors.c
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

#include "gimppreviewrenderervectors.h"


static void   gimp_preview_renderer_vectors_class_init (GimpPreviewRendererVectorsClass *klass);

static void   gimp_preview_renderer_vectors_draw (GimpPreviewRenderer *renderer,
                                                  GdkWindow           *window,
                                                  GtkWidget           *widget,
                                                  const GdkRectangle  *draw_area,
                                                  const GdkRectangle  *expose_area);


static GimpPreviewRendererClass *parent_class = NULL;


GType
gimp_preview_renderer_vectors_get_type (void)
{
  static GType renderer_type = 0;

  if (! renderer_type)
    {
      static const GTypeInfo renderer_info =
      {
        sizeof (GimpPreviewRendererVectorsClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_preview_renderer_vectors_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpPreviewRendererVectors),
        0,              /* n_preallocs */
        NULL,           /* instance_init */
      };

      renderer_type = g_type_register_static (GIMP_TYPE_PREVIEW_RENDERER,
                                              "GimpPreviewRendererVectors",
                                              &renderer_info, 0);
    }

  return renderer_type;
}

static void
gimp_preview_renderer_vectors_class_init (GimpPreviewRendererVectorsClass *klass)
{
  GimpPreviewRendererClass *renderer_class;

  renderer_class = GIMP_PREVIEW_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  renderer_class->draw = gimp_preview_renderer_vectors_draw;
}

static void
gimp_preview_renderer_vectors_draw (GimpPreviewRenderer *renderer,
                                    GdkWindow           *window,
                                    GtkWidget           *widget,
                                    const GdkRectangle  *draw_area,
                                    const GdkRectangle  *expose_area)
{
  GimpVectors  *vectors;
  GimpItem     *item;
  GimpStroke   *stroke;
  GimpImage    *gimage;
  GdkRectangle  previewarea;
  GdkRectangle  rect, rect2;
  GArray       *coordinates;
  GdkPoint     *points;
  gboolean      closed;
  gint          i;
  gint          width, height;
  gboolean      scaling_up;
  gdouble       xscale, yscale;

  if (! gdk_rectangle_intersect ((GdkRectangle *) draw_area,
                                 (GdkRectangle *) expose_area, &rect2))
    return;

  vectors = GIMP_VECTORS (renderer->viewable);
  item    = GIMP_ITEM (vectors);
  gimage  = gimp_item_get_image (item);

  width  = renderer->width;
  height = renderer->height;

  if (gimage && ! renderer->is_popup)
    {
      width  = MAX (1, ROUND (((gdouble) width / (gdouble) gimage->width) *
                               (gdouble) item->width));
      height = MAX (1, ROUND (((gdouble) height / (gdouble) gimage->height) *
                              (gdouble) item->height));

      gimp_viewable_calc_preview_size (renderer->viewable,
                                       item->width,
                                       item->height,
                                       width,
                                       height,
                                       renderer->dot_for_dot,
                                       gimage->xresolution,
                                       gimage->yresolution,
                                       &(previewarea.width),
                                       &(previewarea.height),
                                       &scaling_up);
    }
  else
    {
      gimp_viewable_calc_preview_size (renderer->viewable,
                                       item->width,
                                       item->height,
                                       width,
                                       height,
                                       renderer->dot_for_dot,
                                       gimage ? gimage->xresolution : 1.0,
                                       gimage ? gimage->yresolution : 1.0,
                                       &(previewarea.width),
                                       &(previewarea.height),
                                       &scaling_up);
    }


  previewarea.width = previewarea.width;
  previewarea.height = previewarea.height;
  previewarea.x = (renderer->width  - previewarea.width ) / 2
                     + draw_area->x + renderer->border_width;
  previewarea.y = (renderer->height - previewarea.height) / 2
                     + draw_area->y + renderer->border_width;

  if (! gdk_rectangle_intersect (&rect2, &previewarea, &rect))
    return;

  gdk_draw_rectangle (window, widget->style->white_gc, TRUE,
                      rect.x, rect.y, rect.width, rect.height);

  xscale = (gdouble) item->width  / (gdouble) previewarea.width;
  yscale = (gdouble) item->height / (gdouble) previewarea.height;

  gdk_gc_set_clip_rectangle (widget->style->black_gc, &rect);

  for (stroke = gimp_vectors_stroke_get_next (vectors, NULL);
       stroke != NULL;
       stroke = gimp_vectors_stroke_get_next (vectors, stroke))
    {
      coordinates = gimp_stroke_interpolate (stroke, MIN (xscale, yscale),
                                             &closed);
      points = g_new (GdkPoint, coordinates->len + (closed ? 1 : 0));

      for (i = 0; i < coordinates->len; i++)
        {
          GimpCoords *coords = &(g_array_index (coordinates, GimpCoords, i));

          points[i].x = previewarea.x + ROUND (coords->x / xscale);
          points[i].y = previewarea.y + ROUND (coords->y / yscale);
        }

      if (closed)
        g_array_append_val (coordinates, points[0]);

      gdk_draw_lines (window, widget->style->black_gc,
                      points, coordinates->len);

      g_array_free (coordinates, TRUE);
      g_free (points);
    }

  gdk_gc_set_clip_rectangle (widget->style->black_gc, NULL);
}
