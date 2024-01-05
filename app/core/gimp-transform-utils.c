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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimp-transform-utils.h"
#include "gimpcoords.h"
#include "gimpcoords-interpolate.h"


#define EPSILON 1e-6


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
    case GIMP_ROTATE_DEGREES90:
      angle = G_PI_2;
      break;
    case GIMP_ROTATE_DEGREES180:
      angle = G_PI;
      break;
    case GIMP_ROTATE_DEGREES270:
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

/* multiplies 'matrix' by the matrix that transforms a set of 4 'input_points'
 * to corresponding 'output_points', if such matrix exists, and is valid (i.e.,
 * keeps the output points in front of the camera).
 *
 * returns TRUE if successful.
 */
gboolean
gimp_transform_matrix_generic (GimpMatrix3       *matrix,
                               const GimpVector2  input_points[4],
                               const GimpVector2  output_points[4])
{
  GimpMatrix3 trafo;
  gdouble     coeff[8 * 9];
  gboolean    negative = -1;
  gint        i;
  gboolean    result   = TRUE;

  g_return_val_if_fail (matrix != NULL, FALSE);
  g_return_val_if_fail (input_points != NULL, FALSE);
  g_return_val_if_fail (output_points != NULL, FALSE);

  /* find the matrix that transforms 'input_points' to 'output_points', whose
   * (3, 3) coefficient is 1, by solving a system of linear equations whose
   * solution is the remaining 8 coefficients.
   */
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

  /* if there is no solution, bail */
  if (! mod_gauss (coeff, (gdouble *) trafo.coeff, 8))
    return FALSE;

  trafo.coeff[2][2] = 1.0;

  /* make sure that none of the input points maps to a point at infinity, and
   * that all output points are on the same side of the camera.
   */
  for (i = 0; i < 4; i++)
    {
      gdouble  w;
      gboolean neg;

      w = trafo.coeff[2][0] * input_points[i].x +
          trafo.coeff[2][1] * input_points[i].y +
          trafo.coeff[2][2];

      if (fabs (w) <= EPSILON)
        result = FALSE;

      neg = (w < 0.0);

      if (negative < 0)
        {
          negative = neg;
        }
      else if (neg != negative)
        {
          result = FALSE;
          break;
        }
    }

  /* if the output points are all behind the camera, negate the matrix, which
   * would map the input points to the corresponding points in front of the
   * camera.
   */
  if (negative > 0)
    {
      gint r;
      gint c;

      for (r = 0; r < 3; r++)
        {
          for (c = 0; c < 3; c++)
            {
              trafo.coeff[r][c] = -trafo.coeff[r][c];
            }
        }
    }

  /* append the transformation to 'matrix' */
  gimp_matrix3_mult (&trafo, matrix);

  return result;
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

/* transforms the polygon or polyline, whose vertices are given by 'vertices',
 * by 'matrix', performing clipping by the near plane.  'closed' indicates
 * whether the vertices represent a polygon ('closed == TRUE') or a polyline
 * ('closed == FALSE').
 *
 * returns the transformed vertices in 't_vertices', and their count in
 * 'n_t_vertices'.  the minimal possible number of transformed vertices is 0,
 * which happens when the entire input is clipped.  in general, the maximal
 * possible number of transformed vertices is '3 * n_vertices / 2' (rounded
 * down), however, for convex polygons the number is 'n_vertices + 1', and for
 * a single line segment ('n_vertices == 2' and 'closed == FALSE') the number
 * is 2.
 *
 * 't_vertices' may not alias 'vertices', except when transforming a single
 * line segment.
 */
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

  curr_visible = (curr.z >= GIMP_TRANSFORM_NEAR_Z);

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

          next_visible = (next.z >= GIMP_TRANSFORM_NEAR_Z);

          if (next_visible != curr_visible)
            {
              gdouble ratio = (curr.z - GIMP_TRANSFORM_NEAR_Z) / (curr.z - next.z);

              t_vertices[(*n_t_vertices)++] =
                (GimpVector2) { (curr.x + (next.x - curr.x) * ratio) / GIMP_TRANSFORM_NEAR_Z,
                                (curr.y + (next.y - curr.y) * ratio) / GIMP_TRANSFORM_NEAR_Z };
            }

          curr         = next;
          curr_visible = next_visible;
        }
    }
}

/* same as gimp_transform_polygon(), but using GimpCoords as the vertex type,
 * instead of GimpVector2.
 */
void
gimp_transform_polygon_coords (const GimpMatrix3 *matrix,
                               const GimpCoords  *vertices,
                               gint               n_vertices,
                               gboolean           closed,
                               GimpCoords        *t_vertices,
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

  curr_visible = (curr.z >= GIMP_TRANSFORM_NEAR_Z);

  for (i = 0; i < n_vertices; i++)
    {
      if (curr_visible)
        {
          t_vertices[*n_t_vertices]   = vertices[i];
          t_vertices[*n_t_vertices].x = curr.x / curr.z;
          t_vertices[*n_t_vertices].y = curr.y / curr.z;

          (*n_t_vertices)++;
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

          next_visible = (next.z >= GIMP_TRANSFORM_NEAR_Z);

          if (next_visible != curr_visible)
            {
              gdouble ratio = (curr.z - GIMP_TRANSFORM_NEAR_Z) / (curr.z - next.z);

              gimp_coords_mix (1.0 - ratio, &vertices[i],
                                     ratio, &vertices[j],
                                            &t_vertices[*n_t_vertices]);

              t_vertices[*n_t_vertices].x = (curr.x + (next.x - curr.x) * ratio) /
                                             GIMP_TRANSFORM_NEAR_Z;
              t_vertices[*n_t_vertices].y = (curr.y + (next.y - curr.y) * ratio) /
                                             GIMP_TRANSFORM_NEAR_Z;

              (*n_t_vertices)++;
            }

          curr         = next;
          curr_visible = next_visible;
        }
    }
}

/* returns the value of the polynomial 'poly', of degree 'degree', at 'x'.  the
 * coefficients of 'poly' should be specified in descending-degree order.
 */
static gdouble
polynomial_eval (const gdouble *poly,
                 gint           degree,
                 gdouble        x)
{
  gdouble y = poly[0];
  gint    i;

  for (i = 1; i <= degree; i++)
    y = y * x + poly[i];

  return y;
}

/* derives the polynomial 'poly', of degree 'degree'.
 *
 * returns the derivative in 'result'.
 */
static void
polynomial_derive (const gdouble *poly,
                   gint           degree,
                   gdouble       *result)
{
  while (degree)
    *result++ = *poly++ * degree--;
}

/* finds the real odd-multiplicity root of the polynomial 'poly', of degree
 * 'degree', inside the range '(x1, x2)'.
 *
 * returns TRUE if such a root exists, and stores its value in '*root'.
 *
 * 'poly' shall be monotonic in the range '(x1, x2)'.
 */
static gboolean
polynomial_odd_root (const gdouble *poly,
                     gint           degree,
                     gdouble        x1,
                     gdouble        x2,
                     gdouble       *root)
{
  gdouble y1;
  gdouble y2;
  gint    i;

  y1 = polynomial_eval (poly, degree, x1);
  y2 = polynomial_eval (poly, degree, x2);

  if (y1 * y2 > -EPSILON)
    {
      /* the two endpoints have the same sign, or one of them is zero.  there's
       * no root inside the range.
       */
      return FALSE;
    }
  else if (y1 > 0.0)
    {
      gdouble t;

      /* if the first endpoint is positive, swap the endpoints, so that the
       * first endpoint is always negative, and the second endpoint is always
       * positive.
       */

      t  = x1;
      x1 = x2;
      x2 = t;
    }

  /* approximate the root using binary search */
  for (i = 0; i < 53; i++)
    {
      gdouble x = (x1 + x2) / 2.0;
      gdouble y = polynomial_eval (poly, degree, x);

      if (y > 0.0)
        x2 = x;
      else
        x1 = x;
    }

  *root = (x1 + x2) / 2.0;

  return TRUE;
}

/* finds the real odd-multiplicity roots of the polynomial 'poly', of degree
 * 'degree', inside the range '(x1, x2)'.
 *
 * returns the roots in 'roots', in ascending order, and their count in
 * 'n_roots'.
 */
static void
polynomial_odd_roots (const gdouble *poly,
                      gint           degree,
                      gdouble        x1,
                      gdouble        x2,
                      gdouble       *roots,
                      gint          *n_roots)
{
  *n_roots = 0;

  /* find the real degree of the polynomial (skip any leading coefficients that
   * are 0)
   */
  for (; degree && fabs (*poly) < EPSILON; poly++, degree--);

  #define ADD_ROOT(root) \
    do \
      { \
        gdouble r = (root); \
        \
        if (r > x1 && r < x2) \
          roots[(*n_roots)++] = r; \
      } \
    while (FALSE)

  switch (degree)
    {
    /* constant case */
    case 0:
      break;

    /* linear case */
    case 1:
      ADD_ROOT (-poly[1] / poly[0]);
      break;

    /* quadratic case */
    case 2:
      {
        gdouble s = SQR (poly[1]) - 4 * poly[0] * poly[2];

        if (s > EPSILON)
          {
            s = sqrt (s);

            if (poly[0] < 0.0)
              s = -s;

            ADD_ROOT ((-poly[1] - s) / (2.0 * poly[0]));
            ADD_ROOT ((-poly[1] + s) / (2.0 * poly[0]));
          }

        break;
      }

    /* general case */
    default:
      {
        gdouble deriv[degree];
        gdouble deriv_roots[degree - 1];
        gint    n_deriv_roots;
        gdouble a;
        gdouble b;
        gint    i;

        /* find the odd roots of the derivative, i.e., the local extrema of the
         * polynomial
         */
        polynomial_derive (poly, degree, deriv);
        polynomial_odd_roots (deriv, degree - 1, x1, x2,
                              deriv_roots, &n_deriv_roots);

        /* search for roots between each consecutive pair of extrema, including
         * the endpoints
         */
        a = x1;

        for (i = 0; i <= n_deriv_roots; i++)
          {
            if (i < n_deriv_roots)
              b = deriv_roots[i];
            else
              b = x2;

            *n_roots += polynomial_odd_root (poly, degree, a, b,
                                             &roots[*n_roots]);

            a = b;
          }

        break;
      }
    }

  #undef ADD_ROOT
}

/* clips the cubic bezier segment, defined by the four control points 'bezier',
 * to the halfplane 'ax + by + c >= 0'.
 *
 * returns the clipped set of bezier segments in 'c_bezier', and their count in
 * 'n_c_bezier'.  the minimal possible number of clipped segments is 0, which
 * happens when the entire segment is clipped.  the maximal possible number of
 * clipped segments is 2.
 *
 * if the first clipped segment is an initial segment of 'bezier', sets
 * '*start_in' to TRUE, otherwise to FALSE.  if the last clipped segment is a
 * final segment of 'bezier', sets '*end_in' to TRUE, otherwise to FALSE.
 *
 * 'c_bezier' may not alias 'bezier'.
 */
static void
clip_bezier (const GimpCoords  bezier[4],
             gdouble           a,
             gdouble           b,
             gdouble           c,
             GimpCoords        c_bezier[2][4],
             gint             *n_c_bezier,
             gboolean         *start_in,
             gboolean         *end_in)
{
  gdouble dot[4];
  gdouble poly[4];
  gdouble roots[5];
  gint    n_roots;
  gint    n_positive;
  gint    i;

  n_positive = 0;

  for (i = 0; i < 4; i++)
    {
      dot[i] = a * bezier[i].x + b * bezier[i].y + c;

      n_positive += (dot[i] >= 0.0);
    }

  if (n_positive == 0)
    {
      /* all points are out -- the entire segment is out */

      *n_c_bezier = 0;
      *start_in   = FALSE;
      *end_in     = FALSE;

      return;
    }
  else if (n_positive == 4)
    {
      /* all points are in -- the entire segment is in */

      memcpy (c_bezier[0], bezier, sizeof (GimpCoords[4]));

      *n_c_bezier = 1;
      *start_in   = TRUE;
      *end_in     = TRUE;

      return;
    }

  /* find the points of intersection of the segment with the 'ax + by + c = 0'
   * line
   */
  poly[0] = dot[3] - 3.0 * dot[2] + 3.0 * dot[1] - dot[0];
  poly[1] = 3.0 * (dot[2] - 2.0 * dot[1] + dot[0]);
  poly[2] = 3.0 * (dot[1] - dot[0]);
  poly[3] = dot[0];

  roots[0] = 0.0;
  polynomial_odd_roots (poly, 3, 0.0, 1.0, roots + 1, &n_roots);
  roots[++n_roots] = 1.0;

  /* construct the list of segments that are inside the halfplane */
  *n_c_bezier = 0;
  *start_in   = (polynomial_eval (poly, 3, roots[1] / 2.0) > 0.0);
  *end_in     = (*start_in + n_roots + 1) % 2;

  for (i = ! *start_in; i < n_roots; i += 2)
    {
      gdouble t0 = roots[i];
      gdouble t1 = roots[i + 1];

      gimp_coords_interpolate_bezier_at (bezier, t0,
                                         &c_bezier[*n_c_bezier][0],
                                         &c_bezier[*n_c_bezier][1]);
      gimp_coords_interpolate_bezier_at (bezier, t1,
                                         &c_bezier[*n_c_bezier][3],
                                         &c_bezier[*n_c_bezier][2]);

      gimp_coords_mix (1.0,             &c_bezier[*n_c_bezier][0],
                       (t1 - t0) / 3.0, &c_bezier[*n_c_bezier][1],
                       &c_bezier[*n_c_bezier][1]);
      gimp_coords_mix (1.0,             &c_bezier[*n_c_bezier][3],
                       (t0 - t1) / 3.0, &c_bezier[*n_c_bezier][2],
                       &c_bezier[*n_c_bezier][2]);

      (*n_c_bezier)++;
    }
}

/* transforms the cubic bezier segment, defined by the four control points
 * 'bezier', by 'matrix', subdividing it as necessary to avoid diverging too
 * much from the real transformed curve.  at most 'depth' subdivisions are
 * performed.
 *
 * appends the transformed sequence of bezier segments to 't_beziers'.
 *
 * 'bezier' shall be fully clipped to the near plane.
 */
static void
transform_bezier_coords (const GimpMatrix3 *matrix,
                         const GimpCoords   bezier[4],
                         GQueue            *t_beziers,
                         gint               depth)
{
  GimpCoords *t_bezier;
  gint        n;

  /* check if we need to split the segment */
  if (depth > 0)
    {
      GimpVector2 v[4];
      GimpVector2 c[2];
      GimpVector2 b;
      gint        i;

      for (i = 0; i < 4; i++)
        v[i] = (GimpVector2) { bezier[i].x, bezier[i].y };

      gimp_vector2_sub (&c[0], &v[1], &v[0]);
      gimp_vector2_sub (&c[1], &v[2], &v[3]);

      gimp_vector2_sub (&b, &v[3], &v[0]);
      gimp_vector2_mul (&b, 1.0 / gimp_vector2_inner_product (&b, &b));

      for (i = 0; i < 2; i++)
        {
          /* split the segment if one of the control points is too far from the
           * line connecting the anchors
           */
          if (fabs (gimp_vector2_cross_product (&c[i], &b).x) > 0.5)
            {
              GimpCoords mid_position;
              GimpCoords mid_velocity;
              GimpCoords sub[4];

              gimp_coords_interpolate_bezier_at (bezier, 0.5,
                                                 &mid_position, &mid_velocity);

              /* first half */
              sub[0] = bezier[0];
              sub[3] = mid_position;

              gimp_coords_mix (0.5,        &sub[0],
                               0.5,        &bezier[1],
                                           &sub[1]);
              gimp_coords_mix (1.0,        &sub[3],
                               -1.0 / 6.0, &mid_velocity,
                                           &sub[2]);

              transform_bezier_coords (matrix, sub, t_beziers, depth - 1);

              /* second half */
              sub[0] = mid_position;
              sub[3] = bezier[3];

              gimp_coords_mix (1.0,        &sub[0],
                               +1.0 / 6.0, &mid_velocity,
                                           &sub[1]);
              gimp_coords_mix (0.5,        &sub[3],
                               0.5,        &bezier[2],
                                           &sub[2]);

              transform_bezier_coords (matrix, sub, t_beziers, depth - 1);

              return;
            }
        }
    }

  /* transform the segment by transforming each of the individual points.  note
   * that, for non-affine transforms, this is only an approximation of the real
   * transformed curve, but due to subdivision it should be good enough.
   */
  t_bezier = g_new (GimpCoords, 4);

  /* note that while the segments themselves are clipped to the near plane,
   * their control points may still get transformed behind the camera.  we
   * therefore clip the control points to the near plane as well, which is not
   * too meaningful, but avoids erroneously transforming them behind the
   * camera.
   */
  gimp_transform_polygon_coords (matrix, bezier, 2, FALSE,
                                 t_bezier, &n);
  gimp_transform_polygon_coords (matrix, bezier + 2, 2, FALSE,
                                 t_bezier + 2, &n);

  g_queue_push_tail (t_beziers, t_bezier);
}

/* transforms the cubic bezier segment, defined by the four control points
 * 'bezier', by 'matrix', performing clipping by the near plane and subdividing
 * as necessary.
 *
 * returns the transformed set of bezier-segment sequences in 't_beziers', as
 * GQueues of GimpCoords[4] bezier-segments, and the number of sequences in
 * 'n_t_beziers'.  the minimal possible number of transformed sequences is 0,
 * which happens when the entire segment is clipped.  the maximal possible
 * number of transformed sequences is 2.  each sequence has at least one
 * segment.
 *
 * if the first transformed segment is an initial segment of 'bezier', sets
 * '*start_in' to TRUE, otherwise to FALSE.  if the last transformed segment is
 * a final segment of 'bezier', sets '*end_in' to TRUE, otherwise to FALSE.
 */
void
gimp_transform_bezier_coords (const GimpMatrix3 *matrix,
                              const GimpCoords   bezier[4],
                              GQueue            *t_beziers[2],
                              gint              *n_t_beziers,
                              gboolean          *start_in,
                              gboolean          *end_in)
{
  GimpCoords c_bezier[2][4];
  gint       i;

  g_return_if_fail (matrix != NULL);
  g_return_if_fail (bezier != NULL);
  g_return_if_fail (t_beziers != NULL);
  g_return_if_fail (n_t_beziers != NULL);
  g_return_if_fail (start_in != NULL);
  g_return_if_fail (end_in != NULL);

  /* if the matrix is affine, transform the easy way */
  if (gimp_matrix3_is_affine (matrix))
    {
      GimpCoords *t_bezier;

      t_beziers[0] = g_queue_new ();
      *n_t_beziers = 1;

      t_bezier = g_new (GimpCoords, 1);
      g_queue_push_tail (t_beziers[0], t_bezier);

      for (i = 0; i < 4; i++)
        {
          t_bezier[i] = bezier[i];

          gimp_matrix3_transform_point (matrix,
                                        bezier[i].x,    bezier[i].y,
                                        &t_bezier[i].x, &t_bezier[i].y);
        }

      return;
    }

  /* clip the segment to the near plane */
  clip_bezier (bezier,
               matrix->coeff[2][0],
               matrix->coeff[2][1],
               matrix->coeff[2][2] - GIMP_TRANSFORM_NEAR_Z,
               c_bezier, n_t_beziers,
               start_in, end_in);

  /* transform each of the resulting segments */
  for (i = 0; i < *n_t_beziers; i++)
    {
      t_beziers[i] = g_queue_new ();

      transform_bezier_coords (matrix, c_bezier[i], t_beziers[i], 3);
    }
}
