/* math.c: define some simple array operations, and other functions.
 *
 * Copyright (C) 1992 Free Software Foundation, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
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

#include <errno.h>
#include <math.h>
#include <stdio.h>

#include "libgimp/gimp.h"

#include "types.h"
#include "global.h"

/* Numerical errors sometimes make a floating point number just slightly
   larger or smaller than its true value.  When it matters, we need to
   compare with some tolerance, REAL_EPSILON, defined in kbase.h.  */

boolean
epsilon_equal (real v1, real v2)
{
  return
    v1 == v2		       /* Usually they'll be exactly equal, anyway.  */
    || fabs (v1 - v2) <= REAL_EPSILON;
}


/* Return the Euclidean distance between P1 and P2.  */

real
distance (real_coordinate_type p1, real_coordinate_type p2)
{
  return hypot (p1.x - p2.x, p1.y - p2.y);
}


/* Same thing, for integer points.  */
real
int_distance (coordinate_type p1, coordinate_type p2)
{
  return hypot ((double) p1.x - p2.x, (double) p1.y - p2.y);
}


/* Return the arc cosine of V, in degrees in the range zero to 180.  V
   is taken to be in radians.  */

real
my_acosd (real v)
{
  real a;

  if (epsilon_equal (v, 1.0))
    v = 1.0;
  else if (epsilon_equal (v, -1.0))
    v = -1.0;

  errno = 0;
  a = acos (v);
  if (errno == ERANGE || errno == EDOM)
    FATAL_PERROR ("acosd");

  return a * 180.0 / G_PI;
}


/* The slope of the line defined by COORD1 and COORD2.  */

real
slope (real_coordinate_type coord1, real_coordinate_type coord2)
{
  g_assert (coord2.x - coord1.x != 0);

  return (coord2.y - coord1.y) / (coord2.x - coord1.x);
}


/* Turn an integer point into a real one, and vice versa.  */

real_coordinate_type
int_to_real_coord (coordinate_type int_coord)
{
  real_coordinate_type real_coord;

  real_coord.x = int_coord.x;
  real_coord.y = int_coord.y;

  return real_coord;
}


coordinate_type
real_to_int_coord (real_coordinate_type real_coord)
{
  coordinate_type int_coord;

  int_coord.x = SROUND (real_coord.x);
  int_coord.y = SROUND (real_coord.y);

  return int_coord;
}


/* See if two points (described by their row and column) are adjacent.  */

boolean
points_adjacent_p (int row1, int col1, int row2, int col2)
{
  int row_diff = abs (row1 - row2);
  int col_diff = abs (col1 - col2);

  return
    (row_diff == 1 && col_diff == 1)
    || (row_diff == 0 && col_diff == 1)
    || (row_diff == 1 && col_diff == 0);
}


/* Find the largest and smallest elements in an array of reals.  */

void
find_bounds (real *values, unsigned value_count, real *min, real *max)
{
  unsigned this_value;

  /* We must use FLT_MAX and FLT_MIN, instead of the corresponding
     values for double, because gcc uses the native atof to parse
     floating point constants, and many atof's choke on the extremes.  */
  *min = FLT_MAX;
  *max = FLT_MIN;

  for (this_value = 0; this_value < value_count; this_value++)
    {
      if (values[this_value] < *min)
	*min = values[this_value];

      if (values[this_value] > *max)
	*max = values[this_value];
    }
}

/* Map a range of numbers, some positive and some negative, into all
   positive, with the greatest being at one and the least at zero.

   This allocates new memory.  */

real *
map_to_unit (real *values, unsigned value_count)
{
  real smallest, largest;
  int this_value;
  real *mapped_values = g_new (real, value_count);

  find_bounds (values, value_count, &smallest, &largest);

  largest -= smallest;		/* We never care about largest itself. */

  for (this_value = 0; this_value < value_count; this_value++)
    mapped_values[this_value] = (values[this_value] - smallest) / largest;

  return mapped_values;
}
