/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcurvesconfig.c
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "operations-types.h"

#include "core/gimp-utils.h"
#include "core/gimpcurve.h"
#include "core/gimphistogram.h"

#include "gimpcurvesconfig.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TRC,
  PROP_LINEAR,
  PROP_CHANNEL,
  PROP_CURVE
};


static void     gimp_curves_config_iface_init   (GimpConfigInterface *iface);

static void     gimp_curves_config_finalize     (GObject          *object);
static void     gimp_curves_config_get_property (GObject          *object,
                                                 guint             property_id,
                                                 GValue           *value,
                                                 GParamSpec       *pspec);
static void     gimp_curves_config_set_property (GObject          *object,
                                                 guint             property_id,
                                                 const GValue     *value,
                                                 GParamSpec       *pspec);

static gboolean gimp_curves_config_serialize    (GimpConfig       *config,
                                                 GimpConfigWriter *writer,
                                                 gpointer          data);
static gboolean gimp_curves_config_deserialize  (GimpConfig       *config,
                                                 GScanner         *scanner,
                                                 gint              nest_level,
                                                 gpointer          data);
static gboolean gimp_curves_config_equal        (GimpConfig       *a,
                                                 GimpConfig       *b);
static void     gimp_curves_config_reset        (GimpConfig       *config);
static gboolean gimp_curves_config_copy         (GimpConfig       *src,
                                                 GimpConfig       *dest,
                                                 GParamFlags       flags);

static void     gimp_curves_config_curve_dirty  (GimpCurve        *curve,
                                                 GimpCurvesConfig *config);


G_DEFINE_TYPE_WITH_CODE (GimpCurvesConfig, gimp_curves_config,
                         GIMP_TYPE_OPERATION_SETTINGS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_curves_config_iface_init))

#define parent_class gimp_curves_config_parent_class


static void
gimp_curves_config_class_init (GimpCurvesConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->finalize            = gimp_curves_config_finalize;
  object_class->set_property        = gimp_curves_config_set_property;
  object_class->get_property        = gimp_curves_config_get_property;

  viewable_class->default_icon_name = "gimp-tool-curves";

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_TRC,
                         "trc",
                         _("Tone Reproduction Curve"),
                         _("Work on linear or perceptual RGB, or following the image's TRC"),
                         GIMP_TYPE_TRC_TYPE,
                         GIMP_TRC_NON_LINEAR, 0);

  /* compat */
  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_LINEAR,
                            "linear",
                            _("Linear"),
                            _("Work on linear RGB"),
                            FALSE, 0);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_CHANNEL,
                         "channel",
                         _("Channel"),
                         _("The affected channel"),
                         GIMP_TYPE_HISTOGRAM_CHANNEL,
                         GIMP_HISTOGRAM_VALUE, 0);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_CURVE,
                           "curve",
                           _("Curve"),
                           _("Curve"),
                           GIMP_TYPE_CURVE,
                           GIMP_CONFIG_PARAM_AGGREGATE);
}

static void
gimp_curves_config_iface_init (GimpConfigInterface *iface)
{
  iface->serialize   = gimp_curves_config_serialize;
  iface->deserialize = gimp_curves_config_deserialize;
  iface->equal       = gimp_curves_config_equal;
  iface->reset       = gimp_curves_config_reset;
  iface->copy        = gimp_curves_config_copy;
}

static void
gimp_curves_config_init (GimpCurvesConfig *self)
{
  GimpHistogramChannel channel;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      self->curve[channel] = GIMP_CURVE (gimp_curve_new ("curves config"));

      g_signal_connect_object (self->curve[channel], "dirty",
                               G_CALLBACK (gimp_curves_config_curve_dirty),
                               self, 0);
    }

  gimp_config_reset (GIMP_CONFIG (self));
}

static void
gimp_curves_config_finalize (GObject *object)
{
  GimpCurvesConfig     *self = GIMP_CURVES_CONFIG (object);
  GimpHistogramChannel  channel;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      g_object_unref (self->curve[channel]);
      self->curve[channel] = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_curves_config_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpCurvesConfig *self = GIMP_CURVES_CONFIG (object);

  switch (property_id)
    {
    case PROP_TRC:
      g_value_set_enum (value, self->trc);
      break;

    case PROP_LINEAR:
      g_value_set_boolean (value, self->trc == GIMP_TRC_LINEAR ? TRUE : FALSE);
      break;

    case PROP_CHANNEL:
      g_value_set_enum (value, self->channel);
      break;

    case PROP_CURVE:
      g_value_set_object (value, self->curve[self->channel]);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_curves_config_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpCurvesConfig *self = GIMP_CURVES_CONFIG (object);

  switch (property_id)
    {
    case PROP_TRC:
      self->trc = g_value_get_enum (value);
      break;

    case PROP_LINEAR:
      self->trc = g_value_get_boolean (value) ?
                  GIMP_TRC_LINEAR : GIMP_TRC_NON_LINEAR;
      g_object_notify (object, "trc");
      break;

    case PROP_CHANNEL:
      self->channel = g_value_get_enum (value);
      g_object_notify (object, "curve");
      break;

    case PROP_CURVE:
      {
        GimpCurve *src_curve  = g_value_get_object (value);
        GimpCurve *dest_curve = self->curve[self->channel];

        if (src_curve && dest_curve)
          {
            gimp_config_copy (GIMP_CONFIG (src_curve),
                              GIMP_CONFIG (dest_curve), 0);
          }
      }
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_curves_config_serialize (GimpConfig       *config,
                              GimpConfigWriter *writer,
                              gpointer          data)
{
  GimpCurvesConfig     *c_config = GIMP_CURVES_CONFIG (config);
  GimpHistogramChannel  channel;
  GimpHistogramChannel  old_channel;
  gboolean              success = TRUE;

  if (! gimp_operation_settings_config_serialize_base (config, writer, data) ||
      ! gimp_config_serialize_property_by_name (config, "trc", writer))
    return FALSE;

  old_channel = c_config->channel;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      c_config->channel = channel;

      /*  serialize the channel properties manually (not using
       *  gimp_config_serialize_properties()), so the parent class'
       *  properties don't end up in the config file one per channel.
       *  See bug #700653.
       */
      success =
        (gimp_config_serialize_property_by_name (config, "channel", writer) &&
         gimp_config_serialize_property_by_name (config, "curve",   writer));

      if (! success)
        break;
    }

  c_config->channel = old_channel;

  return success;
}

static gboolean
gimp_curves_config_deserialize (GimpConfig *config,
                                GScanner   *scanner,
                                gint        nest_level,
                                gpointer    data)
{
  GimpCurvesConfig     *c_config = GIMP_CURVES_CONFIG (config);
  GimpHistogramChannel  old_channel;
  gboolean              success = TRUE;

  old_channel = c_config->channel;

  success = gimp_config_deserialize_properties (config, scanner, nest_level);

  g_object_set (config, "channel", old_channel, NULL);

  return success;
}

static gboolean
gimp_curves_config_equal (GimpConfig *a,
                          GimpConfig *b)
{
  GimpCurvesConfig     *config_a = GIMP_CURVES_CONFIG (a);
  GimpCurvesConfig     *config_b = GIMP_CURVES_CONFIG (b);
  GimpHistogramChannel  channel;

  if (! gimp_operation_settings_config_equal_base (a, b) ||
      config_a->trc != config_b->trc)
    return FALSE;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      GimpCurve *curve_a = config_a->curve[channel];
      GimpCurve *curve_b = config_b->curve[channel];

      if (curve_a && curve_b)
        {
          if (! gimp_config_is_equal_to (GIMP_CONFIG (curve_a),
                                         GIMP_CONFIG (curve_b)))
            return FALSE;
        }
      else if (curve_a || curve_b)
        {
          return FALSE;
        }
    }

  /* don't compare "channel" */

  return TRUE;
}

static void
gimp_curves_config_reset (GimpConfig *config)
{
  GimpCurvesConfig     *c_config = GIMP_CURVES_CONFIG (config);
  GimpHistogramChannel  channel;

  gimp_operation_settings_config_reset_base (config);

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      c_config->channel = channel;
      gimp_curves_config_reset_channel (c_config);
    }

  gimp_config_reset_property (G_OBJECT (config), "trc");
  gimp_config_reset_property (G_OBJECT (config), "channel");
}

static gboolean
gimp_curves_config_copy (GimpConfig  *src,
                         GimpConfig  *dest,
                         GParamFlags  flags)
{
  GimpCurvesConfig     *src_config  = GIMP_CURVES_CONFIG (src);
  GimpCurvesConfig     *dest_config = GIMP_CURVES_CONFIG (dest);
  GimpHistogramChannel  channel;

  if (! gimp_operation_settings_config_copy_base (src, dest, flags))
    return FALSE;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      gimp_config_copy (GIMP_CONFIG (src_config->curve[channel]),
                        GIMP_CONFIG (dest_config->curve[channel]),
                        flags);
    }

  dest_config->trc     = src_config->trc;
  dest_config->channel = src_config->channel;

  g_object_notify (G_OBJECT (dest), "trc");
  g_object_notify (G_OBJECT (dest), "channel");

  return TRUE;
}

static void
gimp_curves_config_curve_dirty (GimpCurve        *curve,
                                GimpCurvesConfig *config)
{
  g_object_notify (G_OBJECT (config), "curve");
}


/*  public functions  */

GObject *
gimp_curves_config_new_spline (gint32         channel,
                               const gdouble *points,
                               gint           n_points)
{
  GimpCurvesConfig *config;
  GimpCurve        *curve;
  gint              i;

  g_return_val_if_fail (channel >= GIMP_HISTOGRAM_VALUE &&
                        channel <= GIMP_HISTOGRAM_ALPHA, NULL);
  g_return_val_if_fail (points != NULL, NULL);
  g_return_val_if_fail (n_points >= 2 && n_points <= 1024, NULL);

  config = g_object_new (GIMP_TYPE_CURVES_CONFIG, NULL);

  curve = config->curve[channel];

  gimp_data_freeze (GIMP_DATA (curve));

  gimp_curve_set_curve_type (curve, GIMP_CURVE_SMOOTH);
  gimp_curve_clear_points (curve);

  for (i = 0; i < n_points; i++)
    gimp_curve_add_point (curve,
                          (gdouble) points[i * 2],
                          (gdouble) points[i * 2 + 1]);

  gimp_data_thaw (GIMP_DATA (curve));

  return G_OBJECT (config);
}

GObject *
gimp_curves_config_new_explicit (gint32         channel,
                                 const gdouble *samples,
                                 gint           n_samples)
{
  GimpCurvesConfig *config;
  GimpCurve        *curve;
  gint              i;

  g_return_val_if_fail (channel >= GIMP_HISTOGRAM_VALUE &&
                        channel <= GIMP_HISTOGRAM_ALPHA, NULL);
  g_return_val_if_fail (samples != NULL, NULL);
  g_return_val_if_fail (n_samples >= 2 && n_samples <= 4096, NULL);

  config = g_object_new (GIMP_TYPE_CURVES_CONFIG, NULL);

  curve = config->curve[channel];

  gimp_data_freeze (GIMP_DATA (curve));

  gimp_curve_set_curve_type (curve, GIMP_CURVE_FREE);
  gimp_curve_set_n_samples (curve, n_samples);

  for (i = 0; i < n_samples; i++)
    gimp_curve_set_curve (curve,
                          (gdouble) i / (gdouble) (n_samples - 1),
                          (gdouble) samples[i]);

  gimp_data_thaw (GIMP_DATA (curve));

  return G_OBJECT (config);
}

GObject *
gimp_curves_config_new_spline_cruft (gint32        channel,
                                     const guint8 *points,
                                     gint          n_points)
{
  GObject *config;
  gdouble *d_points;
  gint     i;

  g_return_val_if_fail (channel >= GIMP_HISTOGRAM_VALUE &&
                        channel <= GIMP_HISTOGRAM_ALPHA, NULL);
  g_return_val_if_fail (points != NULL, NULL);
  g_return_val_if_fail (n_points >= 2 && n_points <= 1024, NULL);

  d_points = g_new (gdouble, 2 * n_points);

  for (i = 0; i < n_points; i++)
    {
      d_points[i * 2]     = (gdouble) points[i * 2]     / 255.0;
      d_points[i * 2 + 1] = (gdouble) points[i * 2 + 1] / 255.0;
    }

  config = gimp_curves_config_new_spline (channel, d_points, n_points);

  g_free (d_points);

  return config;
}

GObject *
gimp_curves_config_new_explicit_cruft (gint32        channel,
                                       const guint8 *samples,
                                       gint          n_samples)
{
  GObject *config;
  gdouble *d_samples;
  gint     i;

  g_return_val_if_fail (channel >= GIMP_HISTOGRAM_VALUE &&
                        channel <= GIMP_HISTOGRAM_ALPHA, NULL);
  g_return_val_if_fail (samples != NULL, NULL);
  g_return_val_if_fail (n_samples >= 2 && n_samples <= 4096, NULL);

  d_samples = g_new (gdouble, n_samples);

  for (i = 0; i < n_samples; i++)
    {
      d_samples[i] = (gdouble) samples[i] / 255.0;
    }

  config = gimp_curves_config_new_explicit (channel, d_samples, n_samples);

  g_free (d_samples);

  return config;
}

void
gimp_curves_config_reset_channel (GimpCurvesConfig *config)
{
  g_return_if_fail (GIMP_IS_CURVES_CONFIG (config));

  gimp_config_reset (GIMP_CONFIG (config->curve[config->channel]));
}

#define GIMP_CURVE_N_CRUFT_POINTS 17

gboolean
gimp_curves_config_load_cruft (GimpCurvesConfig  *config,
                               GInputStream      *input,
                               GError           **error)
{
  GDataInputStream *data_input;
  gint              index[5][GIMP_CURVE_N_CRUFT_POINTS];
  gint              value[5][GIMP_CURVE_N_CRUFT_POINTS];
  gchar            *line;
  gsize             line_len;
  gint              i, j;

  g_return_val_if_fail (GIMP_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  data_input = g_data_input_stream_new (input);

  line_len = 64;
  line = gimp_data_input_stream_read_line_always (data_input, &line_len,
                                                  NULL, error);
  if (! line)
    return FALSE;

  if (strcmp (line, "# GIMP Curves File") != 0)
    {
      g_set_error_literal (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                           _("not a GIMP Curves file"));
      g_object_unref (data_input);
      g_free (line);
      return FALSE;
    }

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < GIMP_CURVE_N_CRUFT_POINTS; j++)
        {
          gchar *x_str = NULL;
          gchar *y_str = NULL;

          if (! (x_str = g_data_input_stream_read_upto (data_input, " ", -1,
                                                        NULL, NULL, error)) ||
              ! g_data_input_stream_read_byte (data_input,  NULL, error) ||
              ! (y_str = g_data_input_stream_read_upto (data_input, " ", -1,
                                                        NULL, NULL, error)) ||
              ! g_data_input_stream_read_byte (data_input,  NULL, error))
            {
              g_free (x_str);
              g_free (y_str);
              g_object_unref (data_input);
              return FALSE;
            }

          if (sscanf (x_str, "%d", &index[i][j]) != 1 ||
              sscanf (y_str, "%d", &value[i][j]) != 1)
            {
              g_set_error_literal (error,
                                   GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                                   _("Parse error, didn't find 2 integers"));
              g_free (x_str);
              g_free (y_str);
              g_object_unref (data_input);
              return FALSE;
            }

          g_free (x_str);
          g_free (y_str);
        }
    }

  g_object_unref (data_input);

  g_object_freeze_notify (G_OBJECT (config));

  for (i = 0; i < 5; i++)
    {
      GimpCurve *curve = config->curve[i];

      gimp_data_freeze (GIMP_DATA (curve));

      gimp_curve_set_curve_type (curve, GIMP_CURVE_SMOOTH);
      gimp_curve_clear_points (curve);

      for (j = 0; j < GIMP_CURVE_N_CRUFT_POINTS; j++)
        {
          gdouble x;
          gdouble y;

          x = (gdouble) index[i][j] / 255.0;
          y = (gdouble) value[i][j] / 255.0;

          if (x >= 0.0)
            gimp_curve_add_point (curve, x, y);
        }

      gimp_data_thaw (GIMP_DATA (curve));
    }

  config->trc = GIMP_TRC_NON_LINEAR;

  g_object_notify (G_OBJECT (config), "trc");

  g_object_thaw_notify (G_OBJECT (config));

  return TRUE;
}

gboolean
gimp_curves_config_save_cruft (GimpCurvesConfig  *config,
                               GOutputStream     *output,
                               GError           **error)
{
  GString *string;
  gint     i;

  g_return_val_if_fail (GIMP_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  string = g_string_new ("# GIMP Curves File\n");

  for (i = 0; i < 5; i++)
    {
      GimpCurve *curve = config->curve[i];
      gint       j;

      if (curve->curve_type == GIMP_CURVE_SMOOTH)
        {
          g_object_ref (curve);
        }
      else
        {
          curve = GIMP_CURVE (gimp_data_duplicate (GIMP_DATA (curve)));

          gimp_curve_set_curve_type (curve, GIMP_CURVE_SMOOTH);
        }

      for (j = 0; j < GIMP_CURVE_N_CRUFT_POINTS; j++)
        {
          gint x = -1;
          gint y = -1;

          if (j < gimp_curve_get_n_points (curve))
            {
              gdouble point_x;
              gdouble point_y;

              gimp_curve_get_point (curve, j, &point_x, &point_y);

              x = floor (point_x * 255.999);
              y = floor (point_y * 255.999);
            }

          g_string_append_printf (string, "%d %d ", x, y);
        }

      g_string_append_printf (string, "\n");

      g_object_unref (curve);
    }

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   NULL, NULL, error))
    {
      g_prefix_error (error, _("Writing curves file failed: "));
      g_string_free (string, TRUE);
      return FALSE;
    }

  g_string_free (string, TRUE);

  return TRUE;
}

#define PS_CURVE_N_MAX_POINTS 19

/* Loads Photoshop .acv Curves preset */
gboolean
gimp_curves_config_load_acv (GimpCurvesConfig  *config,
                             GInputStream      *input,
                             GError           **error)
{
  GDataInputStream *data_input;
  guint16           version = 0;
  guint16           count   = 0;
  gint              in[5][PS_CURVE_N_MAX_POINTS];
  gint              out[5][PS_CURVE_N_MAX_POINTS];

  g_return_val_if_fail (GIMP_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  data_input = g_data_input_stream_new (input);

  g_data_input_stream_set_byte_order (data_input,
                                      G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN);

  version = g_data_input_stream_read_uint16 (data_input, NULL, error);
  if (! version)
    goto error;

  count = g_data_input_stream_read_uint16 (data_input, NULL, error);
  if (! count)
    goto error;

  count = CLAMP (count, 1, 5);
  for (gint i = 0; i < count; i++)
    {
      guint16 points = 0;

      points = g_data_input_stream_read_uint16 (data_input, NULL, error);
      if (! points || points > PS_CURVE_N_MAX_POINTS)
        goto error;

      for (gint j = 0; j < points; j++)
        {
          /* Curves can start at 0, 0, so we'll check for an error instead */
          out[i][j] = g_data_input_stream_read_uint16 (data_input, NULL, error);
          if (error && *error)
            goto error;

          in[i][j] = g_data_input_stream_read_uint16 (data_input, NULL, error);
          if (error && *error)
            goto error;
        }
      if (points < PS_CURVE_N_MAX_POINTS)
        {
          in[i][points]  = -1;
          out[i][points] = -1;
        }
    }

  g_object_unref (data_input);
  g_object_freeze_notify (G_OBJECT (config));

  for (gint i = 0; i < count; i++)
    {
      GimpCurve *curve = config->curve[i];

      gimp_data_freeze (GIMP_DATA (curve));

      gimp_curve_set_curve_type (curve, GIMP_CURVE_SMOOTH);
      gimp_curve_clear_points (curve);

      for (gint j = 0; j < PS_CURVE_N_MAX_POINTS; j++)
        {
          if (in[i][j] > -1 && out[i][j] > -1)
            gimp_curve_add_point (curve, in[i][j] / 255.0, out[i][j] / 255.0);
          else
            break;
        }
      gimp_data_thaw (GIMP_DATA (curve));
    }

  config->trc = GIMP_TRC_NON_LINEAR;
  g_object_notify (G_OBJECT (config), "trc");

  g_object_thaw_notify (G_OBJECT (config));

  return TRUE;

  error:
      if (error && *error)
        g_prefix_error (error, _("Could not read header: "));
      else
        g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                     _("Could not read header: "));
    g_object_unref (data_input);

    return FALSE;
}
