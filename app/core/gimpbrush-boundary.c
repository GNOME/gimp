/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>
#include <cairo.h>

#include "core-types.h"

#include "base/boundary.h"
#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "gimpbezierdesc.h"
#include "gimpbrush.h"
#include "gimpbrush-boundary.h"


static void
add_polyline (GArray            *path_data,
              const GimpVector2 *points,
              guint              n_points)
{
  GimpVector2       prev = { 0.0, 0.0, };
  cairo_path_data_t pd;
  gint              i;

  for (i = 0; i < n_points; i++)
    {
      /* compress multiple identical coordinates */
      if (i == 0 ||
          prev.x != points[i].x ||
          prev.y != points[i].y)
        {
          pd.header.type   = (i == 0) ? CAIRO_PATH_MOVE_TO : CAIRO_PATH_LINE_TO;
          pd.header.length = 2;

          g_array_append_val (path_data, pd);

          pd.point.x = points[i].x;
          pd.point.y = points[i].y;

          g_array_append_val (path_data, pd);

          prev = points[i];
        }
    }

  /* close the polyline */
  pd.header.type   = CAIRO_PATH_CLOSE_PATH;
  pd.header.length = 1;

  g_array_append_val (path_data, pd);
}

static GimpBezierDesc *
gimp_brush_transform_boundary_exact (GimpBrush *brush,
                                     gdouble    scale,
                                     gdouble    aspect_ratio,
                                     gdouble    angle,
                                     gdouble    hardness)
{
  TempBuf *mask;

  mask = gimp_brush_transform_mask (brush,
                                    scale, aspect_ratio, angle, hardness);

  if (mask)
    {
      PixelRegion   maskPR;
      BoundSeg     *bound_segs;
      gint          n_bound_segs;
      BoundSeg     *stroke_segs;
      gint          n_stroke_segs;
      GArray       *path_data;

      pixel_region_init_temp_buf (&maskPR, mask,
                                  0, 0, mask->width, mask->height);

      bound_segs = boundary_find (&maskPR, BOUNDARY_WITHIN_BOUNDS,
                                  0, 0, maskPR.w, maskPR.h,
                                  0,
                                  &n_bound_segs);

      temp_buf_free (mask);

      if (! bound_segs)
        return NULL;

      stroke_segs = boundary_sort (bound_segs, n_bound_segs, &n_stroke_segs);

      g_free (bound_segs);

      if (! stroke_segs)
        return NULL;

      path_data = g_array_new (FALSE, FALSE, sizeof (cairo_path_data_t));

      {
        GimpVector2 *points;
        gint         n_points;
        gint         seg;
        gint         i;

        points = g_new0 (GimpVector2, n_bound_segs + 4);

        seg = 0;
        n_points = 0;

        points[n_points].x = (gdouble) (stroke_segs[0].x1);
        points[n_points].y = (gdouble) (stroke_segs[0].y1);

        n_points++;

        for (i = 0; i < n_stroke_segs; i++)
          {
            while (stroke_segs[seg].x1 != -1 ||
                   stroke_segs[seg].x2 != -1 ||
                   stroke_segs[seg].y1 != -1 ||
                   stroke_segs[seg].y2 != -1)
              {
                points[n_points].x = (gdouble) (stroke_segs[seg].x1);
                points[n_points].y = (gdouble) (stroke_segs[seg].y1);

                n_points++;
                seg++;
              }

            /* Close the stroke points up */
            points[n_points] = points[0];

            n_points++;

            add_polyline (path_data, points, n_points);

            n_points = 0;
            seg++;

            points[n_points].x = (gdouble) (stroke_segs[seg].x1);
            points[n_points].y = (gdouble) (stroke_segs[seg].y1);

            n_points++;
          }

        g_free (points);
      }

      g_free (stroke_segs);

      return gimp_bezier_desc_new ((cairo_path_data_t *) g_array_free (path_data, FALSE),
                                   path_data->len);
    }

  return NULL;
}

static GimpBezierDesc *
gimp_brush_transform_boundary_approx (GimpBrush *brush,
                                      gdouble    scale,
                                      gdouble    aspect_ratio,
                                      gdouble    angle,
                                      gdouble    hardness)
{
  return NULL;
}

GimpBezierDesc *
gimp_brush_real_transform_boundary (GimpBrush *brush,
                                    gdouble    scale,
                                    gdouble    aspect_ratio,
                                    gdouble    angle,
                                    gdouble    hardness,
                                    gint      *width,
                                    gint      *height)
{
  gimp_brush_transform_size (brush, scale, aspect_ratio, angle,
                             width, height);

  if (TRUE /* width < foo && height < bar */)
    {
      return gimp_brush_transform_boundary_exact (brush,
                                                  scale, aspect_ratio,
                                                  angle, hardness);
    }

  return gimp_brush_transform_boundary_approx (brush,
                                               scale, aspect_ratio,
                                               angle, hardness);
}
