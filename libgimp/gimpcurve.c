/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcurve.c
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

#include "config.h"

#include "gimp.h"

#include "gimpcurve.h"


enum
{
  PROP_0,
  PROP_CURVE_TYPE,
  PROP_N_POINTS,
  PROP_POINTS,
  PROP_POINT_TYPES,
  PROP_N_SAMPLES,
  PROP_SAMPLES,
  N_PROPS
};
static GParamSpec *obj_props[N_PROPS] = { NULL, };

typedef struct
{
  gdouble           x;
  gdouble           y;

 GimpCurvePointType type;
} GimpCurvePoint;

struct _GimpCurve
{
  GObject           parent_instance;

  GimpCurveType     curve_type;

  gint              n_points;
  GimpCurvePoint   *points;

  gint              n_samples;
  gdouble          *samples;

  gboolean          identity;   /* whether the curve is an identity mapping */
};

static void          gimp_curve_finalize          (GObject          *object);
static void          gimp_curve_set_property      (GObject          *object,
                                                   guint             property_id,
                                                   const GValue     *value,
                                                   GParamSpec       *pspec);
static void          gimp_curve_get_property      (GObject          *object,
                                                   guint             property_id,
                                                   GValue           *value,
                                                   GParamSpec       *pspec);


G_DEFINE_TYPE (GimpCurve, gimp_curve, G_TYPE_OBJECT);

static void  gimp_curve_class_init (GimpCurveClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *array_spec;

  object_class->finalize     = gimp_curve_finalize;
  object_class->set_property = gimp_curve_set_property;
  object_class->get_property = gimp_curve_get_property;

  obj_props[PROP_CURVE_TYPE] =
      g_param_spec_enum ("curve-type",
                         "Curve Type",
                         "The curve type",
                         GIMP_TYPE_CURVE_TYPE,
                         GIMP_CURVE_SMOOTH,
                         GIMP_CONFIG_PARAM_FLAGS);

  obj_props[PROP_N_POINTS] =
      g_param_spec_int ("n-points",
                        "Number of Points",
                        "The number of points",
                        0, G_MAXINT, 0,
                        /* for backward compatibility */
                        GIMP_CONFIG_PARAM_IGNORE | GIMP_CONFIG_PARAM_FLAGS);

  array_spec = g_param_spec_double ("point", NULL, NULL,
                                    -1.0, 1.0, 0.0, GIMP_PARAM_READWRITE);
  obj_props[PROP_POINTS] =
      gimp_param_spec_value_array ("points",
                                   NULL, NULL,
                                   array_spec,
                                   GIMP_CONFIG_PARAM_FLAGS);

  array_spec = g_param_spec_enum ("point-type", NULL, NULL,
                                  GIMP_TYPE_CURVE_POINT_TYPE,
                                  GIMP_CURVE_POINT_SMOOTH,
                                  GIMP_PARAM_READWRITE);
  obj_props[PROP_POINT_TYPES] =
      gimp_param_spec_value_array ("point-types",
                                   NULL, NULL,
                                   array_spec,
                                   GIMP_CONFIG_PARAM_FLAGS);

  obj_props[PROP_N_SAMPLES] =
      g_param_spec_int ("n-samples",
                        "Number of Samples",
                        "The number of samples",
                        256, 256, 256,
                        GIMP_CONFIG_PARAM_FLAGS);

  array_spec = g_param_spec_double ("sample", NULL, NULL,
                                    0.0, 1.0, 0.0, GIMP_PARAM_READWRITE);
  obj_props[PROP_SAMPLES] =
      gimp_param_spec_value_array ("samples",
                                   NULL, NULL,
                                   array_spec,
                                   GIMP_CONFIG_PARAM_FLAGS);

  g_object_class_install_properties (object_class, N_PROPS, obj_props);
}

static void
gimp_curve_init (GimpCurve *curve)
{
  curve->n_points  = 0;
  curve->points    = NULL;
  curve->n_samples = 0;
  curve->samples   = NULL;
  curve->identity  = FALSE;
}

static void
gimp_curve_finalize (GObject *object)
{
  GimpCurve *curve = GIMP_CURVE (object);

  g_clear_pointer (&curve->points,  g_free);
  curve->n_points = 0;

  g_clear_pointer (&curve->samples, g_free);
  curve->n_samples = 0;
}

static void
gimp_curve_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpCurve *curve = GIMP_CURVE (object);

  switch (property_id)
    {
    case PROP_CURVE_TYPE:
      gimp_curve_set_curve_type (curve, g_value_get_enum (value));
      break;

    case PROP_N_POINTS:
      /* ignored */
      break;

    case PROP_POINTS:
      {
        GimpValueArray *array = g_value_get_boxed (value);
        GimpCurvePoint *points;
        gint            length;
        gint            n_points;
        gint            i;

        if (! array)
          {
            gimp_curve_clear_points (curve);

            break;
          }

        length = gimp_value_array_length (array) / 2;

        n_points = 0;
        points   = g_new0 (GimpCurvePoint, length);

        for (i = 0; i < length; i++)
          {
            GValue *x = gimp_value_array_index (array, i * 2);
            GValue *y = gimp_value_array_index (array, i * 2 + 1);

            /* for backward compatibility */
            if (g_value_get_double (x) < 0.0)
              continue;

            points[n_points].x = CLAMP (g_value_get_double (x), 0.0, 1.0);
            points[n_points].y = CLAMP (g_value_get_double (y), 0.0, 1.0);

            if (n_points > 0)
              {
                points[n_points].x = MAX (points[n_points].x,
                                          points[n_points - 1].x);
              }

            if (n_points < curve->n_points)
              points[n_points].type = curve->points[n_points].type;
            else
              points[n_points].type = GIMP_CURVE_POINT_SMOOTH;

            n_points++;
          }

        g_free (curve->points);

        curve->n_points = n_points;
        curve->points   = points;

        g_object_notify_by_pspec (object, obj_props[PROP_N_POINTS]);
        g_object_notify_by_pspec (object, obj_props[PROP_POINT_TYPES]);
      }
      break;

    case PROP_POINT_TYPES:
      {
        GimpValueArray *array = g_value_get_boxed (value);
        GimpCurvePoint *points;
        gint            length;
        gdouble         x     = 0.0;
        gdouble         y     = 0.0;
        gint            i;

        if (! array)
          {
            gimp_curve_clear_points (curve);

            break;
          }

        length = gimp_value_array_length (array);

        points = g_new0 (GimpCurvePoint, length);

        for (i = 0; i < length; i++)
          {
            GValue *type = gimp_value_array_index (array, i);

            points[i].type = g_value_get_uint (type);

            if (i < curve->n_points)
              {
                x = curve->points[i].x;
                y = curve->points[i].y;
              }

            points[i].x = x;
            points[i].y = y;
          }

        g_free (curve->points);

        curve->n_points = length;
        curve->points   = points;

        g_object_notify_by_pspec (object, obj_props[PROP_N_POINTS]);
        g_object_notify_by_pspec (object, obj_props[PROP_POINTS]);
      }
      break;

    case PROP_N_SAMPLES:
      gimp_curve_set_n_samples (curve, g_value_get_int (value));
      break;

    case PROP_SAMPLES:
      {
        GimpValueArray *array = g_value_get_boxed (value);
        gint            length;
        gint            i;

        if (! array)
          break;

        length = gimp_value_array_length (array);

        for (i = 0; i < curve->n_samples && i < length; i++)
          {
            GValue *v = gimp_value_array_index (array, i);

            curve->samples[i] = CLAMP (g_value_get_double (v), 0.0, 1.0);
          }
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_curve_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GimpCurve *curve = GIMP_CURVE (object);

  switch (property_id)
    {
    case PROP_CURVE_TYPE:
      g_value_set_enum (value, curve->curve_type);
      break;

    case PROP_N_POINTS:
      g_value_set_int (value, curve->n_points);
      break;

    case PROP_POINTS:
      {
        GimpValueArray *array = gimp_value_array_new (curve->n_points * 2);
        GValue          v     = G_VALUE_INIT;
        gint            i;

        g_value_init (&v, G_TYPE_DOUBLE);

        for (i = 0; i < curve->n_points; i++)
          {
            g_value_set_double (&v, curve->points[i].x);
            gimp_value_array_append (array, &v);

            g_value_set_double (&v, curve->points[i].y);
            gimp_value_array_append (array, &v);
          }

        g_value_unset (&v);

        g_value_take_boxed (value, array);
      }
      break;

    case PROP_POINT_TYPES:
      {
        GimpValueArray *array = gimp_value_array_new (curve->n_points);
        GValue          v     = G_VALUE_INIT;
        gint            i;

        g_value_init (&v, GIMP_TYPE_CURVE_POINT_TYPE);

        for (i = 0; i < curve->n_points; i++)
          {
            g_value_set_enum (&v, curve->points[i].type);
            gimp_value_array_append (array, &v);
          }

        g_value_unset (&v);

        g_value_take_boxed (value, array);
      }
      break;

    case PROP_N_SAMPLES:
      g_value_set_int (value, curve->n_samples);
      break;

    case PROP_SAMPLES:
      {
        GimpValueArray *array = gimp_value_array_new (curve->n_samples);
        GValue          v     = G_VALUE_INIT;
        gint            i;

        g_value_init (&v, G_TYPE_DOUBLE);

        for (i = 0; i < curve->n_samples; i++)
          {
            g_value_set_double (&v, curve->samples[i]);
            gimp_value_array_append (array, &v);
          }

        g_value_unset (&v);

        g_value_take_boxed (value, array);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* Public Functions */

GimpCurve *
gimp_curve_new (void)
{
  return g_object_new (GIMP_TYPE_CURVE, NULL);
}

void
gimp_curve_set_curve_type (GimpCurve     *curve,
                           GimpCurveType  curve_type)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));

  if (curve->curve_type != curve_type)
    {
      g_object_freeze_notify (G_OBJECT (curve));

      curve->curve_type = curve_type;

      if (curve_type == GIMP_CURVE_SMOOTH)
        {
          gint i;

          g_free (curve->points);

          /*  pick some points from the curve and make them control
           *  points
           */
          curve->n_points = 9;
          curve->points   = g_new0 (GimpCurvePoint, 9);

          for (i = 0; i < curve->n_points; i++)
            {
              gint sample = i * (curve->n_samples - 1) / (curve->n_points - 1);

              curve->points[i].x    = (gdouble) sample /
                                      (gdouble) (curve->n_samples - 1);
              curve->points[i].y    = curve->samples[sample];
              curve->points[i].type = GIMP_CURVE_POINT_SMOOTH;
            }

          g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_N_POINTS]);
          g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINTS]);
          g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINT_TYPES]);
        }
      else
        {
          gimp_curve_clear_points (curve);
        }

      g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_CURVE_TYPE]);

      g_object_thaw_notify (G_OBJECT (curve));
    }
}

gint
gimp_curve_get_n_points (GimpCurve *curve)
{
  g_return_val_if_fail (GIMP_IS_CURVE (curve), 0);

  return curve->n_points;
}

void
gimp_curve_set_n_samples (GimpCurve *curve,
                          gint       n_samples)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));
  g_return_if_fail (n_samples >= 256);
  g_return_if_fail (n_samples <= 4096);

  if (n_samples != curve->n_samples)
    {
      gint i;

      g_object_freeze_notify (G_OBJECT (curve));

      curve->n_samples = n_samples;
      g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_N_SAMPLES]);

      curve->samples = g_renew (gdouble, curve->samples, curve->n_samples);

      for (i = 0; i < curve->n_samples; i++)
        curve->samples[i] = (gdouble) i / (gdouble) (curve->n_samples - 1);

      g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_SAMPLES]);

      if (curve->curve_type == GIMP_CURVE_FREE)
        curve->identity = TRUE;

      g_object_thaw_notify (G_OBJECT (curve));
    }
}

gint
gimp_curve_get_n_samples (GimpCurve *curve)
{
  g_return_val_if_fail (GIMP_IS_CURVE (curve), 0);

  return curve->n_samples;
}

gint
gimp_curve_add_point (GimpCurve *curve,
                      gdouble    x,
                      gdouble    y)
{
  GimpCurvePoint *points;
  gint            point;

  g_return_val_if_fail (GIMP_IS_CURVE (curve), -1);

  if (curve->curve_type == GIMP_CURVE_FREE)
    return -1;

  x = CLAMP (x, 0.0, 1.0);
  y = CLAMP (y, 0.0, 1.0);

  for (point = 0; point < curve->n_points; point++)
    {
      if (curve->points[point].x > x)
        break;
    }

  points = g_new0 (GimpCurvePoint, curve->n_points + 1);

  memcpy (points,         curve->points,
          point * sizeof (GimpCurvePoint));
  memcpy (points + point + 1, curve->points + point,
          (curve->n_points - point) * sizeof (GimpCurvePoint));

  points[point].x    = x;
  points[point].y    = y;
  points[point].type = GIMP_CURVE_POINT_SMOOTH;

  g_free (curve->points);

  curve->n_points++;
  curve->points = points;

  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_N_POINTS]);
  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINTS]);
  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINT_TYPES]);

  return point;
}

void
gimp_curve_get_point (GimpCurve *curve,
                      gint       point,
                      gdouble   *x,
                      gdouble   *y)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));
  g_return_if_fail (point >= 0 && point < curve->n_points);

  if (x) *x = curve->points[point].x;
  if (y) *y = curve->points[point].y;
}

void
gimp_curve_set_point_type (GimpCurve          *curve,
                           gint                point,
                           GimpCurvePointType  type)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));
  g_return_if_fail (point >= 0 && point < curve->n_points);

  curve->points[point].type = type;

  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINT_TYPES]);
}

GimpCurvePointType
gimp_curve_get_point_type (GimpCurve *curve,
                           gint       point)
{
  g_return_val_if_fail (GIMP_IS_CURVE (curve), GIMP_CURVE_POINT_SMOOTH);
  g_return_val_if_fail (point >= 0 && point < curve->n_points, GIMP_CURVE_POINT_SMOOTH);

  return curve->points[point].type;
}

void
gimp_curve_delete_point (GimpCurve *curve,
                         gint       point)
{
  GimpCurvePoint *points;

  g_return_if_fail (GIMP_IS_CURVE (curve));
  g_return_if_fail (point >= 0 && point < curve->n_points);

  points = g_new0 (GimpCurvePoint, curve->n_points - 1);

  memcpy (points,         curve->points,
          point * sizeof (GimpCurvePoint));
  memcpy (points + point, curve->points + point + 1,
          (curve->n_points - point - 1) * sizeof (GimpCurvePoint));

  g_free (curve->points);

  curve->n_points--;
  curve->points = points;

  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_N_POINTS]);
  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINTS]);
  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINT_TYPES]);
}

void
gimp_curve_set_point (GimpCurve *curve,
                      gint       point,
                      gdouble    x,
                      gdouble    y)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));
  g_return_if_fail (point >= 0 && point < curve->n_points);

  curve->points[point].x = CLAMP (x, 0.0, 1.0);
  curve->points[point].y = CLAMP (y, 0.0, 1.0);

  if (point > 0)
    curve->points[point].x = MAX (x, curve->points[point - 1].x);

  if (point < curve->n_points - 1)
    curve->points[point].x = MIN (x, curve->points[point + 1].x);

  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINTS]);
}


void
gimp_curve_clear_points (GimpCurve *curve)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));

  if (curve->points)
    {
      g_clear_pointer (&curve->points, g_free);
      curve->n_points = 0;

      g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_N_POINTS]);
      g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINTS]);
      g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINT_TYPES]);
    }
}


/**
 * gimp_curve_is_identity:
 * @curve: a #GimpCurve object
 *
 * If this function returns %TRUE, then the curve maps each value to
 * itself. If it returns %FALSE, then this assumption can not be made.
 *
 * Returns: %TRUE if the curve is an identity mapping, %FALSE otherwise.
 **/
gboolean
gimp_curve_is_identity (GimpCurve *curve)
{
  g_return_val_if_fail (GIMP_IS_CURVE (curve), FALSE);

  return curve->identity;
}
