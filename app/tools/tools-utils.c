/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "tools-utils.h"


static gdouble gimp_tool_utils_point_to_line_distance (const GimpVector2 *point,
                                                       const GimpVector2 *point_on_line,
                                                       const GimpVector2 *normalized_line_direction,
                                                       GimpVector2       *closest_line_point);


/**
 * gimp_tool_utils_point_to_line_distance:
 * @point:              The point to calculate the distance for.
 * @point_on_line:      A point on the line.
 * @line_direction:     Normalized line direction vector.
 * @closest_line_point: Gets set to the point on the line that is
 *                      closest to @point.
 *
 * Returns: The shortest distance from @point to the line defined by
 *          @point_on_line and @normalized_line_direction.
 **/
static gdouble
gimp_tool_utils_point_to_line_distance (const GimpVector2 *point,
                                        const GimpVector2 *point_on_line,
                                        const GimpVector2 *line_direction,
                                        GimpVector2       *closest_line_point)
{
  GimpVector2 distance_vector;
  GimpVector2 tmp_a;
  GimpVector2 tmp_b;
  gdouble     d;

  gimp_vector2_sub (&tmp_a, point, point_on_line);

  d = gimp_vector2_inner_product (&tmp_a, line_direction);

  tmp_b = gimp_vector2_mul_val (*line_direction, d);

  *closest_line_point = gimp_vector2_add_val (*point_on_line,
                                              tmp_b);

  gimp_vector2_sub (&distance_vector, closest_line_point, point);

  return gimp_vector2_length (&distance_vector);
}

/**
 * gimp_tool_motion_constrain:
 * @start_x:
 * @start_y:
 * @end_x:
 * @end_y:
 * @n_snap_lines: Number evenly disributed lines to snap to.
 *
 * Projects a line onto the specified subset of evenly radially
 * distributed lines. @n_lines of 2 makes the line snap horizontally
 * or vertically. @n_lines of 4 snaps on 45 degree steps. @n_lines of
 * 12 on 15 degree steps. etc.
 **/
void
gimp_tool_motion_constrain (gdouble  start_x,
                            gdouble  start_y,
                            gdouble *end_x,
                            gdouble *end_y,
                            gint     n_snap_lines)
{
  GimpVector2 line_point          = {  start_x,  start_y };
  GimpVector2 point               = { *end_x,   *end_y   };
  GimpVector2 constrained_point;
  GimpVector2 line_dir;
  gdouble     shortest_dist_moved = G_MAXDOUBLE;
  gdouble     dist_moved;
  gdouble     angle;
  gint        i;

  for (i = 0; i < n_snap_lines; i++)
    {
      angle = i * G_PI / n_snap_lines;

      gimp_vector2_set (&line_dir,
                        cos (angle),
                        sin (angle));

      dist_moved = gimp_tool_utils_point_to_line_distance (&point,
                                                           &line_point,
                                                           &line_dir,
                                                           &constrained_point);
      if (dist_moved < shortest_dist_moved)
        {
          shortest_dist_moved = dist_moved;

          *end_x = constrained_point.x;
          *end_y = constrained_point.y;
        }
    }
}
