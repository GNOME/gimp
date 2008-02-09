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
#include <glib/gstdio.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "gegl-types.h"

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

static gboolean gimp_curves_config_equal        (GimpConfig       *a,
                                                 GimpConfig       *b);
static void     gimp_curves_config_reset        (GimpConfig       *config);
static gboolean gimp_curves_config_copy         (GimpConfig       *src,
                                                 GimpConfig       *dest,
                                                 GParamFlags       flags);

static void     gimp_curves_config_curve_dirty  (GimpCurve        *curve,
                                                 GimpCurvesConfig *config);


G_DEFINE_TYPE_WITH_CODE (GimpCurvesConfig, gimp_curves_config,
                         GIMP_TYPE_VIEWABLE,
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
  iface->equal = gimp_curves_config_equal;
  iface->reset = gimp_curves_config_reset;
  iface->copy  = gimp_curves_config_copy;
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
            gimp_config_sync (G_OBJECT (src_curve), G_OBJECT (dest_curve), 0);

            memcpy (dest_curve->points, src_curve->points,
                    sizeof (src_curve->points));
            memcpy (dest_curve->curve, src_curve->curve,
                    sizeof (src_curve->curve));
          }
      }
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_curves_config_equal (GimpConfig *a,
                          GimpConfig *b)
{
  GimpCurvesConfig     *a_config = GIMP_CURVES_CONFIG (a);
  GimpCurvesConfig     *b_config = GIMP_CURVES_CONFIG (b);
  GimpHistogramChannel  channel;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      GimpCurve *a_curve = a_config->curve[channel];
      GimpCurve *b_curve = b_config->curve[channel];

      if (a_curve && b_curve)
        {
          if (a_curve->curve_type != b_curve->curve_type)
            return FALSE;

          if (memcmp (a_curve->points, b_curve->points,
                      sizeof (b_curve->points)) ||
              memcmp (a_curve->curve, b_curve->curve,
                      sizeof (b_curve->curve)))
            return FALSE;
        }
      else if (a_curve || b_curve)
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
      GimpCurve *src_curve  = src_config->curve[channel];
      GimpCurve *dest_curve = dest_config->curve[channel];

      if (src_curve && dest_curve)
        {
          gimp_config_sync (G_OBJECT (src_curve), G_OBJECT (dest_curve), 0);

          memcpy (dest_curve->points,
                  src_curve->points, sizeof (src_curve->points));
          memcpy (dest_curve->curve,
                  src_curve->curve, sizeof (src_curve->curve));
        }
    }

  g_object_notify (G_OBJECT (dest), "curve");

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

  gimp_curve_reset (config->curve[config->channel], TRUE);
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

  g_object_freeze_notify (G_OBJECT (config));

  for (i = 0; i < 5; i++)
    {
      GimpCurve *curve = config->curve[i];

      gimp_data_freeze (GIMP_DATA (curve));

      gimp_curve_set_curve_type (curve, GIMP_CURVE_SMOOTH);

      for (j = 0; j < GIMP_CURVE_NUM_POINTS; j++)
        gimp_curve_set_point (curve, j,
                              (gdouble) index[i][j] / 255.0,
                              (gdouble) value[i][j] / 255.0);

      gimp_data_thaw (GIMP_DATA (curve));
    }

  g_object_thaw_notify (G_OBJECT (config));

  return TRUE;
}

gboolean
gimp_curves_config_save_cruft (GimpCurvesConfig *config,
                               gpointer          fp)
{
  FILE *file = fp;
  gint  i;

  g_return_val_if_fail (GIMP_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (file != NULL, FALSE);

  fprintf (file, "# GIMP Curves File\n");

  for (i = 0; i < 5; i++)
    {
      GimpCurve *curve = config->curve[i];
      gint       j;

      if (curve->curve_type == GIMP_CURVE_FREE)
        {
          /* pick representative points from the curve and make them
           * control points
           */
          for (j = 0; j <= 8; j++)
            {
              gint32 index = CLAMP0255 (j * 32);

              curve->points[j * 2][0] = (gdouble) index / 255.0;
              curve->points[j * 2][1] = curve->curve[index];
            }
        }

      for (j = 0; j < GIMP_CURVE_NUM_POINTS; j++)
        fprintf (file, "%d %d ",
                 (gint) (curve->points[j][0] * 255.999),
                 (gint) (curve->points[j][1] * 255.999));

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
                            cruft->curve[channel]);
    }

  if (! is_color)
    {
      gimp_curve_get_uchar (config->curve[GIMP_HISTOGRAM_ALPHA],
                            cruft->curve[1]);
    }
}
