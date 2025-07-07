/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppath-warp.c
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

#include "path-types.h"

#include "libgimpmath/gimpmath.h"

#include "core/gimp-utils.h"
#include "core/gimpcoords.h"

#include "gimpanchor.h"
#include "gimpstroke.h"
#include "gimppath.h"
#include "gimppath-warp.h"


#define EPSILON 0.2
#define DX      2.0


static void   gimp_stroke_warp_point   (GimpStroke  *stroke,
                                        gdouble      x,
                                        gdouble      y,
                                        GimpCoords  *point_warped,
                                        gdouble      y_offset,
                                        gdouble      x_len);

static void   gimp_path_warp_stroke    (GimpPath    *path,
                                        GimpStroke  *stroke,
                                        gdouble      y_offset);


void
gimp_path_warp_point (GimpPath   *path,
                      GimpCoords *point,
                      GimpCoords *point_warped,
                      gdouble     y_offset)
{
  gdouble     x      = point->x;
  gdouble     y      = point->y;
  gdouble     len;
  GList      *list;
  GimpStroke *stroke;

  for (list = path->strokes->head;
       list;
       list = g_list_next (list))
    {
      stroke = list->data;

      len = gimp_path_stroke_get_length (path, stroke);

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

  gimp_stroke_warp_point (stroke, x, y, point_warped, y_offset, len);
}

static void
gimp_stroke_warp_point (GimpStroke *stroke,
                        gdouble     x,
                        gdouble     y,
                        GimpCoords *point_warped,
                        gdouble     y_offset,
                        gdouble     x_len)
{
  GimpCoords point_zero  = { 0, };
  GimpCoords point_minus = { 0, };
  GimpCoords point_plus  = { 0, };
  gdouble    slope;
  gdouble    dx, dy, nx, ny, len;

  if (x + DX >= x_len)
    {
      gdouble tx, ty;

      if (! gimp_stroke_get_point_at_dist (stroke, x_len, EPSILON,
                                           &point_zero, &slope))
        {
          point_warped->x = 0;
          point_warped->y = 0;
          return;
        }

      point_warped->x = point_zero.x;
      point_warped->y = point_zero.y;

      if (! gimp_stroke_get_point_at_dist (stroke, x_len - DX, EPSILON,
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
gimp_path_warp_stroke (GimpPath   *path,
                       GimpStroke *stroke,
                       gdouble     y_offset)
{
  GList *list;

  for (list = stroke->anchors->head; list; list = g_list_next (list))
    {
      GimpAnchor *anchor = list->data;

      gimp_path_warp_point (path,
                            &anchor->position, &anchor->position,
                            y_offset);
    }
}

void
gimp_path_warp_path (GimpPath *path,
                     GimpPath *path_in,
                     gdouble   y_offset)
{
  GList *list;

  for (list = path_in->strokes->head;
       list;
       list = g_list_next (list))
    {
      GimpStroke *stroke = list->data;

      gimp_path_warp_stroke (path, stroke, y_offset);
    }
}
