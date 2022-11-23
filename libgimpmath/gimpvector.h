/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmavector.h
 *
 * The ligma_vector* functions were taken from:
 * GCK - The General Convenience Kit
 * Copyright (C) 1996 Tom Bech
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

#ifndef __LIGMA_VECTOR_H__
#define __LIGMA_VECTOR_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/* Two dimensional vector functions */
/* ================================ */

LigmaVector2 ligma_vector2_new               (gdouble            x,
                                            gdouble            y);
void        ligma_vector2_set               (LigmaVector2       *vector,
                                            gdouble            x,
                                            gdouble            y);
gdouble     ligma_vector2_length            (const LigmaVector2 *vector);
gdouble     ligma_vector2_length_val        (LigmaVector2        vector);
void        ligma_vector2_mul               (LigmaVector2       *vector,
                                            gdouble            factor);
LigmaVector2 ligma_vector2_mul_val           (LigmaVector2        vector,
                                            gdouble            factor);
void        ligma_vector2_normalize         (LigmaVector2       *vector);
LigmaVector2 ligma_vector2_normalize_val     (LigmaVector2        vector);
void        ligma_vector2_neg               (LigmaVector2       *vector);
LigmaVector2 ligma_vector2_neg_val           (LigmaVector2        vector);
void        ligma_vector2_add               (LigmaVector2       *result,
                                            const LigmaVector2 *vector1,
                                            const LigmaVector2 *vector2);
LigmaVector2 ligma_vector2_add_val           (LigmaVector2        vector1,
                                            LigmaVector2        vector2);
void        ligma_vector2_sub               (LigmaVector2       *result,
                                            const LigmaVector2 *vector1,
                                            const LigmaVector2 *vector2);
LigmaVector2 ligma_vector2_sub_val           (LigmaVector2        vector1,
                                            LigmaVector2        vector2);
gdouble     ligma_vector2_inner_product     (const LigmaVector2 *vector1,
                                            const LigmaVector2 *vector2);
gdouble     ligma_vector2_inner_product_val (LigmaVector2        vector1,
                                            LigmaVector2        vector2);
LigmaVector2 ligma_vector2_cross_product     (const LigmaVector2 *vector1,
                                            const LigmaVector2 *vector2);
LigmaVector2 ligma_vector2_cross_product_val (LigmaVector2        vector1,
                                            LigmaVector2        vector2);
void        ligma_vector2_rotate            (LigmaVector2       *vector,
                                            gdouble            alpha);
LigmaVector2 ligma_vector2_rotate_val        (LigmaVector2        vector,
                                            gdouble            alpha);
LigmaVector2 ligma_vector2_normal            (LigmaVector2       *vector);
LigmaVector2 ligma_vector2_normal_val        (LigmaVector2        vector);

/* Three dimensional vector functions */
/* ================================== */

LigmaVector3 ligma_vector3_new               (gdouble            x,
                                            gdouble            y,
                                            gdouble            z);
void        ligma_vector3_set               (LigmaVector3       *vector,
                                            gdouble            x,
                                            gdouble            y,
                                            gdouble            z);
gdouble     ligma_vector3_length            (const LigmaVector3 *vector);
gdouble     ligma_vector3_length_val        (LigmaVector3        vector);
void        ligma_vector3_mul               (LigmaVector3       *vector,
                                            gdouble            factor);
LigmaVector3 ligma_vector3_mul_val           (LigmaVector3        vector,
                                            gdouble            factor);
void        ligma_vector3_normalize         (LigmaVector3       *vector);
LigmaVector3 ligma_vector3_normalize_val     (LigmaVector3        vector);
void        ligma_vector3_neg               (LigmaVector3       *vector);
LigmaVector3 ligma_vector3_neg_val           (LigmaVector3        vector);
void        ligma_vector3_add               (LigmaVector3       *result,
                                            const LigmaVector3 *vector1,
                                            const LigmaVector3 *vector2);
LigmaVector3 ligma_vector3_add_val           (LigmaVector3        vector1,
                                            LigmaVector3        vector2);
void        ligma_vector3_sub               (LigmaVector3       *result,
                                            const LigmaVector3 *vector1,
                                            const LigmaVector3 *vector2);
LigmaVector3 ligma_vector3_sub_val           (LigmaVector3        vector1,
                                            LigmaVector3        vector2);
gdouble     ligma_vector3_inner_product     (const LigmaVector3 *vector1,
                                            const LigmaVector3 *vector2);
gdouble     ligma_vector3_inner_product_val (LigmaVector3        vector1,
                                            LigmaVector3        vector2);
LigmaVector3 ligma_vector3_cross_product     (const LigmaVector3 *vector1,
                                            const LigmaVector3 *vector2);
LigmaVector3 ligma_vector3_cross_product_val (LigmaVector3        vector1,
                                            LigmaVector3        vector2);
void        ligma_vector3_rotate            (LigmaVector3       *vector,
                                            gdouble            alpha,
                                            gdouble            beta,
                                            gdouble            gamma);
LigmaVector3 ligma_vector3_rotate_val        (LigmaVector3        vector,
                                            gdouble            alpha,
                                            gdouble            beta,
                                            gdouble            gamma);

/* 2d <-> 3d Vector projection functions */
/* ===================================== */

void        ligma_vector_2d_to_3d           (gint               sx,
                                            gint               sy,
                                            gint               w,
                                            gint               h,
                                            gint               x,
                                            gint               y,
                                            const LigmaVector3 *vp,
                                            LigmaVector3       *p);

LigmaVector3 ligma_vector_2d_to_3d_val       (gint               sx,
                                            gint               sy,
                                            gint               w,
                                            gint               h,
                                            gint               x,
                                            gint               y,
                                            LigmaVector3        vp,
                                            LigmaVector3        p);

void        ligma_vector_3d_to_2d           (gint               sx,
                                            gint               sy,
                                            gint               w,
                                            gint               h,
                                            gdouble           *x,
                                            gdouble           *y,
                                            const LigmaVector3 *vp,
                                            const LigmaVector3 *p);


G_END_DECLS

#endif  /* __LIGMA_VECTOR_H__ */
