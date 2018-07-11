/* curve.h: data structures for the conversion from pixels to splines.
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

#ifndef CURVE_H
#define CURVE_H

#include "types.h"
#include "vector.h"


/* We are simultaneously manipulating two different representations of
   the same outline: one based on (x,y) positions in the plane, and one
   based on parametric splines.  (We are trying to match the latter to
   the former.)  Although the original (x,y)'s are pixel positions,
   i.e., integers, after filtering they are reals.  */

typedef struct
{
  real_coordinate_type coord;
  real t;
} point_type;


/* It turns out to be convenient to break the list of all the pixels in
   the outline into sublists, divided at ``corners''.  Then each of the
   sublists is treated independently.  Each of these sublists is a `curve'.  */

struct curve
{
  point_type *point_list;
  int length;
  boolean cyclic;
  vector_type *start_tangent;
  vector_type *end_tangent;
  struct curve *previous;
  struct curve *next;
};

typedef struct curve *curve_type;

/* Get at the coordinates and the t values.  */
#define CURVE_POINT(c, n) ((c)->point_list[n].coord)
#define LAST_CURVE_POINT(c) ((c)->point_list[(c)->length-1].coord)
#define CURVE_T(c, n) ((c)->point_list[n].t)
#define LAST_CURVE_T(c) ((c)->point_list[(c)->length-1].t)

/* This is the length of `point_list'.  */
#define CURVE_LENGTH(c)  ((c)->length)

/* A curve is ``cyclic'' if it didn't have any corners, after all, so
   the last point is adjacent to the first.  */
#define CURVE_CYCLIC(c)  ((c)->cyclic)

/* If the curve is cyclic, the next and previous points should wrap
   around; otherwise, if we get to the end, we return CURVE_LENGTH and
   -1, respectively.  */
#define CURVE_NEXT(c, n)						\
  ((n) + 1 >= CURVE_LENGTH (c)						\
  ? CURVE_CYCLIC (c) ? ((n) + 1) % CURVE_LENGTH (c) : CURVE_LENGTH (c)	\
  : (n) + 1)
#define CURVE_PREV(c, n)						\
  ((int) (n) - 1 < 0							\
  ? CURVE_CYCLIC (c) ? CURVE_LENGTH (c) + (int) (n) - 1 : -1		\
  : (int) (n) - 1)

/* The tangents at the endpoints are computed using the neighboring curves.  */
#define CURVE_START_TANGENT(c) ((c)->start_tangent)
#define CURVE_END_TANGENT(c) ((c)->end_tangent)
#define PREVIOUS_CURVE(c) ((c)->previous)
#define NEXT_CURVE(c) ((c)->next)


/* Return an entirely empty curve.  */
extern curve_type new_curve (void);

/* Return a curve with the point P as its first element.  */
extern curve_type init_curve (coordinate_type p);

/* Return a curve the same as C, except without any points.  */
extern curve_type copy_most_of_curve (curve_type c);

/* Free the memory C uses.  */
extern void free_curve (curve_type c);

/* Append the point P to the end of C's list.  */
extern void append_pixel (curve_type c, coordinate_type p);

/* Like `append_pixel', for a point in real coordinates.  */
extern void append_point (curve_type c, real_coordinate_type p);

/* Write some or all, respectively, of the curve C in human-readable
   form to the log file, if logging is enabled.  */
extern void log_curve (curve_type c, boolean print_t);
extern void log_entire_curve (curve_type c);

/* Display the curve C online, if displaying is enabled.  */
extern void display_curve (curve_type);

/* So, an outline is a list of curves.  */
typedef struct
{
  curve_type *data;
  unsigned length;
  boolean clockwise;
} curve_list_type;

/* Number of curves in the list.  */
#define CURVE_LIST_LENGTH(c_l)  ((c_l).length)

/* Access the individual curves.  */
#define CURVE_LIST_ELT(c_l, n) ((c_l).data[n])
#define LAST_CURVE_LIST_ELT(c_l) ((c_l).data[CURVE_LIST_LENGTH (c_l) - 1])

/* Says whether the outline that this curve list represents moves
   clockwise or counterclockwise.  */
#define CURVE_LIST_CLOCKWISE(c_l) ((c_l).clockwise)


extern curve_list_type new_curve_list (void);
extern void free_curve_list (curve_list_type *);
extern void append_curve (curve_list_type *, curve_type);

/* And a character is a list of outlines.  I named this
   `curve_list_array_type' because `curve_list_list_type' seemed pretty
   monstrous.  */
typedef struct
{
  curve_list_type *data;
  unsigned length;
} curve_list_array_type;

/* Turns out we can use the same definitions for lists of lists as for
   just lists.  But we define the usual names, just in case.  */
#define CURVE_LIST_ARRAY_LENGTH CURVE_LIST_LENGTH
#define CURVE_LIST_ARRAY_ELT CURVE_LIST_ELT
#define LAST_CURVE_LIST_ARRAY_ELT LAST_CURVE_LIST_ELT

extern curve_list_array_type new_curve_list_array (void);
extern void free_curve_list_array (curve_list_array_type *);
extern void append_curve_list (curve_list_array_type *, curve_list_type);

#endif /* not CURVE_H */
