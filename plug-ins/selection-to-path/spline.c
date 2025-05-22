/* spline.c: spline and spline list (represented as arrays) manipulation.
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

#include <assert.h>

#include <glib.h>

#include "global.h"
#include "bounding-box.h"
#include "spline.h"
#include "vector.h"

/* Return a new spline structure, initialized with (recognizable)
   garbage.  */

spline_type
new_spline (void)
{
  real_coordinate_type coord = { -100.0, -100.0 };
  spline_type spline;

  START_POINT (spline)
    = CONTROL1 (spline)
    = CONTROL2 (spline)
    = END_POINT (spline)
    = coord;
  SPLINE_DEGREE (spline) = -1;
  SPLINE_LINEARITY (spline) = 0;

  return spline;
}


/* Print a spline in human-readable form.  */

void
print_spline (FILE *f, spline_type s)
{
  if (SPLINE_DEGREE (s) == LINEAR)
    fprintf (f, "(%.3f,%.3f)--(%.3f,%.3f).\n",
                START_POINT (s).x, START_POINT (s).y,
                END_POINT (s).x, END_POINT (s).y);

  else if (SPLINE_DEGREE (s) == CUBIC)
    fprintf (f, "(%.3f,%.3f)..ctrls(%.3f,%.3f)&(%.3f,%.3f)..(%.3f,%.3f).\n",
                START_POINT (s).x, START_POINT (s).y,
                CONTROL1 (s).x, CONTROL1 (s).y,
                CONTROL2 (s).x, CONTROL2 (s).y,
                END_POINT (s).x, END_POINT (s).y);

  else
    {
/*       FATAL1 ("print_spline: strange degree (%d)", SPLINE_DEGREE (s)); */
    }
}

/* Evaluate the spline S at a given T value.  This is an implementation
   of de Casteljau's algorithm.  See Schneider's thesis (reference in
   ../limn/README), p.37.  The variable names are taken from there.  */

real_coordinate_type
evaluate_spline (spline_type s, real t)
{
  spline_type V[4];    /* We need degree+1 splines, but assert degree <= 3.  */
  unsigned i, j;
  real one_minus_t = 1.0 - t;
  polynomial_degree degree = SPLINE_DEGREE (s);

  for (i = 0; i < 4; i++)
    V[i] = new_spline ();

  for (i = 0; i <= degree; i++)
    V[0].v[i] = s.v[i];

  for (j = 1; j <= degree; j++)
    for (i = 0; i <= degree - j; i++)
      {
#if defined (__GNUC__)
        real_coordinate_type t1 = Pmult_scalar (V[j - 1].v[i], one_minus_t);
        real_coordinate_type t2 = Pmult_scalar (V[j - 1].v[i + 1], t);
        V[j].v[i] = Padd (t1, t2);
#else
	/* HB: the above is really nice, but is there any other compiler
	 * supporting this ??
	 */
        real_coordinate_type t1;
        real_coordinate_type t2;
        t1.x = V[j - 1].v[i].x * one_minus_t;
        t1.y = V[j - 1].v[i].y * one_minus_t;
        t2.x = V[j - 1].v[i + 1].x * t;
        t2.y = V[j - 1].v[i + 1].y * t;
        V[j].v[i].x = t1.x + t2.x;
        V[j].v[i].y = t1.y + t2.y;
#endif
      }

  return V[degree].v[0];
}



/* Return a new, empty, spline list.  */

spline_list_type *
new_spline_list (void)
{
  spline_list_type *answer = g_new (spline_list_type, 1);

  SPLINE_LIST_DATA (*answer) = NULL;
  SPLINE_LIST_LENGTH (*answer) = 0;

  return answer;
}


/* Return a new spline list with SPLINE as the first element.  */

spline_list_type *
init_spline_list (spline_type spline)
{
  spline_list_type *answer = g_new (spline_list_type, 1);

  SPLINE_LIST_DATA (*answer) = g_new (spline_type, 1);
  SPLINE_LIST_ELT (*answer, 0) = spline;
  SPLINE_LIST_LENGTH (*answer) = 1;

  return answer;
}


/* Free the storage in a spline list.  We don't have to free the
   elements, since they are arrays in automatic storage.  And we don't
   want to free the list if it was empty.  */

void
free_spline_list (spline_list_type *spline_list)
{
  if (SPLINE_LIST_DATA (*spline_list) != NULL)
    safe_free ((address *) &(SPLINE_LIST_DATA (*spline_list)));
}


/* Append the spline S to the list SPLINE_LIST.  */

void
append_spline (spline_list_type *l, spline_type s)
{
  assert (l != NULL);

  SPLINE_LIST_LENGTH (*l)++;
  SPLINE_LIST_DATA (*l) = g_realloc (SPLINE_LIST_DATA (*l),
                               SPLINE_LIST_LENGTH (*l) * sizeof (spline_type));
  LAST_SPLINE_LIST_ELT (*l) = s;
}


/* Tack the elements in the list S2 onto the end of S1.
   S2 is not changed.  */

void
concat_spline_lists (spline_list_type *s1, spline_list_type s2)
{
  unsigned this_spline;
  unsigned new_length;

  assert (s1 != NULL);

  new_length = SPLINE_LIST_LENGTH (*s1) + SPLINE_LIST_LENGTH (s2);

  SPLINE_LIST_DATA (*s1) = g_realloc(SPLINE_LIST_DATA (*s1),new_length * sizeof(spline_type));

  for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (s2); this_spline++)
    SPLINE_LIST_ELT (*s1, SPLINE_LIST_LENGTH (*s1)++)
      = SPLINE_LIST_ELT (s2, this_spline);
}


/* Return a new, empty, spline list array.  */

spline_list_array_type
new_spline_list_array (void)
{
  spline_list_array_type answer;

  SPLINE_LIST_ARRAY_DATA (answer) = NULL;
  SPLINE_LIST_ARRAY_LENGTH (answer) = 0;

  return answer;
}


/* Free the storage in a spline list array.  We don't
   want to free the list if it is empty.  */

void
free_spline_list_array (spline_list_array_type *spline_list_array)
{
  unsigned this_list;

  for (this_list = 0;
       this_list < SPLINE_LIST_ARRAY_LENGTH (*spline_list_array);
       this_list++)
    free_spline_list (&SPLINE_LIST_ARRAY_ELT (*spline_list_array, this_list));

  if (SPLINE_LIST_ARRAY_DATA (*spline_list_array) != NULL)
    safe_free ((address *) &(SPLINE_LIST_ARRAY_DATA (*spline_list_array)));
}


/* Append the spline S to the list SPLINE_LIST_ARRAY.  */

void
append_spline_list (spline_list_array_type *l, spline_list_type s)
{
  SPLINE_LIST_ARRAY_LENGTH (*l)++;

  SPLINE_LIST_ARRAY_DATA (*l) = g_realloc(SPLINE_LIST_ARRAY_DATA (*l),(SPLINE_LIST_ARRAY_LENGTH (*l))*sizeof(spline_list_type));
  LAST_SPLINE_LIST_ARRAY_ELT (*l) = s;
}
