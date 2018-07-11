/* vector.h: operations on vectors and points.
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

#ifndef VECTOR_H
#define VECTOR_H

#include "types.h"

/* Our vectors are represented as displacements along the x and y axes.  */

typedef struct
{
  real dx, dy;
} vector_type;


/* Consider a point as a vector from the origin.  */
extern vector_type make_vector (const real_coordinate_type);

/* And a vector as a point, i.e., a displacement from the origin.  */
extern real_coordinate_type vector_to_point (const vector_type);


/* Definitions for these common operations can be found in any decent
   linear algebra book, and most calculus books.  */

extern real magnitude (const vector_type);
extern vector_type normalize (const vector_type);

extern vector_type Vadd (const vector_type, const vector_type);
extern real Vdot (const vector_type, const vector_type);
extern vector_type Vmult_scalar (const vector_type, const real);
extern real Vangle (const vector_type in, const vector_type out);

/* These operations could have been named `P..._vector' just as well as
   V..._point, so we may as well allow both names.  */
#define Padd_vector Vadd_point
extern real_coordinate_type Vadd_point
  (const real_coordinate_type, const vector_type);

#define Psubtract_vector Vsubtract_point
extern real_coordinate_type Vsubtract_point
  (const real_coordinate_type, const vector_type);

/* This returns the rounded sum.  */
#define IPadd_vector Vadd_int_point
extern coordinate_type Vadd_int_point
  (const coordinate_type, const vector_type);

/* Take the absolute value of both components.  */
extern vector_type Vabs (const vector_type);


/* Operations on points with real coordinates.  It is not orthogonal,
   but more convenient, to have the subtraction operator return a
   vector, and the addition operator return a point.  */
extern vector_type Psubtract
  (const real_coordinate_type, const real_coordinate_type);

/* These are heavily used in spline fitting, so we define them as macros
   instead of functions.  */
#define Padd(rc1, rc2)							\
  ((real_coordinate_type) { (rc1).x + (rc2).x, (rc1).y + (rc2).y })
#define Pmult_scalar(rc, r)						\
  ((real_coordinate_type) { (rc).x * (r), (rc).y * (r) })

/* Similarly, for points with integer coordinates; here, a subtraction
   operator that does return another point is useful.  */
extern vector_type IPsubtract
  (const coordinate_type, const coordinate_type);
extern coordinate_type IPsubtractP
  (const coordinate_type, const coordinate_type);
extern coordinate_type IPadd
  (const coordinate_type, const coordinate_type);
extern coordinate_type IPmult_scalar (const coordinate_type, const int);
extern real_coordinate_type IPmult_real
  (const coordinate_type, const real);
extern boolean IPequal (const coordinate_type, const coordinate_type);

#endif /* not VECTOR_H */
