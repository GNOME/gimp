/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#pragma once

#include "gimpdata.h"


#define GIMP_TYPE_CURVE            (gimp_curve_get_type ())
#define GIMP_CURVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CURVE, GimpCurve))
#define GIMP_CURVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CURVE, GimpCurveClass))
#define GIMP_IS_CURVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CURVE))
#define GIMP_IS_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CURVE))
#define GIMP_CURVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CURVE, GimpCurveClass))


typedef struct _GimpCurvePoint GimpCurvePoint;
typedef struct _GimpCurveClass GimpCurveClass;

struct _GimpCurvePoint
{
  gdouble            x;
  gdouble            y;

  GimpCurvePointType type;
};

struct _GimpCurve
{
  GimpData        parent_instance;

  GimpCurveType   curve_type;

  gint            n_points;
  GimpCurvePoint *points;

  gint            n_samples;
  gdouble        *samples;

  gboolean        identity;  /* whether the curve is an identity mapping */
};

struct _GimpCurveClass
{
  GimpDataClass  parent_class;
};


GType                gimp_curve_get_type          (void) G_GNUC_CONST;

GimpData           * gimp_curve_new               (const gchar        *name);
GimpData           * gimp_curve_get_standard      (void);

void                 gimp_curve_reset             (GimpCurve          *curve,
                                                   gboolean            reset_type);

void                 gimp_curve_set_curve_type    (GimpCurve          *curve,
                                                   GimpCurveType       curve_type);
GimpCurveType        gimp_curve_get_curve_type    (GimpCurve          *curve);

gint                 gimp_curve_get_n_points      (GimpCurve          *curve);

void                 gimp_curve_set_n_samples     (GimpCurve          *curve,
                                                   gint                n_samples);
gint                 gimp_curve_get_n_samples     (GimpCurve          *curve);

gint                 gimp_curve_get_point_at      (GimpCurve          *curve,
                                                   gdouble             x);
gint                 gimp_curve_get_closest_point (GimpCurve          *curve,
                                                   gdouble             x,
                                                   gdouble             y,
                                                   gdouble             max_distance);

gint                 gimp_curve_add_point         (GimpCurve          *curve,
                                                   gdouble             x,
                                                   gdouble             y);
void                 gimp_curve_delete_point      (GimpCurve          *curve,
                                                   gint                point);
void                 gimp_curve_set_point         (GimpCurve          *curve,
                                                   gint                point,
                                                   gdouble             x,
                                                   gdouble             y);
void                 gimp_curve_move_point        (GimpCurve          *curve,
                                                   gint                point,
                                                   gdouble             y);
void                 gimp_curve_get_point         (GimpCurve          *curve,
                                                   gint                point,
                                                   gdouble            *x,
                                                   gdouble            *y);
void                 gimp_curve_set_point_type    (GimpCurve          *curve,
                                                   gint                point,
                                                   GimpCurvePointType  type);
GimpCurvePointType   gimp_curve_get_point_type    (GimpCurve          *curve,
                                                   gint                point);
void                 gimp_curve_clear_points      (GimpCurve          *curve);

void                 gimp_curve_set_curve         (GimpCurve          *curve,
                                                   gdouble             x,
                                                   gdouble             y);

gboolean             gimp_curve_is_identity       (GimpCurve          *curve);

void                 gimp_curve_get_uchar         (GimpCurve          *curve,
                                                   gint                n_samples,
                                                   guchar             *samples);
