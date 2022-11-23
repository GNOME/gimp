/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabezierdesc.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#include <gegl.h>
#include <cairo.h>

#include "core-types.h"

#include "ligmabezierdesc.h"
#include "ligmaboundary.h"


G_DEFINE_BOXED_TYPE (LigmaBezierDesc, ligma_bezier_desc, ligma_bezier_desc_copy, ligma_bezier_desc_free)

LigmaBezierDesc *
ligma_bezier_desc_new (cairo_path_data_t *data,
                      gint               n_data)
{
  LigmaBezierDesc *desc;

  g_return_val_if_fail (n_data == 0 || data != NULL, NULL);

  desc = g_slice_new (LigmaBezierDesc);

  desc->status   = CAIRO_STATUS_SUCCESS;
  desc->num_data = n_data;
  desc->data     = data;

  return desc;
}

static void
add_polyline (GArray            *path_data,
              const LigmaVector2 *points,
              gint               n_points,
              gboolean           closed)
{
  LigmaVector2       prev = { 0.0, 0.0, };
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

  /* close the polyline when needed */
  if (closed)
    {
      pd.header.type   = CAIRO_PATH_CLOSE_PATH;
      pd.header.length = 1;

      g_array_append_val (path_data, pd);
    }
}

LigmaBezierDesc *
ligma_bezier_desc_new_from_bound_segs (LigmaBoundSeg *bound_segs,
                                      gint          n_bound_segs,
                                      gint          n_bound_groups)
{
  GArray      *path_data;
  LigmaVector2 *points;
  gint         n_points;
  gint         seg;
  gint         i;
  guint        path_data_len;

  g_return_val_if_fail (bound_segs != NULL, NULL);
  g_return_val_if_fail (n_bound_segs > 0, NULL);

  path_data = g_array_new (FALSE, FALSE, sizeof (cairo_path_data_t));

  points = g_new0 (LigmaVector2, n_bound_segs + 4);

  seg = 0;
  n_points = 0;

  points[n_points].x = (gdouble) (bound_segs[0].x1);
  points[n_points].y = (gdouble) (bound_segs[0].y1);

  n_points++;

  for (i = 0; i < n_bound_groups; i++)
    {
      while (bound_segs[seg].x1 != -1 ||
             bound_segs[seg].x2 != -1 ||
             bound_segs[seg].y1 != -1 ||
             bound_segs[seg].y2 != -1)
        {
          points[n_points].x = (gdouble) (bound_segs[seg].x1);
          points[n_points].y = (gdouble) (bound_segs[seg].y1);

          n_points++;
          seg++;
        }

      /* Close the stroke points up */
      points[n_points] = points[0];

      n_points++;

      add_polyline (path_data, points, n_points, TRUE);

      n_points = 0;
      seg++;

      points[n_points].x = (gdouble) (bound_segs[seg].x1);
      points[n_points].y = (gdouble) (bound_segs[seg].y1);

      n_points++;
    }

  g_free (points);

  path_data_len = path_data->len;

  return ligma_bezier_desc_new ((cairo_path_data_t *) g_array_free (path_data, FALSE),
                               path_data_len);
}

void
ligma_bezier_desc_translate (LigmaBezierDesc *desc,
                            gdouble         offset_x,
                            gdouble         offset_y)
{
  gint i, j;

  g_return_if_fail (desc != NULL);

  for (i = 0; i < desc->num_data; i += desc->data[i].header.length)
    for (j = 1; j < desc->data[i].header.length; ++j)
      {
        desc->data[i+j].point.x += offset_x;
        desc->data[i+j].point.y += offset_y;
      }
}

LigmaBezierDesc *
ligma_bezier_desc_copy (const LigmaBezierDesc *desc)
{
  g_return_val_if_fail (desc != NULL, NULL);

  return ligma_bezier_desc_new (g_memdup2 (desc->data,
                                          desc->num_data * sizeof (cairo_path_data_t)),
                               desc->num_data);
}

void
ligma_bezier_desc_free (LigmaBezierDesc *desc)
{
  g_return_if_fail (desc != NULL);

  g_free (desc->data);
  g_slice_free (LigmaBezierDesc, desc);
}
