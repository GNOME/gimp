/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectors-warp.c
 * Copyright (C) 2005 Bill Skaggs  <weskaggs@primate.ucdavis.edu>
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

#include "config.h"

#include <glib-object.h>

#include "vectors-types.h"

#include "libgimpmath/gimpmath.h"

#include "core/gimp-utils.h"
#include "core/gimpcoords.h"

#include "gimpanchor.h"
#include "gimpstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-warp.h"


#define EPSILON 0.2
#define DX      2.0


static void gimp_stroke_warp_point   (const GimpStroke  *stroke,
                                      gdouble            x,
                                      gdouble            y,
                                      GimpCoords        *point_warped,
                                      gdouble            y_offset);

static void gimp_vectors_warp_stroke (const GimpVectors *vectors,
                                      GimpStroke        *stroke,
                                      gdouble            y_offset);


void
gimp_vectors_warp_point (const GimpVectors *vectors,
                         GimpCoords        *point,
                         GimpCoords        *point_warped,
                         gdouble            y_offset)
{
  gdouble     x      = point->x;
  gdouble     y      = point->y;
  gdouble     len;
  GList      *list;
  GimpStroke *stroke;

  for (list = vectors->strokes; list; list = g_list_next (list))
    {
      stroke = list->data;

      len = gimp_vectors_stroke_get_length (vectors, stroke);

      if (x < len)
        break;

      x -= len;
    }

  if (! list)
    {
      point_warped->x = 0;
      point_warped->y = 0;
      return;
    }

  gimp_stroke_warp_point (stroke, x, y, point_warped, y_offset);
}

static void
gimp_stroke_warp_point (const GimpStroke *stroke,
                        gdouble           x,
                        gdouble           y,
                        GimpCoords       *point_warped,
                        gdouble           y_offset)
{
  GimpCoords point_zero  = { 0, };
  GimpCoords point_minus = { 0, };
  GimpCoords point_plus  = { 0, };
  gdouble    slope;
  gdouble    dx, dy, nx, ny, len;

  if (! gimp_stroke_get_point_at_dist (stroke, x, EPSILON,
                                       &point_zero, &slope))
    {
      point_warped->x = 0;
      point_warped->y = 0;
      return;
    }

  point_warped->x = point_zero.x;
  point_warped->y = point_zero.y;

  if (! gimp_stroke_get_point_at_dist (stroke, x - DX, EPSILON,
                                       &point_minus, &slope))
    return;

  if (! gimp_stroke_get_point_at_dist (stroke, x + DX, EPSILON,
                                       &point_plus, &slope))
    return;

  dx = point_plus.x - point_minus.x;
  dy = point_plus.y - point_minus.y;

  len = hypot (dx, dy);

  if (len < 0.01)
    return;

  nx = - dy / len;
  ny =   dx / len;

  point_warped->x = point_zero.x + nx * (y - y_offset);
  point_warped->y = point_zero.y + ny * (y - y_offset);
}

static void
gimp_vectors_warp_stroke (const GimpVectors *vectors,
                          GimpStroke        *stroke,
                          gdouble            y_offset)
{
  GList *list;

  for (list = stroke->anchors; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      gimp_vectors_warp_point (vectors,
                               &anchor->position, &anchor->position,
                               y_offset);
    }
}

void
gimp_vectors_warp_vectors (const GimpVectors *vectors,
                           GimpVectors       *vectors_in,
                           gdouble            y_offset)
{
  GList *list;

  for (list = vectors_in->strokes; list; list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_vectors_warp_stroke (vectors, stroke, y_offset);
    }
}
