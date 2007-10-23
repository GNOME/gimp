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


/**
 * gimp_tool_motion_constrain_helper:
 * @dx: the (fixed) delta-x
 * @dy: a suggested delta-y
 *
 * Returns: An adjusted dy' near dy such that the slope (dx,dy')
 *          is a multiple of 15 degrees.
 **/
static gdouble
gimp_tool_motion_constrain_helper (gdouble dx,
                                   gdouble dy)
{
  static const gdouble slope[4]   = { 0, 0.26795, 0.57735, 1 };
  static const gdouble divider[3] = { 0.13165, 0.41421, 0.76732 };
  gint  i;

  if (dy < 0)
    return - gimp_tool_motion_constrain_helper (dx, -dy);

  dx = fabs (dx);

  for (i = 0; i < 3; i ++)
    if (dy < dx * divider[i])
      break;

  dy = dx * slope[i];

  return dy;
}

/**
 * gimp_tool_motion_constrain:
 * @start_x:
 * @start_y:
 * @end_x:
 * @end_y:
 *
 * Restricts the motion vector to 15 degree steps by changing the end
 * point (if necessary).
 **/
void
gimp_tool_motion_constrain (gdouble   start_x,
                            gdouble   start_y,
                            gdouble  *end_x,
                            gdouble  *end_y)
{
  gdouble dx = *end_x - start_x;
  gdouble dy = *end_y - start_y;

  /*  This algorithm changes only one of dx and dy, and does not try
   *  to constrain the resulting dx and dy to integers. This gives
   *  at least two benefits:
   *    1. gimp_tool_motion_constrain is idempotent, even if followed by
   *       a rounding operation.
   *    2. For any two lines with the same starting-point and ideal
   *       15-degree direction, the points plotted by
   *       gimp_paint_core_interpolate for the shorter line will always
   *       be a superset of those plotted for the longer line.
   */

  if (fabs (dx) > fabs (dy))
    *end_y = (start_y + gimp_tool_motion_constrain_helper (dx, dy));
  else
    *end_x = (start_x + gimp_tool_motion_constrain_helper (dy, dx));
}

