/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcoords-interpolate.c
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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimpcoords.h"
#include "gimpcoords-interpolate.h"

/* Local helper functions declarations*/
static void     gimp_coords_interpolate_bezier_internal (const GimpCoords  bezier_pt1,
                                                         const GimpCoords  bezier_pt2,
                                                         const GimpCoords  bezier_pt3,
                                                         const GimpCoords  bezier_pt4,
                                                         const gdouble     start_t,
                                                         const gdouble     end_t,
                                                         const gdouble     precision,
                                                         GArray          **ret_coords,
                                                         GArray          **ret_params,
                                                         gint              depth);

/* Functions for bezier subdivision */

void
gimp_coords_interpolate_bezier (const GimpCoords   bezier_pt1,
                                const GimpCoords   bezier_pt2,
                                const GimpCoords   bezier_pt3,
                                const GimpCoords   bezier_pt4,
                                const gdouble      precision,
                                GArray           **ret_coords,
                                GArray           **ret_params)
{
  gimp_coords_interpolate_bezier_internal (bezier_pt1,
                                           bezier_pt2,
                                           bezier_pt3,
                                           bezier_pt4,
                                           0.0, 1.0,
                                           precision,
                                           ret_coords, ret_params, 10);
}

/* Recursive subdivision helper function */
static void
gimp_coords_interpolate_bezier_internal (const GimpCoords  bezier_pt1,
                                         const GimpCoords  bezier_pt2,
                                         const GimpCoords  bezier_pt3,
                                         const GimpCoords  bezier_pt4,
                                         const gdouble     start_t,
                                         const gdouble     end_t,
                                         const gdouble     precision,
                                         GArray          **ret_coords,
                                         GArray          **ret_params,
                                         gint              depth)
{
  /*
   * beziercoords has to contain four GimpCoords with the four control points
   * of the bezier segment. We subdivide it at the parameter 0.5.
   */

  GimpCoords subdivided[8];
  gdouble    middle_t = (start_t + end_t) / 2;

  subdivided[0] = bezier_pt1;
  subdivided[6] = bezier_pt4;

  /* if (!depth) g_printerr ("Hit recursion depth limit!\n"); */

  gimp_coords_average (&bezier_pt1, &bezier_pt2, &(subdivided[1]));

  gimp_coords_average (&bezier_pt2, &bezier_pt3, &(subdivided[7]));

  gimp_coords_average (&bezier_pt3, &bezier_pt4, &(subdivided[5]));

  gimp_coords_average (&(subdivided[1]), &(subdivided[7]),
                       &(subdivided[2]));

  gimp_coords_average (&(subdivided[7]), &(subdivided[5]),
                       &(subdivided[4]));

  gimp_coords_average (&(subdivided[2]), &(subdivided[4]),
                       &(subdivided[3]));

  /*
   * We now have the coordinates of the two bezier segments in
   * subdivided [0-3] and subdivided [3-6]
   */

  /*
   * Here we need to check, if we have sufficiently subdivided, i.e.
   * if the stroke is sufficiently close to a straight line.
   */

  if (!depth || gimp_coords_bezier_is_straight (subdivided[0],
                                                subdivided[1],
                                                subdivided[2],
                                                subdivided[3],
                                                precision)) /* 1st half */
    {
      *ret_coords = g_array_append_vals (*ret_coords, &(subdivided[0]), 3);

      if (ret_params)
        {
          gdouble params[3];

          params[0] = start_t;
          params[1] = (2 * start_t + middle_t) / 3;
          params[2] = (start_t + 2 * middle_t) / 3;

          *ret_params = g_array_append_vals (*ret_params, &(params[0]), 3);
        }
    }
  else
    {
      gimp_coords_interpolate_bezier_internal (subdivided[0],
                                               subdivided[1],
                                               subdivided[2],
                                               subdivided[3],
                                               start_t, (start_t + end_t) / 2,
                                               precision,
                                               ret_coords, ret_params, depth-1);
    }

  if (!depth || gimp_coords_bezier_is_straight (subdivided[3],
                                                subdivided[4],
                                                subdivided[5],
                                                subdivided[6],
                                                precision)) /* 2nd half */
    {
      *ret_coords = g_array_append_vals (*ret_coords, &(subdivided[3]), 3);

      if (ret_params)
        {
          gdouble params[3];

          params[0] = middle_t;
          params[1] = (2 * middle_t + end_t) / 3;
          params[2] = (middle_t + 2 * end_t) / 3;

          *ret_params = g_array_append_vals (*ret_params, &(params[0]), 3);
        }
    }
  else
    {
      gimp_coords_interpolate_bezier_internal (subdivided[3],
                                               subdivided[4],
                                               subdivided[5],
                                               subdivided[6],
                                               (start_t + end_t) / 2, end_t,
                                               precision,
                                               ret_coords, ret_params, depth-1);
    }
}


/*
 * a helper function that determines if a bezier segment is "straight
 * enough" to be approximated by a line.
 *
 * To be more exact, it also checks for the control points to be distributed
 * evenly along the line. This makes it easier to reconstruct parameters for
 * a given point along the segment.
 *
 * Needs four GimpCoords in an array.
 */

gboolean
gimp_coords_bezier_is_straight (const GimpCoords bezier_pt1,
                                const GimpCoords bezier_pt2,
                                const GimpCoords bezier_pt3,
                                const GimpCoords bezier_pt4,
                                gdouble           precision)
{
  GimpCoords pt1, pt2;

  /* calculate the "ideal" positions for the control points */

  gimp_coords_mix (2.0 / 3.0, &bezier_pt1,
                   1.0 / 3.0, &bezier_pt4,
                   &pt1);
  gimp_coords_mix (1.0 / 3.0, &bezier_pt1,
                   2.0 / 3.0, &bezier_pt4,
                   &pt2);

  /* calculate the deviation of the actual control points */

  return (gimp_coords_manhattan_dist (&bezier_pt2, &pt1) < precision &&
          gimp_coords_manhattan_dist (&bezier_pt3, &pt2) < precision);
}
