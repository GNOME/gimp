/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacurvesconfig.c
 * Copyright (C) 2007 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmaconfig/ligmaconfig.h"

#include "operations-types.h"

#include "core/ligma-utils.h"
#include "core/ligmacurve.h"
#include "core/ligmahistogram.h"

#include "ligmacurvesconfig.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_TRC,
  PROP_LINEAR,
  PROP_CHANNEL,
  PROP_CURVE
};


static void     ligma_curves_config_iface_init   (LigmaConfigInterface *iface);

static void     ligma_curves_config_finalize     (GObject          *object);
static void     ligma_curves_config_get_property (GObject          *object,
                                                 guint             property_id,
                                                 GValue           *value,
                                                 GParamSpec       *pspec);
static void     ligma_curves_config_set_property (GObject          *object,
                                                 guint             property_id,
                                                 const GValue     *value,
                                                 GParamSpec       *pspec);

static gboolean ligma_curves_config_serialize    (LigmaConfig       *config,
                                                 LigmaConfigWriter *writer,
                                                 gpointer          data);
static gboolean ligma_curves_config_deserialize  (LigmaConfig       *config,
                                                 GScanner         *scanner,
                                                 gint              nest_level,
                                                 gpointer          data);
static gboolean ligma_curves_config_equal        (LigmaConfig       *a,
                                                 LigmaConfig       *b);
static void     ligma_curves_config_reset        (LigmaConfig       *config);
static gboolean ligma_curves_config_copy         (LigmaConfig       *src,
                                                 LigmaConfig       *dest,
                                                 GParamFlags       flags);

static void     ligma_curves_config_curve_dirty  (LigmaCurve        *curve,
                                                 LigmaCurvesConfig *config);


G_DEFINE_TYPE_WITH_CODE (LigmaCurvesConfig, ligma_curves_config,
                         LIGMA_TYPE_OPERATION_SETTINGS,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_curves_config_iface_init))

#define parent_class ligma_curves_config_parent_class


static void
ligma_curves_config_class_init (LigmaCurvesConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class = LIGMA_VIEWABLE_CLASS (klass);

  object_class->finalize            = ligma_curves_config_finalize;
  object_class->set_property        = ligma_curves_config_set_property;
  object_class->get_property        = ligma_curves_config_get_property;

  viewable_class->default_icon_name = "ligma-tool-curves";

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_TRC,
                         "trc",
                         _("Linear/Perceptual"),
                         _("Work on linear or perceptual RGB"),
                         LIGMA_TYPE_TRC_TYPE,
                         LIGMA_TRC_NON_LINEAR, 0);

  /* compat */
  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_LINEAR,
                            "linear",
                            _("Linear"),
                            _("Work on linear RGB"),
                            FALSE, 0);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_CHANNEL,
                         "channel",
                         _("Channel"),
                         _("The affected channel"),
                         LIGMA_TYPE_HISTOGRAM_CHANNEL,
                         LIGMA_HISTOGRAM_VALUE, 0);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_CURVE,
                           "curve",
                           _("Curve"),
                           _("Curve"),
                           LIGMA_TYPE_CURVE,
                           LIGMA_CONFIG_PARAM_AGGREGATE);
}

static void
ligma_curves_config_iface_init (LigmaConfigInterface *iface)
{
  iface->serialize   = ligma_curves_config_serialize;
  iface->deserialize = ligma_curves_config_deserialize;
  iface->equal       = ligma_curves_config_equal;
  iface->reset       = ligma_curves_config_reset;
  iface->copy        = ligma_curves_config_copy;
}

static void
ligma_curves_config_init (LigmaCurvesConfig *self)
{
  LigmaHistogramChannel channel;

  for (channel = LIGMA_HISTOGRAM_VALUE;
       channel <= LIGMA_HISTOGRAM_ALPHA;
       channel++)
    {
      self->curve[channel] = LIGMA_CURVE (ligma_curve_new ("curves config"));

      g_signal_connect_object (self->curve[channel], "dirty",
                               G_CALLBACK (ligma_curves_config_curve_dirty),
                               self, 0);
    }

  ligma_config_reset (LIGMA_CONFIG (self));
}

static void
ligma_curves_config_finalize (GObject *object)
{
  LigmaCurvesConfig     *self = LIGMA_CURVES_CONFIG (object);
  LigmaHistogramChannel  channel;

  for (channel = LIGMA_HISTOGRAM_VALUE;
       channel <= LIGMA_HISTOGRAM_ALPHA;
       channel++)
    {
      g_object_unref (self->curve[channel]);
      self->curve[channel] = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_curves_config_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaCurvesConfig *self = LIGMA_CURVES_CONFIG (object);

  switch (property_id)
    {
    case PROP_TRC:
      g_value_set_enum (value, self->trc);
      break;

    case PROP_LINEAR:
      g_value_set_boolean (value, self->trc == LIGMA_TRC_LINEAR ? TRUE : FALSE);
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
ligma_curves_config_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaCurvesConfig *self = LIGMA_CURVES_CONFIG (object);

  switch (property_id)
    {
    case PROP_TRC:
      self->trc = g_value_get_enum (value);
      break;

    case PROP_LINEAR:
      self->trc = g_value_get_boolean (value) ?
                  LIGMA_TRC_LINEAR : LIGMA_TRC_NON_LINEAR;
      g_object_notify (object, "trc");
      break;

    case PROP_CHANNEL:
      self->channel = g_value_get_enum (value);
      g_object_notify (object, "curve");
      break;

    case PROP_CURVE:
      {
        LigmaCurve *src_curve  = g_value_get_object (value);
        LigmaCurve *dest_curve = self->curve[self->channel];

        if (src_curve && dest_curve)
          {
            ligma_config_copy (LIGMA_CONFIG (src_curve),
                              LIGMA_CONFIG (dest_curve), 0);
          }
      }
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ligma_curves_config_serialize (LigmaConfig       *config,
                              LigmaConfigWriter *writer,
                              gpointer          data)
{
  LigmaCurvesConfig     *c_config = LIGMA_CURVES_CONFIG (config);
  LigmaHistogramChannel  channel;
  LigmaHistogramChannel  old_channel;
  gboolean              success = TRUE;

  if (! ligma_operation_settings_config_serialize_base (config, writer, data) ||
      ! ligma_config_serialize_property_by_name (config, "trc", writer))
    return FALSE;

  old_channel = c_config->channel;

  for (channel = LIGMA_HISTOGRAM_VALUE;
       channel <= LIGMA_HISTOGRAM_ALPHA;
       channel++)
    {
      c_config->channel = channel;

      /*  serialize the channel properties manually (not using
       *  ligma_config_serialize_properties()), so the parent class'
       *  properties don't end up in the config file one per channel.
       *  See bug #700653.
       */
      success =
        (ligma_config_serialize_property_by_name (config, "channel", writer) &&
         ligma_config_serialize_property_by_name (config, "curve",   writer));

      if (! success)
        break;
    }

  c_config->channel = old_channel;

  return success;
}

static gboolean
ligma_curves_config_deserialize (LigmaConfig *config,
                                GScanner   *scanner,
                                gint        nest_level,
                                gpointer    data)
{
  LigmaCurvesConfig     *c_config = LIGMA_CURVES_CONFIG (config);
  LigmaHistogramChannel  old_channel;
  gboolean              success = TRUE;

  old_channel = c_config->channel;

  success = ligma_config_deserialize_properties (config, scanner, nest_level);

  g_object_set (config, "channel", old_channel, NULL);

  return success;
}

static gboolean
ligma_curves_config_equal (LigmaConfig *a,
                          LigmaConfig *b)
{
  LigmaCurvesConfig     *config_a = LIGMA_CURVES_CONFIG (a);
  LigmaCurvesConfig     *config_b = LIGMA_CURVES_CONFIG (b);
  LigmaHistogramChannel  channel;

  if (! ligma_operation_settings_config_equal_base (a, b) ||
      config_a->trc != config_b->trc)
    return FALSE;

  for (channel = LIGMA_HISTOGRAM_VALUE;
       channel <= LIGMA_HISTOGRAM_ALPHA;
       channel++)
    {
      LigmaCurve *curve_a = config_a->curve[channel];
      LigmaCurve *curve_b = config_b->curve[channel];

      if (curve_a && curve_b)
        {
          if (! ligma_config_is_equal_to (LIGMA_CONFIG (curve_a),
                                         LIGMA_CONFIG (curve_b)))
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
ligma_curves_config_reset (LigmaConfig *config)
{
  LigmaCurvesConfig     *c_config = LIGMA_CURVES_CONFIG (config);
  LigmaHistogramChannel  channel;

  ligma_operation_settings_config_reset_base (config);

  for (channel = LIGMA_HISTOGRAM_VALUE;
       channel <= LIGMA_HISTOGRAM_ALPHA;
       channel++)
    {
      c_config->channel = channel;
      ligma_curves_config_reset_channel (c_config);
    }

  ligma_config_reset_property (G_OBJECT (config), "trc");
  ligma_config_reset_property (G_OBJECT (config), "channel");
}

static gboolean
ligma_curves_config_copy (LigmaConfig  *src,
                         LigmaConfig  *dest,
                         GParamFlags  flags)
{
  LigmaCurvesConfig     *src_config  = LIGMA_CURVES_CONFIG (src);
  LigmaCurvesConfig     *dest_config = LIGMA_CURVES_CONFIG (dest);
  LigmaHistogramChannel  channel;

  if (! ligma_operation_settings_config_copy_base (src, dest, flags))
    return FALSE;

  for (channel = LIGMA_HISTOGRAM_VALUE;
       channel <= LIGMA_HISTOGRAM_ALPHA;
       channel++)
    {
      ligma_config_copy (LIGMA_CONFIG (src_config->curve[channel]),
                        LIGMA_CONFIG (dest_config->curve[channel]),
                        flags);
    }

  dest_config->trc     = src_config->trc;
  dest_config->channel = src_config->channel;

  g_object_notify (G_OBJECT (dest), "trc");
  g_object_notify (G_OBJECT (dest), "channel");

  return TRUE;
}

static void
ligma_curves_config_curve_dirty (LigmaCurve        *curve,
                                LigmaCurvesConfig *config)
{
  g_object_notify (G_OBJECT (config), "curve");
}


/*  public functions  */

GObject *
ligma_curves_config_new_spline (gint32         channel,
                               const gdouble *points,
                               gint           n_points)
{
  LigmaCurvesConfig *config;
  LigmaCurve        *curve;
  gint              i;

  g_return_val_if_fail (channel >= LIGMA_HISTOGRAM_VALUE &&
                        channel <= LIGMA_HISTOGRAM_ALPHA, NULL);
  g_return_val_if_fail (points != NULL, NULL);
  g_return_val_if_fail (n_points >= 2 && n_points <= 1024, NULL);

  config = g_object_new (LIGMA_TYPE_CURVES_CONFIG, NULL);

  curve = config->curve[channel];

  ligma_data_freeze (LIGMA_DATA (curve));

  ligma_curve_set_curve_type (curve, LIGMA_CURVE_SMOOTH);
  ligma_curve_clear_points (curve);

  for (i = 0; i < n_points; i++)
    ligma_curve_add_point (curve,
                          (gdouble) points[i * 2],
                          (gdouble) points[i * 2 + 1]);

  ligma_data_thaw (LIGMA_DATA (curve));

  return G_OBJECT (config);
}

GObject *
ligma_curves_config_new_explicit (gint32         channel,
                                 const gdouble *samples,
                                 gint           n_samples)
{
  LigmaCurvesConfig *config;
  LigmaCurve        *curve;
  gint              i;

  g_return_val_if_fail (channel >= LIGMA_HISTOGRAM_VALUE &&
                        channel <= LIGMA_HISTOGRAM_ALPHA, NULL);
  g_return_val_if_fail (samples != NULL, NULL);
  g_return_val_if_fail (n_samples >= 2 && n_samples <= 4096, NULL);

  config = g_object_new (LIGMA_TYPE_CURVES_CONFIG, NULL);

  curve = config->curve[channel];

  ligma_data_freeze (LIGMA_DATA (curve));

  ligma_curve_set_curve_type (curve, LIGMA_CURVE_FREE);
  ligma_curve_set_n_samples (curve, n_samples);

  for (i = 0; i < n_samples; i++)
    ligma_curve_set_curve (curve,
                          (gdouble) i / (gdouble) (n_samples - 1),
                          (gdouble) samples[i]);

  ligma_data_thaw (LIGMA_DATA (curve));

  return G_OBJECT (config);
}

GObject *
ligma_curves_config_new_spline_cruft (gint32        channel,
                                     const guint8 *points,
                                     gint          n_points)
{
  GObject *config;
  gdouble *d_points;
  gint     i;

  g_return_val_if_fail (channel >= LIGMA_HISTOGRAM_VALUE &&
                        channel <= LIGMA_HISTOGRAM_ALPHA, NULL);
  g_return_val_if_fail (points != NULL, NULL);
  g_return_val_if_fail (n_points >= 2 && n_points <= 1024, NULL);

  d_points = g_new (gdouble, 2 * n_points);

  for (i = 0; i < n_points; i++)
    {
      d_points[i * 2]     = (gdouble) points[i * 2]     / 255.0;
      d_points[i * 2 + 1] = (gdouble) points[i * 2 + 1] / 255.0;
    }

  config = ligma_curves_config_new_spline (channel, d_points, n_points);

  g_free (d_points);

  return config;
}

GObject *
ligma_curves_config_new_explicit_cruft (gint32        channel,
                                       const guint8 *samples,
                                       gint          n_samples)
{
  GObject *config;
  gdouble *d_samples;
  gint     i;

  g_return_val_if_fail (channel >= LIGMA_HISTOGRAM_VALUE &&
                        channel <= LIGMA_HISTOGRAM_ALPHA, NULL);
  g_return_val_if_fail (samples != NULL, NULL);
  g_return_val_if_fail (n_samples >= 2 && n_samples <= 4096, NULL);

  d_samples = g_new (gdouble, n_samples);

  for (i = 0; i < n_samples; i++)
    {
      d_samples[i] = (gdouble) samples[i] / 255.0;
    }

  config = ligma_curves_config_new_explicit (channel, d_samples, n_samples);

  g_free (d_samples);

  return config;
}

void
ligma_curves_config_reset_channel (LigmaCurvesConfig *config)
{
  g_return_if_fail (LIGMA_IS_CURVES_CONFIG (config));

  ligma_config_reset (LIGMA_CONFIG (config->curve[config->channel]));
}

#define LIGMA_CURVE_N_CRUFT_POINTS 17

gboolean
ligma_curves_config_load_cruft (LigmaCurvesConfig  *config,
                               GInputStream      *input,
                               GError           **error)
{
  GDataInputStream *data_input;
  gint              index[5][LIGMA_CURVE_N_CRUFT_POINTS];
  gint              value[5][LIGMA_CURVE_N_CRUFT_POINTS];
  gchar            *line;
  gsize             line_len;
  gint              i, j;

  g_return_val_if_fail (LIGMA_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  data_input = g_data_input_stream_new (input);

  line_len = 64;
  line = ligma_data_input_stream_read_line_always (data_input, &line_len,
                                                  NULL, error);
  if (! line)
    return FALSE;

  if (strcmp (line, "# LIGMA Curves File") != 0)
    {
      g_set_error_literal (error, LIGMA_CONFIG_ERROR, LIGMA_CONFIG_ERROR_PARSE,
                           _("not a LIGMA Curves file"));
      g_object_unref (data_input);
      g_free (line);
      return FALSE;
    }

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < LIGMA_CURVE_N_CRUFT_POINTS; j++)
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
                                   LIGMA_CONFIG_ERROR, LIGMA_CONFIG_ERROR_PARSE,
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
      LigmaCurve *curve = config->curve[i];

      ligma_data_freeze (LIGMA_DATA (curve));

      ligma_curve_set_curve_type (curve, LIGMA_CURVE_SMOOTH);
      ligma_curve_clear_points (curve);

      for (j = 0; j < LIGMA_CURVE_N_CRUFT_POINTS; j++)
        {
          gdouble x;
          gdouble y;

          x = (gdouble) index[i][j] / 255.0;
          y = (gdouble) value[i][j] / 255.0;

          if (x >= 0.0)
            ligma_curve_add_point (curve, x, y);
        }

      ligma_data_thaw (LIGMA_DATA (curve));
    }

  config->trc = LIGMA_TRC_NON_LINEAR;

  g_object_notify (G_OBJECT (config), "trc");

  g_object_thaw_notify (G_OBJECT (config));

  return TRUE;
}

gboolean
ligma_curves_config_save_cruft (LigmaCurvesConfig  *config,
                               GOutputStream     *output,
                               GError           **error)
{
  GString *string;
  gint     i;

  g_return_val_if_fail (LIGMA_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  string = g_string_new ("# LIGMA Curves File\n");

  for (i = 0; i < 5; i++)
    {
      LigmaCurve *curve = config->curve[i];
      gint       j;

      if (curve->curve_type == LIGMA_CURVE_SMOOTH)
        {
          g_object_ref (curve);
        }
      else
        {
          curve = LIGMA_CURVE (ligma_data_duplicate (LIGMA_DATA (curve)));

          ligma_curve_set_curve_type (curve, LIGMA_CURVE_SMOOTH);
        }

      for (j = 0; j < LIGMA_CURVE_N_CRUFT_POINTS; j++)
        {
          gint x = -1;
          gint y = -1;

          if (j < ligma_curve_get_n_points (curve))
            {
              gdouble point_x;
              gdouble point_y;

              ligma_curve_get_point (curve, j, &point_x, &point_y);

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
