/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef  __BOUNDARY_H__
#define  __BOUNDARY_H__

typedef struct _BoundSeg BoundSeg;
struct _PixelArea;

struct _BoundSeg
{
  int x1, y1;
  int x2, y2;
  int open;
  int visited;
};

typedef enum
{
  WithinBounds,
  IgnoreBounds
} BoundaryType;



BoundSeg *  find_mask_boundary (struct _PixelArea *, int *, BoundaryType, int, int, int, int);
BoundSeg *  sort_boundary (BoundSeg *, int, int *);

#endif  /*  __BOUNDARY_H__  */
