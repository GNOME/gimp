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
#ifndef __BEZIER_SELECTP_H__
#define __BEZIER_SELECTP_H__

#define BEZIER_ANCHOR    1
#define BEZIER_CONTROL   2

typedef struct _bezier_point BezierPoint;

struct _bezier_point
{
  int type;                   /* type of point (anchor or control) */
  int x, y;                   /* location of point in image space  */
  int sx, sy;                 /* location of point in screen space */
  BezierPoint *next;          /* next point on curve               */
  BezierPoint *prev;          /* prev point on curve               */
};


/*  Functions  */
int  bezier_select_load  (void *, BezierPoint *, int, int);

#endif /* __BEZIER_SELECTP_H__ */
