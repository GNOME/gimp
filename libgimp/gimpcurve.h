/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcurve.h
 * Copyright (C) 2026 Alx Sa
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_CURVE_H__
#define __GIMP_CURVE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_CURVE (gimp_curve_get_type ())
G_DECLARE_FINAL_TYPE (GimpCurve, gimp_curve, GIMP, CURVE, GObject)


GimpCurve          * gimp_curve_new               (void);

void                 gimp_curve_set_curve_type    (GimpCurve          *curve,
                                                   GimpCurveType       curve_type);

gint                 gimp_curve_get_n_points      (GimpCurve          *curve);

void                 gimp_curve_set_n_samples     (GimpCurve          *curve,
                                                   gint                n_samples);
gint                 gimp_curve_get_n_samples     (GimpCurve          *curve);

gint                 gimp_curve_add_point         (GimpCurve          *curve,
                                                   gdouble             x,
                                                   gdouble             y);
void                 gimp_curve_get_point         (GimpCurve          *curve,
                                                   gint                point,
                                                   gdouble            *x,
                                                   gdouble            *y);

GimpCurvePointType   gimp_curve_get_point_type    (GimpCurve          *curve,
                                                   gint                point);

void                 gimp_curve_set_point_type    (GimpCurve          *curve,
                                                   gint                point,
                                                   GimpCurvePointType  type);

void                 gimp_curve_delete_point      (GimpCurve          *curve,
                                                   gint                point);
void                 gimp_curve_set_point         (GimpCurve          *curve,
                                                   gint                point,
                                                   gdouble             x,
                                                   gdouble             y);
void                 gimp_curve_clear_points      (GimpCurve          *curve);

gboolean             gimp_curve_is_identity       (GimpCurve          *curve);

G_END_DECLS

#endif /* __GIMP_CURVE_H__ */
