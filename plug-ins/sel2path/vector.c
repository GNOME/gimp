/* vector.c: vector/point operations.

Copyright (C) 1992 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <glib.h>

#include <math.h>
#include <assert.h>

#include "global.h"
#include "config.h"

#include "vector.h"


/* Given the point COORD, return the corresponding vector.  */

const vector_type
make_vector (const real_coordinate_type c)
{
  vector_type v;

  v.dx = c.x;
  v.dy = c.y;

  return v;
}


/* And the converse: given a vector, return the corresponding point.  */

const real_coordinate_type
vector_to_point (const vector_type v)
{
  real_coordinate_type coord;

  coord.x = v.dx;
  coord.y = v.dy;

  return coord;
}


const real
magnitude (const vector_type v)
{
  return hypot (v.dx, v.dy);
}


const vector_type
normalize (const vector_type v)
{
  vector_type new_v;
  real m = magnitude (v);

  assert (m > 0.0);

  new_v.dx = v.dx / m;
  new_v.dy = v.dy / m;

  return new_v;
}


const vector_type
Vadd (const vector_type v1, const vector_type v2)
{
  vector_type new_v;

  new_v.dx = v1.dx + v2.dx;
  new_v.dy = v1.dy + v2.dy;

  return new_v;
}


const real
Vdot (const vector_type v1, const vector_type v2)
{
  return v1.dx * v2.dx + v1.dy * v2.dy;
}


const vector_type
Vmult_scalar (const vector_type v, const real r)
{
  vector_type new_v;

  new_v.dx = v.dx * r;
  new_v.dy = v.dy * r;

  return new_v;
}


/* Given the IN_VECTOR and OUT_VECTOR, return the angle between them in
   degrees, in the range zero to 180.  */
   
const real
Vangle (const vector_type in_vector, const vector_type out_vector)
{
  vector_type v1 = normalize (in_vector);
  vector_type v2 = normalize (out_vector);

  return acosd (Vdot (v2, v1));
}


const real_coordinate_type
Vadd_point (const real_coordinate_type c, const vector_type v)
{
  real_coordinate_type new_c;

  new_c.x = c.x + v.dx;
  new_c.y = c.y + v.dy;
  return new_c;
}


const real_coordinate_type
Vsubtract_point (const real_coordinate_type c, const vector_type v)
{
  real_coordinate_type new_c;

  new_c.x = c.x - v.dx;
  new_c.y = c.y - v.dy;
  return new_c;
}


const coordinate_type
Vadd_int_point (const coordinate_type c, const vector_type v)
{
  coordinate_type a;
  
  a.x = ROUND ((real) c.x + v.dx);
  a.y = ROUND ((real) c.y + v.dy);
  return a;
}


const vector_type
Vabs (const vector_type v)
{
  vector_type new_v;
  
  new_v.dx = fabs (v.dx);
  new_v.dy = fabs (v.dy);
  return new_v;
}


/* Operations on points.  */

const vector_type
Psubtract (const real_coordinate_type c1, const real_coordinate_type c2)
{
  vector_type v;

  v.dx = c1.x - c2.x;
  v.dy = c1.y - c2.y;

  return v;
}

/* Operations on integer points.  */

const vector_type
IPsubtract (const coordinate_type coord1, const coordinate_type coord2)
{
  vector_type v;

  v.dx = coord1.x - coord2.x;
  v.dy = coord1.y - coord2.y;

  return v;
}


const coordinate_type
IPsubtractP (const coordinate_type c1, const coordinate_type c2)
{
  coordinate_type c;
  
  c.x = c1.x - c2.x;
  c.y = c1.y - c2.y;
  
  return c;
}


const coordinate_type
IPadd (const coordinate_type c1, const coordinate_type c2)
{
  coordinate_type c;
  
  c.x = c1.x + c2.x;
  c.y = c1.y + c2.y;
  
  return c;
}


const coordinate_type
IPmult_scalar (const coordinate_type c, const int i)
{
  coordinate_type a;
  
  a.x = c.x * i;
  a.y = c.y * i;
  
  return a;
}


const real_coordinate_type
IPmult_real (const coordinate_type c, const real r)
{
  real_coordinate_type a;

  a.x = c.x * r;
  a.y = c.y * r;

  return a;
}


const boolean
IPequal (const coordinate_type c1, const coordinate_type c2)
{
  return c1.x == c2.x && c1.y == c2.y;
}
