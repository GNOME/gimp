/* pxl-outline.c: find the edges of the bitmap image; we call each such
 *   edge an ``outline''; each outline is made up of one or more pixels;
 *   and each pixel participates via one or more edges.
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

#include "global.h"
#include "selection-to-path.h"
#include "bitmap.h"
#include "edge.h"
#include "pxl-outline.h"

#include "libgimp/stdplugins-intl.h"

static pixel_outline_type find_one_outline (edge_type,
			  		    unsigned, unsigned, bitmap_type *);
static void append_pixel_outline (pixel_outline_list_type *,
                                  pixel_outline_type);
static pixel_outline_type new_pixel_outline (void);
static void append_outline_pixel (pixel_outline_type *, coordinate_type);
static void append_coordinate (pixel_outline_type *, int, int, edge_type);


static bitmap_type
local_new_bitmap (unsigned width,unsigned height)
{
  bitmap_type answer;
  unsigned size = width * height;


  BITMAP_HEIGHT(answer) = height;
  BITMAP_WIDTH(answer) = width;

  BITMAP_BITS (answer) = g_new0 (one_byte, size);  /* g_new returns NULL if size == 0 */

/*   printf("local_new_bitmap size = %d @[%p]\n",size,BITMAP_BITS (answer)); */

  return answer;
}

static void
local_free_bitmap (bitmap_type *b)
{
  if (BITMAP_BITS (*b) != NULL)
    safe_free ((address *) &BITMAP_BITS (*b));
}

/* A character is made up of a list of one or more outlines.  Here, we
   go through a character's bitmap top to bottom, left to right, looking
   for the next pixel with an unmarked edge also on the character's outline.
   Each one of these we find is the starting place for one outline.  We
   find these outlines and put them in a list to return.  */

pixel_outline_list_type
find_outline_pixels (void)
{
  pixel_outline_list_type outline_list;
  unsigned row, col;
  gint height;
  gint width;
  bitmap_type marked = local_new_bitmap (sel_get_width(),sel_get_height());

/*   printf("width = %d, height = %d\n",BITMAP_WIDTH(marked),BITMAP_HEIGHT(marked)); */

  gimp_progress_init (_("Selection to Path"));

  O_LIST_LENGTH (outline_list) = 0;
  outline_list.data = NULL;

  height = sel_get_height ();
  width  = sel_get_width ();

  for (row = 0; row < height; row++)
  {
    for (col = 0; col < width; col++)
      {
	edge_type edge;

        if (sel_pixel_is_white(row, col))
          continue;

        edge = next_unmarked_outline_edge (row, col, START_EDGE,marked);

	if (edge != no_edge)
	  {
            pixel_outline_type outline;
            boolean clockwise = edge == bottom;

            outline = find_one_outline (edge, row, col, &marked);

            /* Outside outlines will start at a top edge, and move
               counterclockwise, and inside outlines will start at a
               bottom edge, and move clockwise.  This happens because of
               the order in which we look at the edges.  */
            O_CLOCKWISE (outline) = clockwise;
	    append_pixel_outline (&outline_list, outline);

	  }
      }

    if ((row & 0xf) == 0)
	gimp_progress_update (((gdouble)row) / height);
  }

  gimp_progress_update (1.0);

  local_free_bitmap (&marked);

  return outline_list;
}


/* Here we find one of a character C's outlines.  We're passed the
   position (ORIGINAL_ROW and ORIGINAL_COL) of a starting pixel and one
   of its unmarked edges, ORIGINAL_EDGE.  We traverse the adjacent edges
   of the outline pixels, appending to the coordinate list.  We keep
   track of the marked edges in MARKED, so it should be initialized to
   zeros when we first get it.  */

static pixel_outline_type
find_one_outline (edge_type original_edge,
		  unsigned original_row, unsigned original_col,
		  bitmap_type *marked)
{
  pixel_outline_type outline = new_pixel_outline ();
  unsigned row = original_row, col = original_col;
  edge_type edge = original_edge;

  do
    {
      /* Put this edge on to the output list, changing to Cartesian, and
         taking account of the side bearings.  */
      append_coordinate (&outline, col,
                         sel_get_height() - row, edge);

      mark_edge (edge, row, col, marked);
      next_outline_edge (&edge, &row, &col);
    }
  while (row != original_row || col != original_col || edge != original_edge);

  return outline;
}


/* Append an outline to an outline list.  This is called when we have
   completed an entire pixel outline.  */

static void
append_pixel_outline (pixel_outline_list_type *outline_list,
		      pixel_outline_type outline)
{
  O_LIST_LENGTH (*outline_list)++;
  outline_list->data = (pixel_outline_type *)g_realloc(outline_list->data,outline_list->length *sizeof(pixel_outline_type));
  O_LIST_OUTLINE (*outline_list, O_LIST_LENGTH (*outline_list) - 1) = outline;
}


/* Here is a routine that frees a list of such lists.  */

void
free_pixel_outline_list (pixel_outline_list_type *outline_list)
{
  unsigned this_outline;

  for (this_outline = 0; this_outline < outline_list->length; this_outline++)
    {
      pixel_outline_type o = outline_list->data[this_outline];
      safe_free ((address *) &(o.data));
    }

  if (outline_list->data != NULL)
    safe_free ((address *) &(outline_list->data));
}


/* Return an empty list of pixels.  */


static pixel_outline_type
new_pixel_outline (void)
{
  pixel_outline_type pixel_outline;

  O_LENGTH (pixel_outline) = 0;
  pixel_outline.data = NULL;

  return pixel_outline;
}


/* Add the coordinate C to the pixel list O.  */

static void
append_outline_pixel (pixel_outline_type *o, coordinate_type c)
{
  O_LENGTH (*o)++;
  o->data = (coordinate_type *)g_realloc(o->data, O_LENGTH (*o)*sizeof(coordinate_type));
  O_COORDINATE (*o, O_LENGTH (*o) - 1) = c;
}


/* We are given an (X,Y) in Cartesian coordinates, and the edge of the pixel
   we're on.  We append a corner of that pixel as our coordinate.
   If we're on a top edge, we use the upper-left hand corner; right edge
   => upper right; bottom edge => lower right; left edge => lower left.  */

static void
append_coordinate (pixel_outline_type *o, int x, int y, edge_type edge)
{
  coordinate_type c;

  c.x = x;
  c.y = y;

  switch (edge)
    {
    case top:
      c.y++;
      break;

    case right:
      c.x++;
      c.y++;
      break;

    case bottom:
      c.x++;
      break;

    case left:
      break;

    default:
      g_printerr ("append_coordinate: Bad edge (%d)", edge);
    }

  append_outline_pixel (o, c);
}
