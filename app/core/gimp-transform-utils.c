/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimp-transform-utils.h"


void
gimp_transform_matrix_flip (GimpOrientationType  flip_type,
                            gdouble              axis,
                            GimpMatrix3         *result)
{
  gimp_matrix3_identity (result);

  switch (flip_type)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      gimp_matrix3_translate (result, - axis, 0.0);
      gimp_matrix3_scale (result, -1.0, 1.0);
      gimp_matrix3_translate (result, axis, 0.0);
      break;

    case GIMP_ORIENTATION_VERTICAL:
      gimp_matrix3_translate (result, 0.0, - axis);
      gimp_matrix3_scale (result, 1.0, -1.0);
      gimp_matrix3_translate (result, 0.0, axis);
      break;

    case GIMP_ORIENTATION_UNKNOWN:
      break;
    }
}

void
gimp_transform_matrix_rotate (gint         x1,
                              gint         y1,
                              gint         x2,
                              gint         y2,
                              gdouble      angle,
                              GimpMatrix3 *result)
{
  gdouble cx;
  gdouble cy;

  cx = (gdouble) (x1 + x2) / 2.0;
  cy = (gdouble) (y1 + y2) / 2.0;

  gimp_matrix3_identity  (result);
  gimp_matrix3_translate (result, -cx, -cy);
  gimp_matrix3_rotate    (result, angle);
  gimp_matrix3_translate (result, +cx, +cy);
}

void
gimp_transform_matrix_rotate_center (gdouble      cx,
                                     gdouble      cy,
                                     gdouble      angle,
                                     GimpMatrix3 *result)
{
  gimp_matrix3_identity  (result);
  gimp_matrix3_translate (result, -cx, -cy);
  gimp_matrix3_rotate    (result, angle);
  gimp_matrix3_translate (result, +cx, +cy);
}

void
gimp_transform_matrix_scale (gint         x1,
                             gint         y1,
                             gint         x2,
                             gint         y2,
                             gdouble      tx1,
                             gdouble      ty1,
                             gdouble      tx2,
                             gdouble      ty2,
                             GimpMatrix3 *result)
{
  gdouble scalex;
  gdouble scaley;

  scalex = scaley = 1.0;

  if ((x2 - x1) > 0)
    scalex = (tx2 - tx1) / (gdouble) (x2 - x1);

  if ((y2 - y1) > 0)
    scaley = (ty2 - ty1) / (gdouble) (y2 - y1);

  gimp_matrix3_identity  (result);
  gimp_matrix3_translate (result, -x1, -y1);
  gimp_matrix3_scale     (result, scalex, scaley);
  gimp_matrix3_translate (result, tx1, ty1);
}

void
gimp_transform_matrix_shear (gint                 x1,
                             gint                 y1,
                             gint                 x2,
                             gint                 y2,
                             GimpOrientationType  orientation,
                             gdouble              amount,
                             GimpMatrix3         *result)
{
  gint    width;
  gint    height;
  gdouble cx;
  gdouble cy;

  width  = x2 - x1;
  height = y2 - y1;

  if (width == 0)
    width = 1;
  if (height == 0)
    height = 1;

  cx = (gdouble) (x1 + x2) / 2.0;
  cy = (gdouble) (y1 + y2) / 2.0;

  gimp_matrix3_identity  (result);
  gimp_matrix3_translate (result, -cx, -cy);

  if (orientation == GIMP_ORIENTATION_HORIZONTAL)
    gimp_matrix3_xshear (result, amount / height);
  else
    gimp_matrix3_yshear (result, amount / width);

  gimp_matrix3_translate (result, +cx, +cy);
}

void
gimp_transform_matrix_perspective (gint         x1,
                                   gint         y1,
                                   gint         x2,
                                   gint         y2,
                                   gdouble      tx1,
                                   gdouble      ty1,
                                   gdouble      tx2,
                                   gdouble      ty2,
                                   gdouble      tx3,
                                   gdouble      ty3,
                                   gdouble      tx4,
                                   gdouble      ty4,
                                   GimpMatrix3 *result)
{
  GimpMatrix3 matrix;
  gdouble     scalex;
  gdouble     scaley;

  scalex = scaley = 1.0;

  if ((x2 - x1) > 0)
    scalex = 1.0 / (gdouble) (x2 - x1);

  if ((y2 - y1) > 0)
    scaley = 1.0 / (gdouble) (y2 - y1);

  /* Determine the perspective transform that maps from
   * the unit cube to the transformed coordinates
   */
  {
    gdouble dx1, dx2, dx3, dy1, dy2, dy3;

    dx1 = tx2 - tx4;
    dx2 = tx3 - tx4;
    dx3 = tx1 - tx2 + tx4 - tx3;

    dy1 = ty2 - ty4;
    dy2 = ty3 - ty4;
    dy3 = ty1 - ty2 + ty4 - ty3;

    /*  Is the mapping affine?  */
    if ((dx3 == 0.0) && (dy3 == 0.0))
      {
        matrix.coeff[0][0] = tx2 - tx1;
        matrix.coeff[0][1] = tx4 - tx2;
        matrix.coeff[0][2] = tx1;
        matrix.coeff[1][0] = ty2 - ty1;
        matrix.coeff[1][1] = ty4 - ty2;
        matrix.coeff[1][2] = ty1;
        matrix.coeff[2][0] = 0.0;
        matrix.coeff[2][1] = 0.0;
      }
    else
      {
        gdouble det1, det2;

        det1 = dx3 * dy2 - dy3 * dx2;
        det2 = dx1 * dy2 - dy1 * dx2;

        if (det1 == 0.0 && det2 == 0.0)
          matrix.coeff[2][0] = 1.0;
        else
          matrix.coeff[2][0] = det1 / det2;

        det1 = dx1 * dy3 - dy1 * dx3;

        if (det1 == 0.0 && det2 == 0.0)
          matrix.coeff[2][1] = 1.0;
        else
          matrix.coeff[2][1] = det1 / det2;

        matrix.coeff[0][0] = tx2 - tx1 + matrix.coeff[2][0] * tx2;
        matrix.coeff[0][1] = tx3 - tx1 + matrix.coeff[2][1] * tx3;
        matrix.coeff[0][2] = tx1;

        matrix.coeff[1][0] = ty2 - ty1 + matrix.coeff[2][0] * ty2;
        matrix.coeff[1][1] = ty3 - ty1 + matrix.coeff[2][1] * ty3;
        matrix.coeff[1][2] = ty1;
      }

    matrix.coeff[2][2] = 1.0;
  }

  gimp_matrix3_identity  (result);
  gimp_matrix3_translate (result, -x1, -y1);
  gimp_matrix3_scale     (result, scalex, scaley);
  gimp_matrix3_mult      (&matrix, result);
}
