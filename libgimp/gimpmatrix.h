/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmatrix.h
 * Copyright (C) 1998 Jay Cox <jaycox@earthlink.net>
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

#ifndef __GIMP_MATRIX_H__
#define __GIMP_MATRIX_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef gdouble GimpMatrix3[3][3];
typedef gdouble GimpMatrix4[4][4];

void          gimp_matrix3_transform_point (GimpMatrix3  matrix, 
					    gdouble      x, 
					    gdouble      y,
					    gdouble     *newx, 
					    gdouble     *newy);
void          gimp_matrix3_mult            (GimpMatrix3  matrix1, 
					    GimpMatrix3  matrix2);
void          gimp_matrix3_identity        (GimpMatrix3  matrix);
void          gimp_matrix3_translate       (GimpMatrix3  matrix, 
					    gdouble      x, 
					    gdouble      y);
void          gimp_matrix3_scale           (GimpMatrix3  matrix, 
					    gdouble      x, 
					    gdouble      y);
void          gimp_matrix3_rotate          (GimpMatrix3  matrix, 
					    gdouble      theta);
void          gimp_matrix3_xshear          (GimpMatrix3  matrix, 
					    gdouble      amount);
void          gimp_matrix3_yshear          (GimpMatrix3  matrix, 
					    gdouble      amount);
gdouble       gimp_matrix3_determinant     (GimpMatrix3  matrix);
void          gimp_matrix3_invert          (GimpMatrix3  matrix, 
					    GimpMatrix3  matrix_inv);
void          gimp_matrix3_duplicate       (GimpMatrix3  src, 
					    GimpMatrix3  target);

gboolean      gimp_matrix3_is_diagonal     (GimpMatrix3  matrix);
gboolean      gimp_matrix3_is_identity     (GimpMatrix3  matrix);
gboolean      gimp_matrix3_is_simple       (GimpMatrix3  matrix);


void          gimp_matrix4_to_deg          (GimpMatrix4  matrix,
					    gdouble     *a,
					    gdouble     *b,
					    gdouble     *c);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /*  __GIMP_MATRIX_H__  */
