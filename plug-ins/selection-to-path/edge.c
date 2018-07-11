/* edge.c: operations on edges in bitmaps.
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

#include "types.h"
#include "selection-to-path.h"
#include "edge.h"

/* We can move in any of eight directions as we are traversing
   the outline.  These numbers are not arbitrary; TRY_PIXEL depends on
   them.  */

typedef enum
{
  north = 0, northwest = 1, west = 2, southwest = 3, south = 4,
  southeast = 5, east = 6, northeast = 7
} direction_type;


static boolean is_marked_edge (edge_type, unsigned, unsigned, bitmap_type);
static boolean is_outline_edge (edge_type, unsigned, unsigned);
static edge_type next_edge (edge_type);

/* The following macros are used (directly or indirectly) by the
   `next_outline_edge' routine.  */

/* Given the direction DIR of the pixel to test, decide which edge on
   that pixel we are supposed to test.  Because we've chosen the mapping
   from directions to numbers carefully, we don't have to do much.  */

#define FIND_TEST_EDGE(dir) ((dir) / 2)


/* Find how to move in direction DIR on the axis AXIS (either `ROW' or
  `COL').   We are in the ``display'' coordinate system, with y
  increasing downward and x increasing to the right.  Therefore, we are
  implementing the following table:

  direction  row delta  col delta
    north       -1          0
    south	+1	    0
    east	 0	   +1
    west	 0	   +1

  with the other four directions (e.g., northwest) being the sum of
  their components (e.g., north + west).

  The first macro, `COMPUTE_DELTA', handles splitting up the latter
  cases, all of which have been assigned odd numbers.  */

#define COMPUTE_DELTA(axis, dir)					\
  ((dir) % 2 != 0							\
    ? COMPUTE_##axis##_DELTA ((dir) - 1)				\
      + COMPUTE_##axis##_DELTA (((dir) + 1) % 8)			\
    : COMPUTE_##axis##_DELTA (dir)					\
  )

/* Now it is trivial to implement the four cardinal directions.  */
#define COMPUTE_ROW_DELTA(dir)						\
  ((dir) == north ? -1 : (dir) == south ? +1 : 0)

#define COMPUTE_COL_DELTA(dir)						\
  ((dir) == west ? -1 : (dir) == east ? +1 : 0)


/* See if the appropriate edge on the pixel from (row,col) in direction
   DIR is on the outline.  If so, update `row', `col', and `edge', and
   break.  We also use the variable `character' as the bitmap in which
   to look.  */

#define TRY_PIXEL(dir)							\
  {									\
    int delta_r = COMPUTE_DELTA (ROW, dir);				\
    int delta_c = COMPUTE_DELTA (COL, dir);				\
    int test_row = *row + delta_r;					\
    int test_col = *col + delta_c;					\
    edge_type test_edge = FIND_TEST_EDGE (dir);				\
									\
    if (sel_valid_pixel(test_row, test_col)               		\
        && is_outline_edge (test_edge, test_row, test_col))     	\
      {									\
        *row = test_row;						\
        *col = test_col;						\
        *edge = test_edge;						\
        break;								\
      }									\
  }

/* Finally, we are ready to implement the routine that finds the next
   edge on the outline.  We look first for an adjacent edge that is not
   on the current pixel.  We want to go around outside outlines
   counterclockwise, and inside outlines clockwise (because that is how
   both Metafont and Adobe Type 1 format want their curves to be drawn).

   The very first outline (an outside one) on each character starts on a
   top edge (STARTING_EDGE in edge.h defines this); so, if we're at a
   top edge, we want to go only to the left (on the pixel to the west)
   or down (on the same pixel), to begin with.  Then, when we're on a
   left edge, we want to go to the top edge (on the southwest pixel) or
   to the left edge (on the south pixel).

   All well and good. But if you draw a rasterized circle (or whatever),
   eventually we have to come back around to the beginning; at that
   point, we'll be on a top edge, and we'll have to go to the right edge
   on the northwest pixel.  Draw pictures.

   The upshot is, if we find an edge on another pixel, we return (in ROW
   and COL) the position of the new pixel, and (in EDGE) the kind of
   edge it is.  If we don't find such an edge, we return (in EDGE) the
   next (in a counterclockwise direction) edge on the current pixel.  */

void
next_outline_edge (edge_type *edge,
		   unsigned *row, unsigned *col)
{
  unsigned original_row = *row;
  unsigned original_col = *col;

  switch (*edge)
    {
    case right:
      TRY_PIXEL (north);
      TRY_PIXEL (northeast);
      break;

    case top:
      TRY_PIXEL (west);
      TRY_PIXEL (northwest);
      break;

    case left:
      TRY_PIXEL (south);
      TRY_PIXEL (southwest);
      break;

    case bottom:
      TRY_PIXEL (east);
      TRY_PIXEL (southeast);
      break;

    default:
      printf ("next_outline_edge: Bad edge value (%d)", *edge);

    }

  /* If we didn't find an adjacent edge on another pixel, return the
     next edge on the current pixel.  */
  if (*row == original_row && *col == original_col)
    *edge = next_edge (*edge);
}

/* We return the next edge on the pixel at position ROW and COL which is
   an unmarked outline edge.  By ``next'' we mean either the one sent in
   in STARTING_EDGE, if it qualifies, or the next such returned by
   `next_edge'.  */

edge_type
next_unmarked_outline_edge (unsigned row, unsigned col,
	                    edge_type starting_edge,
                            bitmap_type marked)
{
  edge_type edge = starting_edge;

  assert (edge != no_edge);

  while (is_marked_edge (edge, row, col, marked)
	 || !is_outline_edge (edge, row, col))
    {
      edge = next_edge (edge);
      if (edge == starting_edge)
        return no_edge;
    }

  return edge;
}


/* We check to see if the edge EDGE of the pixel at position ROW and COL
   is an outline edge; i.e., that it is a black pixel which shares that
   edge with a white pixel.  The position ROW and COL should be inside
   the bitmap CHARACTER.  */

static boolean
is_outline_edge (edge_type edge,
		 unsigned row, unsigned col)
{
  /* If this pixel isn't black, it's not part of the outline.  */
  if (sel_pixel_is_white(row, col))
    return false;

  switch (edge)
    {
    case left:
      return col == 0 || sel_pixel_is_white(row, col - 1);

    case top:
      return row == 0 || sel_pixel_is_white(row - 1, col);

    case right:
      return (col ==  sel_get_width() - 1)
	|| sel_pixel_is_white(row, col + 1);

    case bottom:
      return (row ==  sel_get_height() - 1)
	|| sel_pixel_is_white(row + 1, col);

    case no_edge:
    default:
      printf ("is_outline_edge: Bad edge value(%d)", edge);
    }

  return 0; /* NOTREACHED */
}

/* If EDGE is not already marked, we mark it; otherwise, it's a fatal error.
   The position ROW and COL should be inside the bitmap MARKED.  EDGE can
   be `no_edge'; we just return false.  */

void
mark_edge (edge_type edge, unsigned row, unsigned col, bitmap_type *marked)
{
  /* printf("row = %d, col = %d \n",row,col); */
  assert (!is_marked_edge (edge, row, col, *marked));

  if (edge != no_edge)
    BITMAP_PIXEL (*marked, row, col) |= 1 << edge;
}


/* Test if the edge EDGE at ROW/COL in MARKED is marked.  */

static boolean
is_marked_edge (edge_type edge, unsigned row, unsigned col, bitmap_type marked)
{
  return
    edge == no_edge ? false : BITMAP_PIXEL (marked, row, col) & (1 << edge);
}


/* Return the edge which is counterclockwise-adjacent to EDGE.  This
   code makes use of the ``numericness'' of C enumeration constants;
   sorry about that.  */

#define NUM_EDGES no_edge

static edge_type
next_edge (edge_type edge)
{
  return edge == no_edge ? edge : (edge + 1) % NUM_EDGES;
}
