/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmatrix.h
 * Copyright (C) 1998 Jay Cox <jaycox@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_MATH_H_INSIDE__) && !defined (GIMP_MATH_COMPILATION)
#error "Only <libgimpmath/gimpmath.h> can be included directly."
#endif

#ifndef __GIMP_MATRIX_H__
#define __GIMP_MATRIX_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*****************/
/*  GimpMatrix2  */
/*****************/

#define GIMP_TYPE_MATRIX2               (gimp_matrix2_get_type ())
#define GIMP_VALUE_HOLDS_MATRIX2(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_MATRIX2))

GType         gimp_matrix2_get_type        (void) G_GNUC_CONST;


#define GIMP_TYPE_PARAM_MATRIX2            (gimp_param_matrix2_get_type ())
#define GIMP_IS_PARAM_SPEC_MATRIX2(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_MATRIX2))

GType         gimp_param_matrix2_get_type  (void) G_GNUC_CONST;

GParamSpec *  gimp_param_spec_matrix2      (const gchar        *name,
                                            const gchar        *nick,
                                            const gchar        *blurb,
                                            const GimpMatrix2  *default_value,
                                            GParamFlags         flags);


void          gimp_matrix2_identity        (GimpMatrix2       *matrix);
void          gimp_matrix2_mult            (const GimpMatrix2 *matrix1,
                                            GimpMatrix2       *matrix2);

gdouble       gimp_matrix2_determinant     (const GimpMatrix2 *matrix);
void          gimp_matrix2_invert          (GimpMatrix2       *matrix);

void          gimp_matrix2_transform_point (const GimpMatrix2 *matrix,
                                            gdouble            x,
                                            gdouble            y,
                                            gdouble           *newx,
                                            gdouble           *newy);


/*****************/
/*  GimpMatrix3  */
/*****************/

#define GIMP_TYPE_MATRIX3               (gimp_matrix3_get_type ())
#define GIMP_VALUE_HOLDS_MATRIX3(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_MATRIX3))

GType         gimp_matrix3_get_type        (void) G_GNUC_CONST;


#define GIMP_TYPE_PARAM_MATRIX3            (gimp_param_matrix3_get_type ())
#define GIMP_IS_PARAM_SPEC_MATRIX3(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_MATRIX3))

GType         gimp_param_matrix3_get_type  (void) G_GNUC_CONST;

GParamSpec *  gimp_param_spec_matrix3      (const gchar        *name,
                                            const gchar        *nick,
                                            const gchar        *blurb,
                                            const GimpMatrix3  *default_value,
                                            GParamFlags         flags);


void          gimp_matrix3_identity        (GimpMatrix3       *matrix);
void          gimp_matrix3_mult            (const GimpMatrix3 *matrix1,
                                            GimpMatrix3       *matrix2);
void          gimp_matrix3_translate       (GimpMatrix3       *matrix,
                                            gdouble            x,
                                            gdouble            y);
void          gimp_matrix3_scale           (GimpMatrix3       *matrix,
                                            gdouble            x,
                                            gdouble            y);
void          gimp_matrix3_rotate          (GimpMatrix3       *matrix,
                                            gdouble            theta);
void          gimp_matrix3_xshear          (GimpMatrix3       *matrix,
                                            gdouble            amount);
void          gimp_matrix3_yshear          (GimpMatrix3       *matrix,
                                            gdouble            amount);
void          gimp_matrix3_affine          (GimpMatrix3       *matrix,
                                            gdouble            a,
                                            gdouble            b,
                                            gdouble            c,
                                            gdouble            d,
                                            gdouble            e,
                                            gdouble            f);

gdouble       gimp_matrix3_determinant     (const GimpMatrix3 *matrix);
void          gimp_matrix3_invert          (GimpMatrix3       *matrix);

gboolean      gimp_matrix3_is_identity     (const GimpMatrix3 *matrix);
gboolean      gimp_matrix3_is_diagonal     (const GimpMatrix3 *matrix);
gboolean      gimp_matrix3_is_affine       (const GimpMatrix3 *matrix);
gboolean      gimp_matrix3_is_simple       (const GimpMatrix3 *matrix);

gboolean      gimp_matrix3_equal           (const GimpMatrix3 *matrix1,
                                            const GimpMatrix3 *matrix2);

void          gimp_matrix3_transform_point (const GimpMatrix3 *matrix,
                                            gdouble            x,
                                            gdouble            y,
                                            gdouble           *newx,
                                            gdouble           *newy);


/*****************/
/*  GimpMatrix4  */
/*****************/

void          gimp_matrix4_identity        (GimpMatrix4       *matrix);
void          gimp_matrix4_mult            (const GimpMatrix4 *matrix1,
                                            GimpMatrix4       *matrix2);

void          gimp_matrix4_to_deg          (const GimpMatrix4 *matrix,
                                            gdouble           *a,
                                            gdouble           *b,
                                            gdouble           *c);

gdouble       gimp_matrix4_transform_point (const GimpMatrix4 *matrix,
                                            gdouble            x,
                                            gdouble            y,
                                            gdouble            z,
                                            gdouble           *newx,
                                            gdouble           *newy,
                                            gdouble           *newz);


G_END_DECLS

#endif /* __GIMP_MATRIX_H__ */
