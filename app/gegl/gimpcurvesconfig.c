/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcurvesconfig.c
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "gegl-types.h"

#include "base/gimphistogram.h"

/*  temp cruft  */
#include "base/curves.h"

#include "gimpcurvesconfig.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CHANNEL,
  PROP_CURVE
};


static void   gimp_curves_config_finalize     (GObject       *object);
static void   gimp_curves_config_get_property (GObject       *object,
                                               guint          property_id,
                                               GValue        *value,
                                               GParamSpec    *pspec);
static void   gimp_curves_config_set_property (GObject       *object,
                                               guint          property_id,
                                               const GValue  *value,
                                               GParamSpec    *pspec);


G_DEFINE_TYPE (GimpCurvesConfig, gimp_curves_config, G_TYPE_OBJECT)

#define parent_class gimp_curves_config_parent_class


static void
gimp_curves_config_class_init (GimpCurvesConfigClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_curves_config_finalize;
  object_class->set_property = gimp_curves_config_set_property;
  object_class->get_property = gimp_curves_config_get_property;

  g_object_class_install_property (object_class, PROP_CHANNEL,
                                   g_param_spec_enum ("channel",
                                                      "Channel",
                                                      "The affected channel",
                                                      GIMP_TYPE_HISTOGRAM_CHANNEL,
                                                      GIMP_HISTOGRAM_VALUE,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CURVE,
                                   g_param_spec_object ("curve",
                                                        "Curve",
                                                        "Curve",
                                                        GIMP_TYPE_CURVE,
                                                        G_PARAM_READWRITE));
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
    }

  gimp_curves_config_reset (self);
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
      if (self->curve[channel])
        {
          g_object_unref (self->curve[channel]);
          self->curve[channel] = NULL;
        }
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
      break;

    case PROP_CURVE:
      if (self->curve[self->channel])
        g_object_unref (self->curve[self->channel]);
      self->curve[self->channel] = g_value_dup_object (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

void
gimp_curves_config_reset (GimpCurvesConfig *config)
{
  GimpHistogramChannel channel;

  g_return_if_fail (GIMP_IS_CURVES_CONFIG (config));

  config->channel = GIMP_HISTOGRAM_VALUE;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      gimp_curve_reset (config->curve[channel], FALSE);
    }
}

void
gimp_curves_config_reset_channel (GimpCurvesConfig     *config,
                                  GimpHistogramChannel  channel)
{
  g_return_if_fail (GIMP_IS_CURVES_CONFIG (config));

  gimp_curve_reset (config->curve[channel], FALSE);
}

gboolean
gimp_curves_config_load_cruft (GimpCurvesConfig  *config,
                               gpointer           fp,
                               GError           **error)
{
  FILE  *file = fp;
  gint   i, j;
  gint   fields;
  gchar  buf[50];
  gint   index[5][GIMP_CURVE_NUM_POINTS];
  gint   value[5][GIMP_CURVE_NUM_POINTS];

  g_return_val_if_fail (GIMP_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (file != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! fgets (buf, sizeof (buf), file) ||
      strcmp (buf, "# GIMP Curves File\n") != 0)
    {
      g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                   _("not a GIMP Curves file"));
      return FALSE;
    }

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < GIMP_CURVE_NUM_POINTS; j++)
        {
          fields = fscanf (file, "%d %d ", &index[i][j], &value[i][j]);
          if (fields != 2)
            {
              /*  FIXME: should have a helpful error message here  */
              g_printerr ("fields != 2");
              g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                           _("parse error"));
              return FALSE;
            }
        }
    }

  for (i = 0; i < 5; i++)
    {
      GimpCurve *curve = config->curve[i];

      gimp_data_freeze (GIMP_DATA (curve));

      gimp_curve_set_curve_type (curve, GIMP_CURVE_SMOOTH);

      for (j = 0; j < GIMP_CURVE_NUM_POINTS; j++)
        gimp_curve_set_point (curve, j, index[i][j], value[i][j]);

      gimp_data_thaw (GIMP_DATA (curve));
    }

  return TRUE;
}

gboolean
gimp_curves_config_save_cruft (GimpCurvesConfig *config,
                               gpointer          fp)
{
  FILE   *file = fp;
  gint    i, j;
  gint32  index;

  g_return_val_if_fail (GIMP_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (file != NULL, FALSE);

  fprintf (file, "# GIMP Curves File\n");

  for (i = 0; i < 5; i++)
    {
      GimpCurve *curve = config->curve[i];

      if (curve->curve_type == GIMP_CURVE_FREE)
        {
          /* pick representative points from the curve and make them
           * control points
           */
          for (j = 0; j <= 8; j++)
            {
              index = CLAMP0255 (j * 32);

              curve->points[j * 2][0] = index;
              curve->points[j * 2][1] = curve->curve[index];
            }
        }

      for (j = 0; j < GIMP_CURVE_NUM_POINTS; j++)
        fprintf (file, "%d %d ",
                 curve->points[j][0],
                 curve->points[j][1]);

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
      gimp_curve_get_uchar (config->curve[channel], cruft->curve[channel]);
    }

  if (! is_color)
    {
      gimp_curve_get_uchar (config->curve[GIMP_HISTOGRAM_ALPHA], cruft->curve[1]);
    }
}
