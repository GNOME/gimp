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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <glib/gstdio.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimp-gegl-types.h"

#include "base/gimphistogram.h"

/*  temp cruft  */
#include "base/curves.h"

#include "core/gimpcurve.h"

#include "gimpcurvesconfig.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
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
                         GIMP_TYPE_IMAGE_MAP_CONFIG,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_curves_config_iface_init))

#define parent_class gimp_curves_config_parent_class


static void
gimp_curves_config_class_init (GimpCurvesConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->finalize           = gimp_curves_config_finalize;
  object_class->set_property       = gimp_curves_config_set_property;
  object_class->get_property       = gimp_curves_config_get_property;

  viewable_class->default_stock_id = "gimp-tool-curves";

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_CHANNEL,
                                 "channel",
                                 "The affected channel",
                                 GIMP_TYPE_HISTOGRAM_CHANNEL,
                                 GIMP_HISTOGRAM_VALUE, 0);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_CURVE,
                                   "curve",
                                   "Curve",
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

  if (! gimp_config_serialize_property_by_name (config, "time", writer))
    return FALSE;

  old_channel = c_config->channel;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      c_config->channel = channel;

      /*  Serialize the channel properties manually (not using
       *  gimp_config_serialize_properties()), so the parent class'
       *  "time" property doesn't end up in the config file once per
       *  channel. See bug #700653.
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

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      c_config->channel = channel;
      gimp_curves_config_reset_channel (c_config);
    }

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

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      gimp_config_copy (GIMP_CONFIG (src_config->curve[channel]),
                        GIMP_CONFIG (dest_config->curve[channel]),
                        flags);
    }

  dest_config->channel = src_config->channel;

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

void
gimp_curves_config_reset_channel (GimpCurvesConfig *config)
{
  g_return_if_fail (GIMP_IS_CURVES_CONFIG (config));

  gimp_config_reset (GIMP_CONFIG (config->curve[config->channel]));
}

#define GIMP_CURVE_N_CRUFT_POINTS 17

gboolean
gimp_curves_config_load_cruft (GimpCurvesConfig  *config,
                               gpointer           fp,
                               GError           **error)
{
  FILE  *file = fp;
  gint   i, j;
  gint   fields;
  gchar  buf[50];
  gint   index[5][GIMP_CURVE_N_CRUFT_POINTS];
  gint   value[5][GIMP_CURVE_N_CRUFT_POINTS];

  g_return_val_if_fail (GIMP_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (file != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! fgets (buf, sizeof (buf), file) ||
      strcmp (buf, "# GIMP Curves File\n") != 0)
    {
      g_set_error_literal (error,
			   GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
			   _("not a GIMP Curves file"));
      return FALSE;
    }

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < GIMP_CURVE_N_CRUFT_POINTS; j++)
        {
          fields = fscanf (file, "%d %d ", &index[i][j], &value[i][j]);
          if (fields != 2)
            {
              /*  FIXME: should have a helpful error message here  */
              g_printerr ("fields != 2");
              g_set_error_literal (error,
				   GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
				   _("parse error"));
              return FALSE;
            }
        }
    }

  g_object_freeze_notify (G_OBJECT (config));

  for (i = 0; i < 5; i++)
    {
      GimpCurve *curve = config->curve[i];

      gimp_data_freeze (GIMP_DATA (curve));

      gimp_curve_set_curve_type (curve, GIMP_CURVE_SMOOTH);

      gimp_curve_reset (curve, FALSE);

      for (j = 0; j < GIMP_CURVE_N_CRUFT_POINTS; j++)
        {
          if (index[i][j] < 0 || value[i][j] < 0)
            gimp_curve_set_point (curve, j, -1.0, -1.0);
          else
            gimp_curve_set_point (curve, j,
                                  (gdouble) index[i][j] / 255.0,
                                  (gdouble) value[i][j] / 255.0);
        }

      gimp_data_thaw (GIMP_DATA (curve));
    }

  g_object_thaw_notify (G_OBJECT (config));

  return TRUE;
}

gboolean
gimp_curves_config_save_cruft (GimpCurvesConfig  *config,
                               gpointer           fp,
                               GError           **error)
{
  FILE *file = fp;
  gint  i;

  g_return_val_if_fail (GIMP_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (file != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  fprintf (file, "# GIMP Curves File\n");

  for (i = 0; i < 5; i++)
    {
      GimpCurve *curve = config->curve[i];
      gint       j;

      if (curve->curve_type == GIMP_CURVE_FREE)
        {
          gint n_points;

          for (j = 0; j < curve->n_points; j++)
            {
              curve->points[j].x = -1;
              curve->points[j].y = -1;
            }

          /* pick some points from the curve and make them control
           * points
           */
          n_points = CLAMP (9, curve->n_points / 2, curve->n_points);

          for (j = 0; j < n_points; j++)
            {
              gint sample = j * (curve->n_samples - 1) / (n_points - 1);
              gint point  = j * (curve->n_points  - 1) / (n_points - 1);

              curve->points[point].x = ((gdouble) sample /
                                        (gdouble) (curve->n_samples - 1));
              curve->points[point].y = curve->samples[sample];
            }
        }

      for (j = 0; j < curve->n_points; j++)
        {
          /* don't use gimp_curve_get_point() becaue that doesn't
           * work when the curve type is GIMP_CURVE_FREE
           */
          gdouble x = curve->points[j].x;
          gdouble y = curve->points[j].y;

          if (x < 0.0 || y < 0.0)
            {
              fprintf (file, "%d %d ", -1, -1);
            }
          else
            {
              fprintf (file, "%d %d ",
                       (gint) (x * 255.999),
                       (gint) (y * 255.999));
            }
        }

      fprintf (file, "\n");
    }

  return TRUE;
}


/*  temp cruft  */

void
gimp_curves_config_to_cruft (GimpCurvesConfig *config,
                             Curves           *cruft,
                             gboolean          is_color)
{
  GimpHistogramChannel channel;

  g_return_if_fail (GIMP_IS_CURVES_CONFIG (config));
  g_return_if_fail (cruft != NULL);

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      gimp_curve_get_uchar (config->curve[channel],
                            sizeof (cruft->curve[channel]),
                            cruft->curve[channel]);
    }

  if (! is_color)
    {
      gimp_curve_get_uchar (config->curve[GIMP_HISTOGRAM_ALPHA],
                            sizeof (cruft->curve[1]),
                            cruft->curve[1]);
    }
}
