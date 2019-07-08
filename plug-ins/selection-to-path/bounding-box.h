/* bounding-box.h: operations on both real- and integer-valued bounding boxes.
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

#ifndef BOUNDING_BOX_H
#define BOUNDING_BOX_H

#include "types.h"


/* The bounding box's numbers are usually in Cartesian/Metafont
   coordinates: (0,0) is towards the lower left.  */
typedef struct
{
  signed_4_bytes min_row, max_row;
  signed_4_bytes min_col, max_col;
} bounding_box_type;

typedef struct
{
  real min_row, max_row;
  real min_col, max_col;
} real_bounding_box_type;

/* These accessing macros work for both types of bounding boxes, since
   the member names are the same.  */
#define MIN_ROW(bb) ((bb).min_row)
#define MAX_ROW(bb) ((bb).max_row)
#define MIN_COL(bb) ((bb).min_col)
#define MAX_COL(bb) ((bb).max_col)

/* See the comments at `get_character_bitmap' in gf_input.c for why the
   width and height are treated asymmetrically.  */
#define BB_WIDTH(bb) (MAX_COL (bb) - MIN_COL (bb))
#define BB_HEIGHT(bb) (MAX_ROW (bb) - MIN_ROW (bb) + 1)


/* Convert a dimensions structure to an integer bounding box, and vice
   versa.  */
extern bounding_box_type dimensions_to_bb (dimensions_type);
extern dimensions_type bb_to_dimensions (bounding_box_type);


/* Update the bounding box BB from the point P.  */
extern void update_real_bounding_box (real_bounding_box_type *bb,
                                      real_coordinate_type p);
extern void update_bounding_box (bounding_box_type *bb, coordinate_type p);

#endif /* not BOUNDING_BOX_H */
