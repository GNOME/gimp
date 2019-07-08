/* edge.h: declarations for edge traversing.
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

#ifndef EDGE_H
#define EDGE_H

#include "bitmap.h"

/* We consider each pixel to consist of four edges, and we travel along
   edges, instead of through pixel centers.  This is necessary for those
   unfortunate times when a single pixel is on both an inside and an
   outside outline.

   The numbers chosen here are not arbitrary; the code that figures out
   which edge to move to depends on particular values.  See the
   `TRY_PIXEL' macro in `edge.c'.  To emphasize this, I've written in the
   numbers we need for each edge value.  */

typedef enum
{
  top = 1, left = 2, bottom = 3, right = 0, no_edge = 4
} edge_type;

/* This choice is also not arbitrary: starting at the top edge makes the
   code find outside outlines before inside ones, which is certainly
   what we want.  */
#define START_EDGE  top


/* Return the next outline edge on B in EDGE, ROW, and COL.  */
extern void next_outline_edge (edge_type *edge,
	   	   	       unsigned *row, unsigned *col);

/* Return the next edge after START on the pixel ROW/COL in B that is
   unmarked, according to the MARKED array.  */
extern edge_type next_unmarked_outline_edge (unsigned row, unsigned col,
                                             edge_type start,
                                             bitmap_type marked);

/* Mark the edge E at the pixel ROW/COL in MARKED.  */
extern void mark_edge (edge_type e, unsigned row, unsigned col,
                       bitmap_type *marked);

#endif /* not EDGE_H */
