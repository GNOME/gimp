/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimprectangle.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib.h>

#include "gimprectangle.h"


/**
 * gimp_rectangle_intersect:
 * @x1:          origin of first rectangle
 * @y1:          origin of first rectangle
 * @width1:      width of first rectangle
 * @height1:     height of first rectangle
 * @x2:          origin of second rectangle
 * @y2:          origin of second rectangle
 * @width2:      width of second rectangle
 * @height2:     height of second rectangle
 * @dest_x:      return location for origin of intersection (may be %NULL)
 * @dest_y:      return location for origin of intersection (may be %NULL)
 * @dest_width:  return location for width of intersection (may be %NULL)
 * @dest_height: return location for height of intersection (may be %NULL)
 *
 * Calculates the intersection of two rectangles.
 *
 * Return value: %TRUE if the intersection is non-empty, %FALSE otherwise
 *
 * Since: GIMP 2.4
 */
gboolean
gimp_rectangle_intersect (gint  x1,
                          gint  y1,
                          gint  width1,
                          gint  height1,
                          gint  x2,
                          gint  y2,
                          gint  width2,
                          gint  height2,
                          gint *dest_x,
                          gint *dest_y,
                          gint *dest_width,
                          gint *dest_height)
{
  gint d_x, d_y;
  gint d_w, d_h;

  d_x = MAX (x1, x2);
  d_y = MAX (y1, y2);
  d_w = MIN (x1 + width1,  x2 + width2)  - d_x;
  d_h = MIN (y1 + height1, y2 + height2) - d_y;

  if (dest_x)      *dest_x      = d_x;
  if (dest_y)      *dest_y      = d_y;
  if (dest_width)  *dest_width  = d_w;
  if (dest_height) *dest_height = d_h;

  return (d_w > 0 && d_h > 0);
}
