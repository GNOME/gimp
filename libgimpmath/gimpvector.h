/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpvector.h
 *
 * The gimp_vector* functions were taken from:
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

#if !defined (__GIMP_MATH_H_INSIDE__) && !defined (GIMP_MATH_COMPILATION)
#error "Only <libgimpmath/gimpmath.h> can be included directly."
#endif

#ifndef __GIMP_VECTOR_H__
#define __GIMP_VECTOR_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/* Two dimensional vector functions */
/* ================================ */

GimpVector2 gimp_vector2_new               (gdouble            x,
                                            gdouble            y);
void        gimp_vector2_set               (GimpVector2       *vector,
                                            gdouble            x,
                                            gdouble            y);
gdouble     gimp_vector2_length            (const GimpVector2 *vector);
gdouble     gimp_vector2_length_val        (GimpVector2        vector);
void        gimp_vector2_mul               (GimpVector2       *vector,
                                            gdouble            factor);
GimpVector2 gimp_vector2_mul_val           (GimpVector2        vector,
                                            gdouble            factor);
void        gimp_vector2_normalize         (GimpVector2       *vector);
GimpVector2 gimp_vector2_normalize_val     (GimpVector2        vector);
void        gimp_vector2_neg               (GimpVector2       *vector);
GimpVector2 gimp_vector2_neg_val           (GimpVector2        vector);
void        gimp_vector2_add               (GimpVector2       *result,
                                            const GimpVector2 *vector1,
                                            const GimpVector2 *vector2);
GimpVector2 gimp_vector2_add_val           (GimpVector2        vector1,
                                            GimpVector2        vector2);
void        gimp_vector2_sub               (GimpVector2       *result,
                                            const GimpVector2 *vector1,
                                            const GimpVector2 *vector2);
GimpVector2 gimp_vector2_sub_val           (GimpVector2        vector1,
                                            GimpVector2        vector2);
gdouble     gimp_vector2_inner_product     (const GimpVector2 *vector1,
                                            const GimpVector2 *vector2);
gdouble     gimp_vector2_inner_product_val (GimpVector2        vector1,
                                            GimpVector2        vector2);
GimpVector2 gimp_vector2_cross_product     (const GimpVector2 *vector1,
                                            const GimpVector2 *vector2);
GimpVector2 gimp_vector2_cross_product_val (GimpVector2        vector1,
                                            GimpVector2        vector2);
void        gimp_vector2_rotate            (GimpVector2       *vector,
                                            gdouble            alpha);
GimpVector2 gimp_vector2_rotate_val        (GimpVector2        vector,
                                            gdouble            alpha);
GimpVector2 gimp_vector2_normal            (GimpVector2       *vector);
GimpVector2 gimp_vector2_normal_val        (GimpVector2        vector);

/* Three dimensional vector functions */
/* ================================== */

GimpVector3 gimp_vector3_new               (gdouble            x,
                                            gdouble            y,
                                            gdouble            z);
void        gimp_vector3_set               (GimpVector3       *vector,
                                            gdouble            x,
                                            gdouble            y,
                                            gdouble            z);
gdouble     gimp_vector3_length            (const GimpVector3 *vector);
gdouble     gimp_vector3_length_val        (GimpVector3        vector);
void        gimp_vector3_mul               (GimpVector3       *vector,
                                            gdouble            factor);
GimpVector3 gimp_vector3_mul_val           (GimpVector3        vector,
                                            gdouble            factor);
void        gimp_vector3_normalize         (GimpVector3       *vector);
GimpVector3 gimp_vector3_normalize_val     (GimpVector3        vector);
void        gimp_vector3_neg               (GimpVector3       *vector);
GimpVector3 gimp_vector3_neg_val           (GimpVector3        vector);
void        gimp_vector3_add               (GimpVector3       *result,
                                            const GimpVector3 *vector1,
                                            const GimpVector3 *vector2);
GimpVector3 gimp_vector3_add_val           (GimpVector3        vector1,
                                            GimpVector3        vector2);
void        gimp_vector3_sub               (GimpVector3       *result,
                                            const GimpVector3 *vector1,
                                            const GimpVector3 *vector2);
GimpVector3 gimp_vector3_sub_val           (GimpVector3        vector1,
                                            GimpVector3        vector2);
gdouble     gimp_vector3_inner_product     (const GimpVector3 *vector1,
                                            const GimpVector3 *vector2);
gdouble     gimp_vector3_inner_product_val (GimpVector3        vector1,
                                            GimpVector3        vector2);
GimpVector3 gimp_vector3_cross_product     (const GimpVector3 *vector1,
                                            const GimpVector3 *vector2);
GimpVector3 gimp_vector3_cross_product_val (GimpVector3        vector1,
                                            GimpVector3        vector2);
void        gimp_vector3_rotate            (GimpVector3       *vector,
                                            gdouble            alpha,
                                            gdouble            beta,
                                            gdouble            gamma);
GimpVector3 gimp_vector3_rotate_val        (GimpVector3        vector,
                                            gdouble            alpha,
                                            gdouble            beta,
                                            gdouble            gamma);

/* 2d <-> 3d Vector projection functions */
/* ===================================== */

void        gimp_vector_2d_to_3d           (gint               sx,
                                            gint               sy,
                                            gint               w,
                                            gint               h,
                                            gint               x,
                                            gint               y,
                                            const GimpVector3 *vp,
                                            GimpVector3       *p);

GimpVector3 gimp_vector_2d_to_3d_val       (gint               sx,
                                            gint               sy,
                                            gint               w,
                                            gint               h,
                                            gint               x,
                                            gint               y,
                                            GimpVector3        vp,
                                            GimpVector3        p);

void        gimp_vector_3d_to_2d           (gint               sx,
                                            gint               sy,
                                            gint               w,
                                            gint               h,
                                            gdouble           *x,
                                            gdouble           *y,
                                            const GimpVector3 *vp,
                                            const GimpVector3 *p);


G_END_DECLS

#endif  /* __GIMP_VECTOR_H__ */
