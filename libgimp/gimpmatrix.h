/* gimpmatrix.h
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

#ifndef __GIMPMATRIX_H__
#define __GIMPMATRIX_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef gdouble GimpMatrix[3][3];

void          gimp_matrix_transform_point (GimpMatrix  matrix, 
					   gdouble     x, 
					   gdouble     y,
					   gdouble    *newx, 
					   gdouble    *newy);
void          gimp_matrix_mult            (GimpMatrix  matrix1, 
					   GimpMatrix  matrix2);
void          gimp_matrix_identity        (GimpMatrix  matrix);
void          gimp_matrix_translate       (GimpMatrix  matrix, 
					   gdouble     x, 
					   gdouble     y);
void          gimp_matrix_scale           (GimpMatrix  matrix, 
					   gdouble     x, 
					   gdouble     y);
void          gimp_matrix_rotate          (GimpMatrix  matrix, 
					   gdouble     theta);
void          gimp_matrix_xshear          (GimpMatrix  matrix, 
					   gdouble     amount);
void          gimp_matrix_yshear          (GimpMatrix  matrix, 
					   gdouble     amount);
gdouble       gimp_matrix_determinant     (GimpMatrix  matrix);
void          gimp_matrix_invert          (GimpMatrix  matrix, 
					   GimpMatrix  matrix_inv);
void          gimp_matrix_duplicate       (GimpMatrix  src, 
					   GimpMatrix  target);

gboolean      gimp_matrix_is_diagonal     (GimpMatrix  matrix);
gboolean      gimp_matrix_is_identity     (GimpMatrix  matrix);
gboolean      gimp_matrix_is_simple       (GimpMatrix  matrix);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /*  __GIMPMATRIX_H__  */
