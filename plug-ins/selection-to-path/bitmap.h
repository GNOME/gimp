/* bitmap.h: definition for a bitmap type.  No packing is done by
 *   default; each pixel is represented by an entire byte.  Among other
 *   things, this means the type can be used for both grayscale and binary
 *   images.
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

#ifndef BITMAP_H
#define BITMAP_H

#include <stdio.h>
#include "bounding-box.h"
#include "types.h"


/* If the bitmap holds 8-bit values, rather than one-bit, the
   definition of BLACK here is wrong.  So don't use it in that case!  */
#define WHITE 0
#define BLACK 1


/* The basic structure and macros to access it.  */
typedef struct
{
  dimensions_type dimensions;
  one_byte *bitmap;
} bitmap_type;

/* The dimensions of the bitmap, in pixels.  */
#define BITMAP_DIMENSIONS(b)  ((b).dimensions)

/* The pixels, represented as an array of bytes (in contiguous storage).
   Each pixel is a single byte, even for binary fonts.  */
#define BITMAP_BITS(b)  ((b).bitmap)

/* These are convenient abbreviations for getting inside the members.  */
#define BITMAP_WIDTH(b)  DIMENSIONS_WIDTH (BITMAP_DIMENSIONS (b))
#define BITMAP_HEIGHT(b)  DIMENSIONS_HEIGHT (BITMAP_DIMENSIONS (b))

/* This is the address of the first pixel in the row ROW.  */
#define BITMAP_ROW(b, row)  (BITMAP_BITS (b) + (row) * BITMAP_WIDTH (b))

/* This is the pixel at [ROW,COL].  */
#define BITMAP_PIXEL(b, row, col)					\
  (*(BITMAP_BITS (b) + (row) * BITMAP_WIDTH (b) + (col)))

#define BITMAP_VALID_PIXEL(b, row, col)					\
   	(0 <= (row) && (row) < BITMAP_HEIGHT (b) 			\
         && 0 <= (col) && (col) < BITMAP_WIDTH (b))

/* Assume that the pixel at [ROW,COL] itself is black.  */

#define BITMAP_INTERIOR_PIXEL(b, row, col)				\
   	(0 != (row) && (row) != BITMAP_HEIGHT (b) - 1 			\
         && 0 != (col) && (col) != BITMAP_WIDTH (b) - 1			\
         && BITMAP_PIXEL (b, row - 1, col - 1) == BLACK			\
         && BITMAP_PIXEL (b, row - 1, col) == BLACK			\
         && BITMAP_PIXEL (b, row - 1, col + 1) == BLACK			\
         && BITMAP_PIXEL (b, row, col - 1) == BLACK			\
         && BITMAP_PIXEL (b, row, col + 1) == BLACK			\
         && BITMAP_PIXEL (b, row + 1, col - 1) == BLACK			\
         && BITMAP_PIXEL (b, row + 1, col) == BLACK			\
         && BITMAP_PIXEL (b, row + 1, col + 1) == BLACK)

/* Allocate storage for the bits, set them all to white, and return an
   initialized structure.  */
extern bitmap_type new_bitmap (dimensions_type);

/* Free that storage.  */
extern void free_bitmap (bitmap_type *);

/* Make a fresh copy of BITMAP in a new structure, and return it.  */
extern bitmap_type copy_bitmap (bitmap_type bitmap);

/* Return the pixels in the bitmap B enclosed by the bounding box BB.
   The result is put in newly allocated storage.  */
extern bitmap_type extract_subbitmap (bitmap_type b, bounding_box_type bb);

/* Consider the dimensions of a bitmap as a bounding box.  The bounding
   box returned is in bitmap coordinates, rather than Cartesian, and
   refers to pixels, rather than edges.  Specifically, this means that
   the maximum column is one less than results from `dimensions_to_bb
   (BITMAP_DIMENSIONS ())'.  */
extern bounding_box_type bitmap_to_bb (const bitmap_type);

/* Return a vector of zero-based column numbers marking transitions from
   black to white or white to black in ROW, which is of length WIDTH.
   The end of the vector is marked with an element of length WIDTH + 1.
   The first element always marks a white-to-black transition (or it's
   0, if the first pixel in ROW is black).  */
extern unsigned *bitmap_find_transitions (const one_byte *row, unsigned width);

/* Print part of or all of a bitmap.  */
extern void print_bounded_bitmap (FILE *, bitmap_type, bounding_box_type);
extern void print_bitmap (FILE *, bitmap_type);

#endif /* not BITMAP_H */

