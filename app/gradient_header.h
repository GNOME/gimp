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
#ifndef __GRADIENT_HEADER_H__
#define __GRADIENT_HEADER_H__

#include <gtk/gtk.h>

/***** Types *****/

/* Gradient segment type */

typedef enum
{
  GRAD_LINEAR = 0,
  GRAD_CURVED,
  GRAD_SINE,
  GRAD_SPHERE_INCREASING,
  GRAD_SPHERE_DECREASING
} grad_type_t;

typedef enum
{
  GRAD_RGB = 0,  /* normal RGB */
  GRAD_HSV_CCW,  /* counterclockwise hue */
  GRAD_HSV_CW    /* clockwise hue */
} grad_color_t;

typedef struct _grad_segment_t grad_segment_t;

struct _gradient_t
{
  gchar          *name;
  gchar          *filename;

  grad_segment_t *segments;
  grad_segment_t *last_visited;

  gboolean        dirty;
  GdkPixmap      *pixmap;
};

struct _grad_segment_t
{
  double       left, middle, right; /* Left pos, midpoint, right pos */
  double       r0, g0, b0, a0;   	/* Left color */
  double       r1, g1, b1, a1;   	/* Right color */
  grad_type_t  type;             	/* Segment's blending function */
  grad_color_t color;             /* Segment's coloring type */

  struct _grad_segment_t *prev, *next; /* For linked list of segments */
};

#endif  /*  __GRADIENT_HEADER_H__  */
