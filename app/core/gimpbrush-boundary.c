/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "ligmabezierdesc.h"
#include "ligmaboundary.h"
#include "ligmabrush.h"
#include "ligmabrush-boundary.h"
#include "ligmatempbuf.h"


static LigmaBezierDesc *
ligma_brush_transform_boundary_exact (LigmaBrush *brush,
                                     gdouble    scale,
                                     gdouble    aspect_ratio,
                                     gdouble    angle,
                                     gboolean   reflect,
                                     gdouble    hardness)
{
  const LigmaTempBuf *mask;

  mask = ligma_brush_transform_mask (brush,
                                    scale, aspect_ratio,
                                    angle, reflect, hardness);

  if (mask)
    {
      GeglBuffer    *buffer;
      LigmaBoundSeg  *bound_segs;
      gint           n_bound_segs;

      buffer = ligma_temp_buf_create_buffer ((LigmaTempBuf *) mask);

      bound_segs = ligma_boundary_find (buffer, NULL,
                                       babl_format ("Y float"),
                                       LIGMA_BOUNDARY_WITHIN_BOUNDS,
                                       0, 0,
                                       gegl_buffer_get_width  (buffer),
                                       gegl_buffer_get_height (buffer),
                                       0.0,
                                       &n_bound_segs);

      g_object_unref (buffer);

      if (bound_segs)
        {
          LigmaBoundSeg *stroke_segs;
          gint          n_stroke_groups;

          stroke_segs = ligma_boundary_sort (bound_segs, n_bound_segs,
                                            &n_stroke_groups);

          g_free (bound_segs);

          if (stroke_segs)
            {
              LigmaBezierDesc *path;

              path = ligma_bezier_desc_new_from_bound_segs (stroke_segs,
                                                           n_bound_segs,
                                                           n_stroke_groups);

              g_free (stroke_segs);

              return path;
            }
        }
    }

  return NULL;
}

static LigmaBezierDesc *
ligma_brush_transform_boundary_approx (LigmaBrush *brush,
                                      gdouble    scale,
                                      gdouble    aspect_ratio,
                                      gdouble    angle,
                                      gboolean   reflect,
                                      gdouble    hardness)
{
  return ligma_brush_transform_boundary_exact (brush,
                                              scale, aspect_ratio,
                                              angle, reflect, hardness);
}

LigmaBezierDesc *
ligma_brush_real_transform_boundary (LigmaBrush *brush,
                                    gdouble    scale,
                                    gdouble    aspect_ratio,
                                    gdouble    angle,
                                    gboolean   reflect,
                                    gdouble    hardness,
                                    gint      *width,
                                    gint      *height)
{
  ligma_brush_transform_size (brush, scale, aspect_ratio, angle, reflect,
                             width, height);

  if (*width < 256 && *height < 256)
    {
      return ligma_brush_transform_boundary_exact (brush,
                                                  scale, aspect_ratio,
                                                  angle, reflect, hardness);
    }

  return ligma_brush_transform_boundary_approx (brush,
                                               scale, aspect_ratio,
                                               angle, reflect, hardness);
}
