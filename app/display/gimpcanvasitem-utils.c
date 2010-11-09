/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasitem-utils.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "gimpcanvasitem-utils.h"


void
gimp_canvas_item_shift_to_north_west (GimpHandleAnchor  anchor,
                                      gdouble           x,
                                      gdouble           y,
                                      gint              width,
                                      gint              height,
                                      gdouble          *shifted_x,
                                      gdouble          *shifted_y)
{
  switch (anchor)
    {
    case GIMP_HANDLE_ANCHOR_CENTER:
      x -= width  / 2;
      y -= height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH:
      x -= width / 2;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_WEST:
      /*  nothing, this is the default  */
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_EAST:
      x -= width;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH:
      x -= width / 2;
      y -= height;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_WEST:
      y -= height;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_EAST:
      x -= width;
      y -= height;
      break;

    case GIMP_HANDLE_ANCHOR_WEST:
      y -= height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_EAST:
      x -= width;
      y -= height / 2;
      break;

    default:
      break;
    }

  if (shifted_x)
    *shifted_x = x;

  if (shifted_y)
    *shifted_y = y;
}

void
gimp_canvas_item_shift_to_center (GimpHandleAnchor  anchor,
                                  gdouble           x,
                                  gdouble           y,
                                  gint              width,
                                  gint              height,
                                  gdouble          *shifted_x,
                                  gdouble          *shifted_y)
{
  switch (anchor)
    {
    case GIMP_HANDLE_ANCHOR_CENTER:
      /*  nothing, this is the default  */
      break;

    case GIMP_HANDLE_ANCHOR_NORTH:
      y += height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_WEST:
      x += width  / 2;
      y += height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_EAST:
      x -= width  / 2;
      y += height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH:
      y -= height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_WEST:
      x += width  / 2;
      y -= height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_EAST:
      x -= width  / 2;
      y -= height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_WEST:
      x += width / 2;
      break;

    case GIMP_HANDLE_ANCHOR_EAST:
      x -= width / 2;
      break;

    default:
      break;
    }

  if (shifted_x)
    *shifted_x = x;

  if (shifted_y)
    *shifted_y = y;
}
