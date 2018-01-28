/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimp-transform-utils.h"


#define EPSILON 1e-6
#define NEAR_Z  0.02


void
gimp_transform_get_rotate_center (gint      x,
                                  gint      y,
                                  gint      width,
                                  gint      height,
                                  gboolean  auto_center,
                                  gdouble  *center_x,
                                  gdouble  *center_y)
{
  g_return_if_fail (center_x != NULL);
  g_return_if_fail (center_y != NULL);

  if (auto_center)
    {
      *center_x = (gdouble) x + (gdouble) width  / 2.0;
      *center_y = (gdouble) y + (gdouble) height / 2.0;
    }
}

void
gimp_transform_get_flip_axis (gint                 x,
                              gint                 y,
                              gint                 width,
                              gint                 height,
                              GimpOrientationType  flip_type,
                              gboolean             auto_center,
                              gdouble             *axis)
{
  g_return_if_fail (axis != NULL);

  if (auto_center)
    {
      switch (flip_type)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          *axis = ((gdouble) x + (gdouble) width / 2.0);
          break;

        case GIMP_ORIENTATION_VERTICAL:
          *axis = ((gdouble) y + (gdouble) height / 2.0);
          break;

        default:
          g_return_if_reached ();
          break;
        }
    }
}

void
gimp_transform_matrix_flip (GimpMatrix3         *matrix,
                            GimpOrientationType  flip_type,
                            gdouble              axis)
{
  g_return_if_fail (matrix != NULL);

  switch (flip_type)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      gimp_matrix3_translate (matrix, - axis, 0.0);
      gimp_matrix3_scale (matrix, -1.0, 1.0);
      gimp_matrix3_translate (matrix, axis, 0.0);
      break;

    case GIMP_ORIENTATION_VERTICAL:
      gimp_matrix3_translate (matrix, 0.0, - axis);
      gimp_matrix3_scale (matrix, 1.0, -1.0);
      gimp_matrix3_translate (matrix, 0.0, axis);
      break;

    case GIMP_ORIENTATION_UNKNOWN:
      break;
    }
}

void
gimp_transform_matrix_flip_free (GimpMatrix3 *matrix,
                                 gdouble      x1,
                                 gdouble      y1,
                                 gdouble      x2,
                                 gdouble      y2)
{
  gdouble angle;

  g_return_if_fail (matrix != NULL);

  angle = atan2  (y2 - y1, x2 - x1);

  gimp_matrix3_identity  (matrix);
  gimp_matrix3_translate (matrix, -x1, -y1);
  gimp_matrix3_rotate    (matrix, -angle);
  gimp_matrix3_scale     (matrix, 1.0, -1.0);
  gimp_matrix3_rotate    (matrix, angle);
  gimp_matrix3_translate (matrix, x1, y1);
}

void
gimp_transform_matrix_rotate (GimpMatrix3         *matrix,
                              GimpRotationType     rotate_type,
                              gdouble              center_x,
                              gdouble              center_y)
{
  gdouble angle = 0;

  switch (rotate_type)
    {
    case GIMP_ROTATE_90:
      angle = G_PI_2;
      break;
    case GIMP_ROTATE_180:
      angle = G_PI;
      break;
    case GIMP_ROTATE_270:
      angle = - G_PI_2;
      break;
    }

  gimp_transform_matrix_rotate_center (matrix, center_x, center_y, angle);
}

void
gimp_transform_matrix_rotate_rect (GimpMatrix3 *matrix,
                                   gint         x,
                                   gint         y,
                                   gint         width,
                                   gint         height,
                                   gdouble      angle)
{
  gdouble center_x;
  gdouble center_y;

  g_return_if_fail (matrix != NULL);

  center_x = (gdouble) x + (gdouble) width  / 2.0;
  center_y = (gdouble) y + (gdouble) height / 2.0;

  gimp_matrix3_translate (matrix, -center_x, -center_y);
  gimp_matrix3_rotate    (matrix, angle);
  gimp_matrix3_translate (matrix, +center_x, +center_y);
}

void
gimp_transform_matrix_rotate_center (GimpMatrix3 *matrix,
                                     gdouble      center_x,
                                     gdouble      center_y,
                                     gdouble      angle)
{
  g_return_if_fail (matrix != NULL);

  gimp_matrix3_translate (matrix, -center_x, -center_y);
  gimp_matrix3_rotate    (matrix, angle);
  gimp_matrix3_translate (matrix, +center_x, +center_y);
}

void
gimp_transform_matrix_scale (GimpMatrix3 *matrix,
                             gint         x,
                             gint         y,
                             gint         width,
                             gint         height,
                             gdouble      t_x,
                             gdouble      t_y,
                             gdouble      t_width,
                             gdouble      t_height)
{
  gdouble scale_x = 1.0;
  gdouble scale_y = 1.0;

  g_return_if_fail (matrix != NULL);

  if (width > 0)
    scale_x = t_width / (gdouble) width;

  if (height > 0)
    scale_y = t_height / (gdouble) height;

  gimp_matrix3_identity  (matrix);
  gimp_matrix3_translate (matrix, -x, -y);
  gimp_matrix3_scale     (matrix, scale_x, scale_y);
  gimp_matrix3_translate (matrix, t_x, t_y);
}

void
gimp_transform_matrix_shear (GimpMatrix3         *matrix,
                             gint                 x,
                             gint                 y,
                             gint                 width,
                             gint                 height,
                             GimpOrientationType  orientation,
                             gdouble              amount)
{
  gdouble center_x;
  gdouble center_y;

  g_return_if_fail (matrix != NULL);

  if (width == 0)
    width = 1;

  if (height == 0)
    height = 1;

  center_x = (gdouble) x + (gdouble) width  / 2.0;
  center_y = (gdouble) y + (gdouble) height / 2.0;

  gimp_matrix3_identity  (matrix);
  gimp_matrix3_translate (matrix, -center_x, -center_y);

  if (orientation == GIMP_ORIENTATION_HORIZONTAL)
    gimp_matrix3_xshear (matrix, amount / height);
  else
    gimp_matrix3_yshear (matrix, amount / width);

  gimp_matrix3_translate (matrix, +center_x, +center_y);
}

void
gimp_transform_matrix_perspective (GimpMatrix3 *matrix,
                                   gint         x,
                                   gint         y,
                                   gint         width,
                                   gint         height,
                                   gdouble      t_x1,
                                   gdouble      t_y1,
                                   gdouble      t_x2,
                                   gdouble      t_y2,
                                   gdouble      t_x3,
                                   gdouble      t_y3,
                                   gdouble      t_x4,
                                   gdouble      t_y4)
{
  GimpMatrix3 trafo;
  gdouble     scalex;
  gdouble     scaley;

  g_return_if_fail (matrix != NULL);

  scalex = scaley = 1.0;

  if (width > 0)
    scalex = 1.0 / (gdouble) width;

  if (height > 0)
    scaley = 1.0 / (gdouble) height;

  gimp_matrix3_translate (matrix, -x, -y);
  gimp_matrix3_scale     (matrix, scalex, scaley);

  /* Determine the perspective transform that maps from
   * the unit cube to the transformed coordinates
   */
  {
    gdouble dx1, dx2, dx3, dy1, dy2, dy3;

    dx1 = t_x2 - t_x4;
    dx2 = t_x3 - t_x4;
    dx3 = t_x1 - t_x2 + t_x4 - t_x3;

    dy1 = t_y2 - t_y4;
    dy2 = t_y3 - t_y4;
    dy3 = t_y1 - t_y2 + t_y4 - t_y3;

    /*  Is the mapping affine?  */
    if ((dx3 == 0.0) && (dy3 == 0.0))
      {
        trafo.coeff[0][0] = t_x2 - t_x1;
        trafo.coeff[0][1] = t_x4 - t_x2;
        trafo.coeff[0][2] = t_x1;
        trafo.coeff[1][0] = t_y2 - t_y1;
        trafo.coeff[1][1] = t_y4 - t_y2;
        trafo.coeff[1][2] = t_y1;
        trafo.coeff[2][0] = 0.0;
        trafo.coeff[2][1] = 0.0;
      }
    else
      {
        gdouble det1, det2;

        det1 = dx3 * dy2 - dy3 * dx2;
        det2 = dx1 * dy2 - dy1 * dx2;

        trafo.coeff[2][0] = (det2 == 0.0) ? 1.0 : det1 / det2;

        det1 = dx1 * dy3 - dy1 * dx3;

        trafo.coeff[2][1] = (det2 == 0.0) ? 1.0 : det1 / det2;

        trafo.coeff[0][0] = t_x2 - t_x1 + trafo.coeff[2][0] * t_x2;
        trafo.coeff[0][1] = t_x3 - t_x1 + trafo.coeff[2][1] * t_x3;
        trafo.coeff[0][2] = t_x1;

        trafo.coeff[1][0] = t_y2 - t_y1 + trafo.coeff[2][0] * t_y2;
        trafo.coeff[1][1] = t_y3 - t_y1 + trafo.coeff[2][1] * t_y3;
        trafo.coeff[1][2] = t_y1;
      }

    trafo.coeff[2][2] = 1.0;
  }

  gimp_matrix3_mult (&trafo, matrix);
}

/* modified gaussian algorithm
 * solves a system of linear equations
 *
 * Example:
 * 1x + 2y + 4z = 25
 * 2x + 1y      = 4
 * 3x + 5y + 2z = 23
 * Solution: x=1, y=2, z=5
 *
 * Input:
 * matrix = { 1,2,4,25,2,1,0,4,3,5,2,23 }
 * s = 3 (Number of variables)
 * Output:
 * return value == TRUE (TRUE, if there is a single unique solution)
 * solution == { 1,2,5 } (if the return value is FALSE, the content
 * of solution is of no use)
 */
static gboolean
mod_gauss (gdouble matrix[],
           gdouble solution[],
           gint    s)
{
  gint    p[s]; /* row permutation */
  gint    i, j, r, temp;
  gdouble q;
  gint    t = s + 1;

  for (i = 0; i < s; i++)
    {
      p[i] = i;
    }

  for (r = 0; r < s; r++)
    {
      /* make sure that (r,r) is not 0 */
      if (fabs (matrix[p[r] * t + r]) <= EPSILON)
        {
          /* we need to permutate rows */
          for (i = r + 1; i <= s; i++)
            {
              if (i == s)
                {
                  /* if this happens, the linear system has zero or
                   * more than one solutions.
                   */
                  return FALSE;
                }

              if (fabs (matrix[p[i] * t + r]) > EPSILON)
                break;
            }

          temp = p[r];
          p[r] = p[i];
          p[i] = temp;
        }

      /* make (r,r) == 1 */
      q = 1.0 / matrix[p[r] * t + r];
      matrix[p[r] * t + r] = 1.0;

      for (j = r + 1; j < t; j++)
        {
          matrix[p[r] * t + j] *= q;
        }

      /* make that all entries in column r are 0 (except (r,r)) */
      for (i = 0; i < s; i++)
        {
          if (i == r)
            continue;

          for (j = r + 1; j < t ; j++)
            {
              matrix[p[i] * t + j] -= matrix[p[r] * t + j] * matrix[p[i] * t + r];
            }

          /* we don't need to execute the following line
           * since we won't access this element again:
           *
           * matrix[p[i] * t + r] = 0.0;
           */
        }
    }

  for (i = 0; i < s; i++)
    {
      solution[i] = matrix[p[i] * t + s];
    }

  return TRUE;
}

gboolean
gimp_transform_matrix_generic (GimpMatrix3       *matrix,
                               const GimpVector2 *input_points,
                               const GimpVector2 *output_points)
{
  GimpMatrix3 trafo;
  gdouble     coeff[8 * 9];
  gint        i;

  g_return_val_if_fail (matrix != NULL, FALSE);
  g_return_val_if_fail (input_points != NULL, FALSE);
  g_return_val_if_fail (output_points != NULL, FALSE);

  for (i = 0; i < 4; i++)
    {
      coeff[i * 9 + 0] = input_points[i].x;
      coeff[i * 9 + 1] = input_points[i].y;
      coeff[i * 9 + 2] = 1.0;
      coeff[i * 9 + 3] = 0.0;
      coeff[i * 9 + 4] = 0.0;
      coeff[i * 9 + 5] = 0.0;
      coeff[i * 9 + 6] = -input_points[i].x * output_points[i].x;
      coeff[i * 9 + 7] = -input_points[i].y * output_points[i].x;
      coeff[i * 9 + 8] =                      output_points[i].x;

      coeff[(i + 4) * 9 + 0] = 0.0;
      coeff[(i + 4) * 9 + 1] = 0.0;
      coeff[(i + 4) * 9 + 2] = 0.0;
      coeff[(i + 4) * 9 + 3] = input_points[i].x;
      coeff[(i + 4) * 9 + 4] = input_points[i].y;
      coeff[(i + 4) * 9 + 5] = 1.0;
      coeff[(i + 4) * 9 + 6] = -input_points[i].x * output_points[i].y;
      coeff[(i + 4) * 9 + 7] = -input_points[i].y * output_points[i].y;
      coeff[(i + 4) * 9 + 8] =                      output_points[i].y;
    }

  if (! mod_gauss (coeff, (gdouble *) trafo.coeff, 8))
    return FALSE;

  trafo.coeff[2][2] = 1.0;

  gimp_matrix3_mult (&trafo, matrix);

  for (i = 0; i < 4; i++)
    {
      gdouble w = matrix->coeff[2][0] * input_points[i].x +
                  matrix->coeff[2][1] * input_points[i].y +
                  matrix->coeff[2][2];

      if (w <= EPSILON)
        return FALSE;
    }

  return TRUE;
}

gboolean
gimp_transform_polygon_is_convex (gdouble x1,
                                  gdouble y1,
                                  gdouble x2,
                                  gdouble y2,
                                  gdouble x3,
                                  gdouble y3,
                                  gdouble x4,
                                  gdouble y4)
{
  gdouble z1, z2, z3, z4;

  /* We test if the transformed polygon is convex.  if z1 and z2 have
   * the same sign as well as z3 and z4 the polygon is convex.
   */
  z1 = ((x2 - x1) * (y4 - y1) -
        (x4 - x1) * (y2 - y1));
  z2 = ((x4 - x1) * (y3 - y1) -
        (x3 - x1) * (y4 - y1));
  z3 = ((x4 - x2) * (y3 - y2) -
        (x3 - x2) * (y4 - y2));
  z4 = ((x3 - x2) * (y1 - y2) -
        (x1 - x2) * (y3 - y2));

  return (z1 * z2 > 0) && (z3 * z4 > 0);
}

void
gimp_transform_polygon (const GimpMatrix3 *matrix,
                        const GimpVector2 *vertices,
                        gint               n_vertices,
                        gboolean           closed,
                        GimpVector2       *t_vertices,
                        gint              *n_t_vertices)
{
  GimpVector3 curr;
  gboolean    curr_visible;
  gint        i;

  g_return_if_fail (matrix != NULL);
  g_return_if_fail (vertices != NULL);
  g_return_if_fail (n_vertices >= 0);
  g_return_if_fail (t_vertices != NULL);
  g_return_if_fail (n_t_vertices != NULL);

  *n_t_vertices = 0;

  if (n_vertices == 0)
    return;

  curr.x = matrix->coeff[0][0] * vertices[0].x +
           matrix->coeff[0][1] * vertices[0].y +
           matrix->coeff[0][2];
  curr.y = matrix->coeff[1][0] * vertices[0].x +
           matrix->coeff[1][1] * vertices[0].y +
           matrix->coeff[1][2];
  curr.z = matrix->coeff[2][0] * vertices[0].x +
           matrix->coeff[2][1] * vertices[0].y +
           matrix->coeff[2][2];

  curr_visible = (curr.z >= NEAR_Z);

  for (i = 0; i < n_vertices; i++)
    {
      if (curr_visible)
        {
          t_vertices[(*n_t_vertices)++] = (GimpVector2) { curr.x / curr.z,
                                                          curr.y / curr.z };
        }

      if (i < n_vertices - 1 || closed)
        {
          GimpVector3 next;
          gboolean    next_visible;
          gint        j = (i + 1) % n_vertices;

          next.x = matrix->coeff[0][0] * vertices[j].x +
                   matrix->coeff[0][1] * vertices[j].y +
                   matrix->coeff[0][2];
          next.y = matrix->coeff[1][0] * vertices[j].x +
                   matrix->coeff[1][1] * vertices[j].y +
                   matrix->coeff[1][2];
          next.z = matrix->coeff[2][0] * vertices[j].x +
                   matrix->coeff[2][1] * vertices[j].y +
                   matrix->coeff[2][2];

          next_visible = (next.z >= NEAR_Z);

          if (next_visible != curr_visible)
            {
              gdouble ratio = (curr.z - NEAR_Z) / (curr.z - next.z);

              t_vertices[(*n_t_vertices)++] =
                (GimpVector2) { (curr.x + (next.x - curr.x) * ratio) / NEAR_Z,
                                (curr.y + (next.y - curr.y) * ratio) / NEAR_Z };
            }

          curr         = next;
          curr_visible = next_visible;
        }
    }
}
