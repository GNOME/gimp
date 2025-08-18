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

#include "config.h"

#include <stdlib.h>
#include <string.h> /* memcmp */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpcurve.h"
#include "gimpcurve-load.h"
#include "gimpcurve-save.h"
#include "gimpparamspecs.h"

#include "gimp-intl.h"


#define EPSILON 1e-6


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


/*  local function prototypes  */

static void          gimp_curve_config_iface_init (GimpConfigInterface *iface);

static void          gimp_curve_finalize          (GObject          *object);
static void          gimp_curve_set_property      (GObject          *object,
                                                   guint             property_id,
                                                   const GValue     *value,
                                                   GParamSpec       *pspec);
static void          gimp_curve_get_property      (GObject          *object,
                                                   guint             property_id,
                                                   GValue           *value,
                                                   GParamSpec       *pspec);

static gint64        gimp_curve_get_memsize       (GimpObject       *object,
                                                   gint64           *gui_size);

static void          gimp_curve_get_preview_size  (GimpViewable     *viewable,
                                                   gint              size,
                                                   gboolean          popup,
                                                   gboolean          dot_for_dot,
                                                   gint             *width,
                                                   gint             *height);
static gboolean      gimp_curve_get_popup_size    (GimpViewable     *viewable,
                                                   gint              width,
                                                   gint              height,
                                                   gboolean          dot_for_dot,
                                                   gint             *popup_width,
                                                   gint             *popup_height);
static GimpTempBuf * gimp_curve_get_new_preview   (GimpViewable     *viewable,
                                                   GimpContext      *context,
                                                   gint              width,
                                                   gint              height,
                                                   GeglColor        *fg_color);
static gchar       * gimp_curve_get_description   (GimpViewable     *viewable,
                                                   gchar           **tooltip);

static void          gimp_curve_dirty             (GimpData         *data);
static const gchar * gimp_curve_get_extension     (GimpData         *data);
static void          gimp_curve_data_copy         (GimpData         *data,
                                                   GimpData         *src_data);

static gboolean      gimp_curve_serialize         (GimpConfig       *config,
                                                   GimpConfigWriter *writer,
                                                   gpointer          data);
static gboolean      gimp_curve_deserialize       (GimpConfig       *config,
                                                   GScanner         *scanner,
                                                   gint              nest_level,
                                                   gpointer          data);
static gboolean      gimp_curve_equal             (GimpConfig       *a,
                                                   GimpConfig       *b);
static void          _gimp_curve_reset            (GimpConfig       *config);
static gboolean      gimp_curve_config_copy       (GimpConfig       *src,
                                                   GimpConfig       *dest,
                                                   GParamFlags       flags);

static void          gimp_curve_calculate         (GimpCurve        *curve);
static void          gimp_curve_plot              (GimpCurve        *curve,
                                                   gint              p1,
                                                   gint              p2,
                                                   gint              p3,
                                                   gint              p4);


G_DEFINE_TYPE_WITH_CODE (GimpCurve, gimp_curve, GIMP_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_curve_config_iface_init))

#define parent_class gimp_curve_parent_class


static void
gimp_curve_class_init (GimpCurveClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);
  GParamSpec        *array_spec;

  object_class->finalize            = gimp_curve_finalize;
  object_class->set_property        = gimp_curve_set_property;
  object_class->get_property        = gimp_curve_get_property;

  gimp_object_class->get_memsize    = gimp_curve_get_memsize;

  viewable_class->default_icon_name = "FIXME icon name";
  viewable_class->get_preview_size  = gimp_curve_get_preview_size;
  viewable_class->get_popup_size    = gimp_curve_get_popup_size;
  viewable_class->get_new_preview   = gimp_curve_get_new_preview;
  viewable_class->get_description   = gimp_curve_get_description;

  data_class->dirty                 = gimp_curve_dirty;
  data_class->save                  = gimp_curve_save;
  data_class->get_extension         = gimp_curve_get_extension;
  data_class->copy                  = gimp_curve_data_copy;

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
gimp_curve_config_iface_init (GimpConfigInterface *iface)
{
  iface->serialize   = gimp_curve_serialize;
  iface->deserialize = gimp_curve_deserialize;
  iface->equal       = gimp_curve_equal;
  iface->reset       = _gimp_curve_reset;
  iface->copy        = gimp_curve_config_copy;
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

  G_OBJECT_CLASS (parent_class)->finalize (object);
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

            points[i].type = g_value_get_enum (type);

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

static gint64
gimp_curve_get_memsize (GimpObject *object,
                        gint64     *gui_size)
{
  GimpCurve *curve   = GIMP_CURVE (object);
  gint64     memsize = 0;

  memsize += curve->n_points  * sizeof (GimpCurvePoint);
  memsize += curve->n_samples * sizeof (gdouble);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_curve_get_preview_size (GimpViewable *viewable,
                             gint          size,
                             gboolean      popup,
                             gboolean      dot_for_dot,
                             gint         *width,
                             gint         *height)
{
  *width  = size;
  *height = size;
}

static gboolean
gimp_curve_get_popup_size (GimpViewable *viewable,
                           gint          width,
                           gint          height,
                           gboolean      dot_for_dot,
                           gint         *popup_width,
                           gint         *popup_height)
{
  *popup_width  = width  * 2;
  *popup_height = height * 2;

  return TRUE;
}

static GimpTempBuf *
gimp_curve_get_new_preview (GimpViewable *viewable,
                            GimpContext  *context,
                            gint          width,
                            gint          height,
                            GeglColor    *fg_color G_GNUC_UNUSED)
{
  return NULL;
}

static gchar *
gimp_curve_get_description (GimpViewable  *viewable,
                            gchar        **tooltip)
{
  GimpCurve *curve = GIMP_CURVE (viewable);

  return g_strdup_printf ("%s", gimp_object_get_name (curve));
}

static void
gimp_curve_dirty (GimpData *data)
{
  GimpCurve *curve = GIMP_CURVE (data);

  curve->identity = FALSE;

  gimp_curve_calculate (curve);

  GIMP_DATA_CLASS (parent_class)->dirty (data);
}

static const gchar *
gimp_curve_get_extension (GimpData *data)
{
  return GIMP_CURVE_FILE_EXTENSION;
}

static void
gimp_curve_data_copy (GimpData *data,
                      GimpData *src_data)
{
  gimp_data_freeze (data);

  gimp_config_copy (GIMP_CONFIG (src_data),
                    GIMP_CONFIG (data), 0);

  gimp_data_thaw (data);
}

static gboolean
gimp_curve_serialize (GimpConfig       *config,
                      GimpConfigWriter *writer,
                      gpointer          data)
{
  return gimp_config_serialize_properties (config, writer);
}

static gboolean
gimp_curve_deserialize (GimpConfig *config,
                        GScanner   *scanner,
                        gint        nest_level,
                        gpointer    data)
{
  gboolean success;

  success = gimp_config_deserialize_properties (config, scanner, nest_level);

  GIMP_CURVE (config)->identity = FALSE;

  return success;
}

static gboolean
gimp_curve_equal (GimpConfig *a,
                  GimpConfig *b)
{
  GimpCurve *a_curve = GIMP_CURVE (a);
  GimpCurve *b_curve = GIMP_CURVE (b);

  if (a_curve->curve_type != b_curve->curve_type)
    return FALSE;

  if (a_curve->n_points != b_curve->n_points ||
      memcmp (a_curve->points, b_curve->points,
              sizeof (GimpCurvePoint) * a_curve->n_points))
    {
      return FALSE;
    }

  if (a_curve->n_samples != b_curve->n_samples ||
      memcmp (a_curve->samples, b_curve->samples,
              sizeof (gdouble) * a_curve->n_samples))
    {
      return FALSE;
    }

  return TRUE;
}

static void
_gimp_curve_reset (GimpConfig *config)
{
  gimp_curve_reset (GIMP_CURVE (config), TRUE);
}

static gboolean
gimp_curve_config_copy (GimpConfig  *src,
                        GimpConfig  *dest,
                        GParamFlags  flags)
{
  GimpCurve *src_curve  = GIMP_CURVE (src);
  GimpCurve *dest_curve = GIMP_CURVE (dest);

  /* make sure the curve type is copied *before* the points, so that we don't
   * overwrite the copied points when changing the type
   */
  dest_curve->curve_type = src_curve->curve_type;

  gimp_config_sync (G_OBJECT (src), G_OBJECT (dest), flags);

  dest_curve->identity = src_curve->identity;

  gimp_data_dirty (GIMP_DATA (dest));

  return TRUE;
}


/*  public functions  */

GimpData *
gimp_curve_new (const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  return g_object_new (GIMP_TYPE_CURVE,
                       "name", name,
                       NULL);
}

GimpData *
gimp_curve_get_standard (void)
{
  static GimpData *standard_curve = NULL;

  if (! standard_curve)
    {
      standard_curve = gimp_curve_new ("Standard");

      gimp_data_clean (standard_curve);
      gimp_data_make_internal (standard_curve,
                               "gimp-curve-standard");

      g_object_ref (standard_curve);
    }

  return standard_curve;
}

void
gimp_curve_reset (GimpCurve *curve,
                  gboolean   reset_type)
{
  gint i;

  g_return_if_fail (GIMP_IS_CURVE (curve));

  g_object_freeze_notify (G_OBJECT (curve));

  for (i = 0; i < curve->n_samples; i++)
    curve->samples[i] = (gdouble) i / (gdouble) (curve->n_samples - 1);

  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_SAMPLES]);

  g_free (curve->points);

  curve->n_points = 2;
  curve->points   = g_new0 (GimpCurvePoint, 2);

  curve->points[0].x    = 0.0;
  curve->points[0].y    = 0.0;
  curve->points[0].type = GIMP_CURVE_POINT_SMOOTH;

  curve->points[1].x    = 1.0;
  curve->points[1].y    = 1.0;
  curve->points[1].type = GIMP_CURVE_POINT_SMOOTH;

  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_N_POINTS]);
  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINTS]);
  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINT_TYPES]);

  if (reset_type)
    {
      curve->curve_type = GIMP_CURVE_SMOOTH;
      g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_CURVE_TYPE]);
    }

  curve->identity = TRUE;

  g_object_thaw_notify (G_OBJECT (curve));

  gimp_data_dirty (GIMP_DATA (curve));
}

void
gimp_curve_set_curve_type (GimpCurve     *curve,
                           GimpCurveType  curve_type)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));

  if (curve->curve_type != curve_type)
    {
      gimp_data_freeze (GIMP_DATA (curve));

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

      gimp_data_thaw (GIMP_DATA (curve));
    }
}

GimpCurveType
gimp_curve_get_curve_type (GimpCurve *curve)
{
  g_return_val_if_fail (GIMP_IS_CURVE (curve), GIMP_CURVE_SMOOTH);

  return curve->curve_type;
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
gimp_curve_get_point_at (GimpCurve *curve,
                         gdouble    x)
{
  gint    closest_point = -1;
  gdouble distance      = EPSILON;
  gint    i;

  g_return_val_if_fail (GIMP_IS_CURVE (curve), -1);

  for (i = 0; i < curve->n_points; i++)
    {
      gdouble point_distance;

      point_distance = fabs (x - curve->points[i].x);

      if (point_distance <= distance)
        {
          closest_point = i;
          distance      = point_distance;
        }
    }

  return closest_point;
}

gint
gimp_curve_get_closest_point (GimpCurve *curve,
                              gdouble    x,
                              gdouble    y,
                              gdouble    max_distance)
{
  gint    closest_point = -1;
  gdouble distance2     = G_MAXDOUBLE;
  gint    i;

  g_return_val_if_fail (GIMP_IS_CURVE (curve), -1);

  if (max_distance >= 0.0)
    distance2 = SQR (max_distance);

  for (i = curve->n_points - 1; i >= 0; i--)
    {
      gdouble point_distance2;

      point_distance2 = SQR (x - curve->points[i].x) +
                        SQR (y - curve->points[i].y);

      if (point_distance2 <= distance2)
        {
          closest_point = i;
          distance2     = point_distance2;
        }
    }

  return closest_point;
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

  gimp_data_dirty (GIMP_DATA (curve));

  return point;
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

  gimp_data_dirty (GIMP_DATA (curve));
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

  gimp_data_dirty (GIMP_DATA (curve));
}

void
gimp_curve_move_point (GimpCurve *curve,
                       gint       point,
                       gdouble    y)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));
  g_return_if_fail (point >= 0 && point < curve->n_points);

  curve->points[point].y = CLAMP (y, 0.0, 1.0);

  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_POINTS]);

  gimp_data_dirty (GIMP_DATA (curve));
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

  gimp_data_dirty (GIMP_DATA (curve));
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

      gimp_data_dirty (GIMP_DATA (curve));
    }
}

void
gimp_curve_set_curve (GimpCurve *curve,
                      gdouble    x,
                      gdouble    y)
{
  g_return_if_fail (GIMP_IS_CURVE (curve));
  g_return_if_fail (x >= 0 && x <= 1.0);
  g_return_if_fail (y >= 0 && y <= 1.0);

  if (curve->curve_type == GIMP_CURVE_SMOOTH)
    return;

  curve->samples[ROUND (x * (gdouble) (curve->n_samples - 1))] = y;

  g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_SAMPLES]);

  gimp_data_dirty (GIMP_DATA (curve));
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

void
gimp_curve_get_uchar (GimpCurve *curve,
                      gint       n_samples,
                      guchar    *samples)
{
  gint i;

  g_return_if_fail (GIMP_IS_CURVE (curve));
  /* FIXME: support n_samples != curve->n_samples */
  g_return_if_fail (n_samples == curve->n_samples);
  g_return_if_fail (samples != NULL);

  for (i = 0; i < curve->n_samples; i++)
    samples[i] = curve->samples[i] * 255.999;
}


/*  private functions  */

static void
gimp_curve_calculate (GimpCurve *curve)
{
  gint i;
  gint p1, p2, p3, p4;

  if (gimp_data_is_frozen (GIMP_DATA (curve)))
    return;

  switch (curve->curve_type)
    {
    case GIMP_CURVE_SMOOTH:
      /*  Initialize boundary curve points */
      if (curve->n_points > 0)
        {
          GimpCurvePoint point;
          gint           boundary;

          point    = curve->points[0];
          boundary = ROUND (point.x * (gdouble) (curve->n_samples - 1));

          for (i = 0; i < boundary; i++)
            curve->samples[i] = point.y;

          point    = curve->points[curve->n_points - 1];
          boundary = ROUND (point.x * (gdouble) (curve->n_samples - 1));

          for (i = boundary; i < curve->n_samples; i++)
            curve->samples[i] = point.y;
        }

      for (i = 0; i < curve->n_points - 1; i++)
        {
          p1 = MAX (i - 1, 0);
          p2 = i;
          p3 = i + 1;
          p4 = MIN (i + 2, curve->n_points - 1);

          if (curve->points[p2].type == GIMP_CURVE_POINT_CORNER)
            p1 = p2;

          if (curve->points[p3].type == GIMP_CURVE_POINT_CORNER)
            p4 = p3;

          gimp_curve_plot (curve, p1, p2, p3, p4);
        }

      /* ensure that the control points are used exactly */
      for (i = 0; i < curve->n_points; i++)
        {
          gdouble x = curve->points[i].x;
          gdouble y = curve->points[i].y;

          curve->samples[ROUND (x * (gdouble) (curve->n_samples - 1))] = y;
        }

      g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_SAMPLES]);
      break;

    case GIMP_CURVE_FREE:
      break;
    }
}

/*
 * This function calculates the curve values between the control points
 * p2 and p3, taking the potentially existing neighbors p1 and p4 into
 * account.
 *
 * This function uses a cubic bezier curve for the individual segments and
 * calculates the necessary intermediate control points depending on the
 * neighbor curve control points.
 */
static void
gimp_curve_plot (GimpCurve *curve,
                 gint       p1,
                 gint       p2,
                 gint       p3,
                 gint       p4)
{
  gint    i;
  gdouble x0, x3;
  gdouble y0, y1, y2, y3;
  gdouble dx, dy;
  gdouble slope;

  /* the outer control points for the bezier curve. */
  x0 = curve->points[p2].x;
  y0 = curve->points[p2].y;
  x3 = curve->points[p3].x;
  y3 = curve->points[p3].y;

  /*
   * the x values of the inner control points are fixed at
   * x1 = 2/3*x0 + 1/3*x3   and  x2 = 1/3*x0 + 2/3*x3
   * this ensures that the x values increase linearly with the
   * parameter t and enables us to skip the calculation of the x
   * values altogether - just calculate y(t) evenly spaced.
   */

  dx = x3 - x0;
  dy = y3 - y0;

  if (dx <= EPSILON)
    {
      gint index;

      index = ROUND (x0 * (gdouble) (curve->n_samples - 1));

      curve->samples[index] = y3;

      return;
    }

  if (p1 == p2 && p3 == p4)
    {
      /* No information about the neighbors,
       * calculate y1 and y2 to get a straight line
       */
      y1 = y0 + dy / 3.0;
      y2 = y0 + dy * 2.0 / 3.0;
    }
  else if (p1 == p2 && p3 != p4)
    {
      /* only the right neighbor is available. Make the tangent at the
       * right endpoint parallel to the line between the left endpoint
       * and the right neighbor. Then point the tangent at the left towards
       * the control handle of the right tangent, to ensure that the curve
       * does not have an inflection point.
       */
      slope = (curve->points[p4].y - y0) / (curve->points[p4].x - x0);

      y2 = y3 - slope * dx / 3.0;
      y1 = y0 + (y2 - y0) / 2.0;
    }
  else if (p1 != p2 && p3 == p4)
    {
      /* see previous case */
      slope = (y3 - curve->points[p1].y) / (x3 - curve->points[p1].x);

      y1 = y0 + slope * dx / 3.0;
      y2 = y3 + (y1 - y3) / 2.0;
    }
  else /* (p1 != p2 && p3 != p4) */
    {
      /* Both neighbors are available. Make the tangents at the endpoints
       * parallel to the line between the opposite endpoint and the adjacent
       * neighbor.
       */
      slope = (y3 - curve->points[p1].y) / (x3 - curve->points[p1].x);

      y1 = y0 + slope * dx / 3.0;

      slope = (curve->points[p4].y - y0) / (curve->points[p4].x - x0);

      y2 = y3 - slope * dx / 3.0;
    }

  /*
   * finally calculate the y(t) values for the given bezier values. We can
   * use homogeneously distributed values for t, since x(t) increases linearly.
   */
  for (i = 0; i <= ROUND (dx * (gdouble) (curve->n_samples - 1)); i++)
    {
      gdouble y, t;
      gint    index;

      t = i / dx / (gdouble) (curve->n_samples - 1);
      y =     y0 * (1-t) * (1-t) * (1-t) +
          3 * y1 * (1-t) * (1-t) * t     +
          3 * y2 * (1-t) * t     * t     +
              y3 * t     * t     * t;

      index = i + ROUND (x0 * (gdouble) (curve->n_samples - 1));

      if (index < curve->n_samples)
        curve->samples[index] = CLAMP (y, 0.0, 1.0);
    }
}
