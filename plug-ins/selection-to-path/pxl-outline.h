/* pxl-outline.h: find a list of outlines which make up one character.
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

#ifndef PXL_OUTLINE_H
#define PXL_OUTLINE_H

#include "types.h"


/* This is a list of contiguous points on the bitmap.  */
typedef struct
{
  coordinate_type *data;
  unsigned length;
  boolean clockwise;
} pixel_outline_type;

/* The Nth coordinate in the list.  */
#define O_COORDINATE(p_o, n) ((p_o).data[n])

/* The length of the list.  */
#define O_LENGTH(p_o) ((p_o).length)

/* Whether the outline moves clockwise or counterclockwise.  */
#define O_CLOCKWISE(p_o) ((p_o).clockwise)

/* Since a pixel outline is cyclic, the index of the next coordinate
   after the last is the first, and the previous coordinate before the
   first is the last.  */
#define O_NEXT(p_o, n) (((n) + 1) % O_LENGTH (p_o))
#define O_PREV(p_o, n) ((n) == 0 ? O_LENGTH (p_o) - 1 : (n) - 1)

/* And the character turns into a list of such lists.  */
typedef struct
{
  pixel_outline_type *data;
  unsigned length;
} pixel_outline_list_type;

/* The Nth list in the list of lists.  */
#define O_LIST_OUTLINE(p_o_l, n) ((p_o_l).data[n])

/* The length of the list of lists.  */
#define O_LIST_LENGTH(p_o_l) ((p_o_l).length)


/* Find all pixels on the outline in the character C.  */
extern pixel_outline_list_type find_outline_pixels (void);

/* Free the memory in the list.  */
extern void free_pixel_outline_list (pixel_outline_list_type *);

#endif /* not PXL_OUTLINE_H */
