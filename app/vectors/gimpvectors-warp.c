/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmavectors-warp.c
 * Copyright (C) 2005 Bill Skaggs  <weskaggs@primate.ucdavis.edu>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "vectors-types.h"

#include "libligmamath/ligmamath.h"

#include "core/ligma-utils.h"
#include "core/ligmacoords.h"

#include "ligmaanchor.h"
#include "ligmastroke.h"
#include "ligmavectors.h"
#include "ligmavectors-warp.h"


#define EPSILON 0.2
#define DX      2.0


static void   ligma_stroke_warp_point   (LigmaStroke  *stroke,
                                        gdouble      x,
                                        gdouble      y,
                                        LigmaCoords  *point_warped,
                                        gdouble      y_offset,
                                        gdouble      x_len);

static void   ligma_vectors_warp_stroke (LigmaVectors *vectors,
                                        LigmaStroke  *stroke,
                                        gdouble      y_offset);


void
ligma_vectors_warp_point (LigmaVectors *vectors,
                         LigmaCoords  *point,
                         LigmaCoords  *point_warped,
                         gdouble      y_offset)
{
  gdouble     x      = point->x;
  gdouble     y      = point->y;
  gdouble     len;
  GList      *list;
  LigmaStroke *stroke;

  for (list = vectors->strokes->head;
       list;
       list = g_list_next (list))
    {
      stroke = list->data;

      len = ligma_vectors_stroke_get_length (vectors, stroke);

      if (x < len || ! list->next)
        break;

      x -= len;
    }

  if (! list)
    {
      point_warped->x = 0;
      point_warped->y = 0;
      return;
    }

  ligma_stroke_warp_point (stroke, x, y, point_warped, y_offset, len);
}

static void
ligma_stroke_warp_point (LigmaStroke *stroke,
                        gdouble     x,
                        gdouble     y,
                        LigmaCoords *point_warped,
                        gdouble     y_offset,
                        gdouble     x_len)
{
  LigmaCoords point_zero  = { 0, };
  LigmaCoords point_minus = { 0, };
  LigmaCoords point_plus  = { 0, };
  gdouble    slope;
  gdouble    dx, dy, nx, ny, len;

  if (x + DX >= x_len)
    {
      gdouble tx, ty;

      if (! ligma_stroke_get_point_at_dist (stroke, x_len, EPSILON,
                                           &point_zero, &slope))
        {
          point_warped->x = 0;
          point_warped->y = 0;
          return;
        }

      point_warped->x = point_zero.x;
      point_warped->y = point_zero.y;

      if (! ligma_stroke_get_point_at_dist (stroke, x_len - DX, EPSILON,
                                           &point_minus, &slope))
        return;

      dx = point_zero.x - point_minus.x;
      dy = point_zero.y - point_minus.y;

      len = hypot (dx, dy);

      if (len < 0.01)
        return;

      tx = dx / len;
      ty = dy / len;

      nx = - dy / len;
      ny =   dx / len;

      point_warped->x += tx * (x - x_len) + nx * (y - y_offset);
      point_warped->y += ty * (x - x_len) + ny * (y - y_offset);

      return;
    }

  if (! ligma_stroke_get_point_at_dist (stroke, x, EPSILON,
                                       &point_zero, &slope))
    {
      point_warped->x = 0;
      point_warped->y = 0;
      return;
    }

  point_warped->x = point_zero.x;
  point_warped->y = point_zero.y;

  if (! ligma_stroke_get_point_at_dist (stroke, x - DX, EPSILON,
                                       &point_minus, &slope))
    return;

  if (! ligma_stroke_get_point_at_dist (stroke, x + DX, EPSILON,
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
ligma_vectors_warp_stroke (LigmaVectors *vectors,
                          LigmaStroke  *stroke,
                          gdouble      y_offset)
{
  GList *list;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      LigmaAnchor *anchor = list->data;

      ligma_vectors_warp_point (vectors,
                               &anchor->position, &anchor->position,
                               y_offset);
    }
}

void
ligma_vectors_warp_vectors (LigmaVectors *vectors,
                           LigmaVectors *vectors_in,
                           gdouble      y_offset)
{
  GList *list;

  for (list = vectors_in->strokes->head;
       list;
       list = g_list_next (list))
    {
      LigmaStroke *stroke = list->data;

      ligma_vectors_warp_stroke (vectors, stroke, y_offset);
    }
}
