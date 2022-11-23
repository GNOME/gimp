/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_CURVE_H__
#define __LIGMA_CURVE_H__


#include "ligmadata.h"


#define LIGMA_TYPE_CURVE            (ligma_curve_get_type ())
#define LIGMA_CURVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CURVE, LigmaCurve))
#define LIGMA_CURVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CURVE, LigmaCurveClass))
#define LIGMA_IS_CURVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CURVE))
#define LIGMA_IS_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CURVE))
#define LIGMA_CURVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CURVE, LigmaCurveClass))


typedef struct _LigmaCurvePoint LigmaCurvePoint;
typedef struct _LigmaCurveClass LigmaCurveClass;

struct _LigmaCurvePoint
{
  gdouble            x;
  gdouble            y;

  LigmaCurvePointType type;
};

struct _LigmaCurve
{
  LigmaData        parent_instance;

  LigmaCurveType   curve_type;

  gint            n_points;
  LigmaCurvePoint *points;

  gint            n_samples;
  gdouble        *samples;

  gboolean        identity;  /* whether the curve is an identity mapping */
};

struct _LigmaCurveClass
{
  LigmaDataClass  parent_class;
};


GType                ligma_curve_get_type          (void) G_GNUC_CONST;

LigmaData           * ligma_curve_new               (const gchar        *name);
LigmaData           * ligma_curve_get_standard      (void);

void                 ligma_curve_reset             (LigmaCurve          *curve,
                                                   gboolean            reset_type);

void                 ligma_curve_set_curve_type    (LigmaCurve          *curve,
                                                   LigmaCurveType       curve_type);
LigmaCurveType        ligma_curve_get_curve_type    (LigmaCurve          *curve);

gint                 ligma_curve_get_n_points      (LigmaCurve          *curve);

void                 ligma_curve_set_n_samples     (LigmaCurve          *curve,
                                                   gint                n_samples);
gint                 ligma_curve_get_n_samples     (LigmaCurve          *curve);

gint                 ligma_curve_get_point_at      (LigmaCurve          *curve,
                                                   gdouble             x);
gint                 ligma_curve_get_closest_point (LigmaCurve          *curve,
                                                   gdouble             x,
                                                   gdouble             y,
                                                   gdouble             max_distance);

gint                 ligma_curve_add_point         (LigmaCurve          *curve,
                                                   gdouble             x,
                                                   gdouble             y);
void                 ligma_curve_delete_point      (LigmaCurve          *curve,
                                                   gint                point);
void                 ligma_curve_set_point         (LigmaCurve          *curve,
                                                   gint                point,
                                                   gdouble             x,
                                                   gdouble             y);
void                 ligma_curve_move_point        (LigmaCurve          *curve,
                                                   gint                point,
                                                   gdouble             y);
void                 ligma_curve_get_point         (LigmaCurve          *curve,
                                                   gint                point,
                                                   gdouble            *x,
                                                   gdouble            *y);
void                 ligma_curve_set_point_type    (LigmaCurve          *curve,
                                                   gint                point,
                                                   LigmaCurvePointType  type);
LigmaCurvePointType   ligma_curve_get_point_type    (LigmaCurve          *curve,
                                                   gint                point);
void                 ligma_curve_clear_points      (LigmaCurve          *curve);

void                 ligma_curve_set_curve         (LigmaCurve          *curve,
                                                   gdouble             x,
                                                   gdouble             y);

gboolean             ligma_curve_is_identity       (LigmaCurve          *curve);

void                 ligma_curve_get_uchar         (LigmaCurve          *curve,
                                                   gint                n_samples,
                                                   guchar             *samples);


#endif /* __LIGMA_CURVE_H__ */
