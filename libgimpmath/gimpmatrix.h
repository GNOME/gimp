/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmamatrix.h
 * Copyright (C) 1998 Jay Cox <jaycox@ligma.org>
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

#if !defined (__LIGMA_MATH_H_INSIDE__) && !defined (LIGMA_MATH_COMPILATION)
#error "Only <libligmamath/ligmamath.h> can be included directly."
#endif

#ifndef __LIGMA_MATRIX_H__
#define __LIGMA_MATRIX_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*****************/
/*  LigmaMatrix2  */
/*****************/

#define LIGMA_TYPE_MATRIX2               (ligma_matrix2_get_type ())
#define LIGMA_VALUE_HOLDS_MATRIX2(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_MATRIX2))

GType         ligma_matrix2_get_type        (void) G_GNUC_CONST;


#define LIGMA_TYPE_PARAM_MATRIX2            (ligma_param_matrix2_get_type ())
#define LIGMA_IS_PARAM_SPEC_MATRIX2(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_MATRIX2))

GType         ligma_param_matrix2_get_type  (void) G_GNUC_CONST;

GParamSpec *  ligma_param_spec_matrix2      (const gchar        *name,
                                            const gchar        *nick,
                                            const gchar        *blurb,
                                            const LigmaMatrix2  *default_value,
                                            GParamFlags         flags);


void          ligma_matrix2_identity        (LigmaMatrix2       *matrix);
void          ligma_matrix2_mult            (const LigmaMatrix2 *matrix1,
                                            LigmaMatrix2       *matrix2);

gdouble       ligma_matrix2_determinant     (const LigmaMatrix2 *matrix);
void          ligma_matrix2_invert          (LigmaMatrix2       *matrix);

void          ligma_matrix2_transform_point (const LigmaMatrix2 *matrix,
                                            gdouble            x,
                                            gdouble            y,
                                            gdouble           *newx,
                                            gdouble           *newy);


/*****************/
/*  LigmaMatrix3  */
/*****************/

#define LIGMA_TYPE_MATRIX3               (ligma_matrix3_get_type ())
#define LIGMA_VALUE_HOLDS_MATRIX3(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_MATRIX3))

GType         ligma_matrix3_get_type        (void) G_GNUC_CONST;


#define LIGMA_TYPE_PARAM_MATRIX3            (ligma_param_matrix3_get_type ())
#define LIGMA_IS_PARAM_SPEC_MATRIX3(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_MATRIX3))

GType         ligma_param_matrix3_get_type  (void) G_GNUC_CONST;

GParamSpec *  ligma_param_spec_matrix3      (const gchar        *name,
                                            const gchar        *nick,
                                            const gchar        *blurb,
                                            const LigmaMatrix3  *default_value,
                                            GParamFlags         flags);


void          ligma_matrix3_identity        (LigmaMatrix3       *matrix);
void          ligma_matrix3_mult            (const LigmaMatrix3 *matrix1,
                                            LigmaMatrix3       *matrix2);
void          ligma_matrix3_translate       (LigmaMatrix3       *matrix,
                                            gdouble            x,
                                            gdouble            y);
void          ligma_matrix3_scale           (LigmaMatrix3       *matrix,
                                            gdouble            x,
                                            gdouble            y);
void          ligma_matrix3_rotate          (LigmaMatrix3       *matrix,
                                            gdouble            theta);
void          ligma_matrix3_xshear          (LigmaMatrix3       *matrix,
                                            gdouble            amount);
void          ligma_matrix3_yshear          (LigmaMatrix3       *matrix,
                                            gdouble            amount);
void          ligma_matrix3_affine          (LigmaMatrix3       *matrix,
                                            gdouble            a,
                                            gdouble            b,
                                            gdouble            c,
                                            gdouble            d,
                                            gdouble            e,
                                            gdouble            f);

gdouble       ligma_matrix3_determinant     (const LigmaMatrix3 *matrix);
void          ligma_matrix3_invert          (LigmaMatrix3       *matrix);

gboolean      ligma_matrix3_is_identity     (const LigmaMatrix3 *matrix);
gboolean      ligma_matrix3_is_diagonal     (const LigmaMatrix3 *matrix);
gboolean      ligma_matrix3_is_affine       (const LigmaMatrix3 *matrix);
gboolean      ligma_matrix3_is_simple       (const LigmaMatrix3 *matrix);

gboolean      ligma_matrix3_equal           (const LigmaMatrix3 *matrix1,
                                            const LigmaMatrix3 *matrix2);

void          ligma_matrix3_transform_point (const LigmaMatrix3 *matrix,
                                            gdouble            x,
                                            gdouble            y,
                                            gdouble           *newx,
                                            gdouble           *newy);


/*****************/
/*  LigmaMatrix4  */
/*****************/

void          ligma_matrix4_identity        (LigmaMatrix4       *matrix);
void          ligma_matrix4_mult            (const LigmaMatrix4 *matrix1,
                                            LigmaMatrix4       *matrix2);

void          ligma_matrix4_to_deg          (const LigmaMatrix4 *matrix,
                                            gdouble           *a,
                                            gdouble           *b,
                                            gdouble           *c);

gdouble       ligma_matrix4_transform_point (const LigmaMatrix4 *matrix,
                                            gdouble            x,
                                            gdouble            y,
                                            gdouble            z,
                                            gdouble           *newx,
                                            gdouble           *newy,
                                            gdouble           *newz);


G_END_DECLS

#endif /* __LIGMA_MATRIX_H__ */
