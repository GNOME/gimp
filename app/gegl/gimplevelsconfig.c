/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplevelsconfig.c
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

#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gegl-types.h"

#include "base/gimphistogram.h"

#include "gimplevelsconfig.h"


enum
{
  PROP_0,
  PROP_CHANNEL,
  PROP_GAMMA,
  PROP_LOW_INPUT,
  PROP_HIGH_INPUT,
  PROP_LOW_OUTPUT,
  PROP_HIGH_OUTPUT
};


static void   gimp_levels_config_get_property (GObject       *object,
                                               guint          property_id,
                                               GValue        *value,
                                               GParamSpec    *pspec);
static void   gimp_levels_config_set_property (GObject       *object,
                                               guint          property_id,
                                               const GValue  *value,
                                               GParamSpec    *pspec);


G_DEFINE_TYPE (GimpLevelsConfig, gimp_levels_config, G_TYPE_OBJECT)

#define parent_class gimp_levels_config_parent_class


static void
gimp_levels_config_class_init (GimpLevelsConfigClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_levels_config_set_property;
  object_class->get_property = gimp_levels_config_get_property;

  g_object_class_install_property (object_class, PROP_CHANNEL,
                                   g_param_spec_enum ("channel",
                                                      "Channel",
                                                      "The affected channel",
                                                      GIMP_TYPE_HISTOGRAM_CHANNEL,
                                                      GIMP_HISTOGRAM_VALUE,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_GAMMA,
                                   g_param_spec_double ("gamma",
                                                        "Gamma",
                                                        "Gamma",
                                                        0.1, 10.0, 1.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LOW_INPUT,
                                   g_param_spec_double ("low-input",
                                                        "Low Input",
                                                        "Low Input",
                                                        0.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HIGH_INPUT,
                                   g_param_spec_double ("high-input",
                                                        "High Input",
                                                        "High Input",
                                                        0.0, 1.0, 1.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LOW_OUTPUT,
                                   g_param_spec_double ("low-output",
                                                        "Low Output",
                                                        "Low Output",
                                                        0.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HIGH_OUTPUT,
                                   g_param_spec_double ("high-output",
                                                        "High Output",
                                                        "High Output",
                                                        0.0, 1.0, 1.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_levels_config_init (GimpLevelsConfig *self)
{
  gimp_levels_config_reset (self);
}

static void
gimp_levels_config_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpLevelsConfig *self = GIMP_LEVELS_CONFIG (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      g_value_set_enum (value, self->channel);
      break;

    case PROP_GAMMA:
      g_value_set_double (value, self->gamma[self->channel]);
      break;

    case PROP_LOW_INPUT:
      g_value_set_double (value, self->low_input[self->channel]);
      break;

    case PROP_HIGH_INPUT:
      g_value_set_double (value, self->high_input[self->channel]);
      break;

    case PROP_LOW_OUTPUT:
      g_value_set_double (value, self->low_output[self->channel]);
      break;

    case PROP_HIGH_OUTPUT:
      g_value_set_double (value, self->high_output[self->channel]);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_levels_config_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpLevelsConfig *self = GIMP_LEVELS_CONFIG (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      self->channel = g_value_get_enum (value);
      break;

    case PROP_GAMMA:
      self->gamma[self->channel] = g_value_get_double (value);
      break;

    case PROP_LOW_INPUT:
      self->low_input[self->channel] = g_value_get_double (value);
      break;

    case PROP_HIGH_INPUT:
      self->high_input[self->channel] = g_value_get_double (value);
      break;

    case PROP_LOW_OUTPUT:
      self->low_output[self->channel] = g_value_get_double (value);
      break;

    case PROP_HIGH_OUTPUT:
      self->high_output[self->channel] = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

void
gimp_levels_config_reset (GimpLevelsConfig *config)
{
  GimpHistogramChannel channel;

  g_return_if_fail (GIMP_IS_LEVELS_CONFIG (config));

  config->channel = GIMP_HISTOGRAM_VALUE;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      gimp_levels_config_reset_channel (config, channel);
    }
}

void
gimp_levels_config_reset_channel (GimpLevelsConfig     *config,
                                  GimpHistogramChannel  channel)
{
  g_return_if_fail (GIMP_IS_LEVELS_CONFIG (config));

  config->gamma[channel]       = 1.0;
  config->low_input[channel]   = 0.0;
  config->high_input[channel]  = 1.0;
  config->low_output[channel]  = 0.0;
  config->high_output[channel] = 1.0;
}

void
gimp_levels_config_stretch (GimpLevelsConfig     *config,
                            GimpHistogram        *histogram,
                            gboolean              is_color)
{
  g_return_if_fail (GIMP_IS_LEVELS_CONFIG (config));
  g_return_if_fail (histogram != NULL);

  if (is_color)
    {
      GimpHistogramChannel channel;

     /*  Set the overall value to defaults  */
      gimp_levels_config_reset_channel (config, GIMP_HISTOGRAM_VALUE);

      for (channel = GIMP_HISTOGRAM_RED;
           channel <= GIMP_HISTOGRAM_BLUE;
           channel++)
        gimp_levels_config_stretch_channel (config, histogram, channel);
    }
  else
    {
      gimp_levels_config_stretch_channel (config, histogram,
                                          GIMP_HISTOGRAM_VALUE);
    }
}

void
gimp_levels_config_stretch_channel (GimpLevelsConfig     *config,
                                    GimpHistogram        *histogram,
                                    GimpHistogramChannel  channel)
{
  gdouble count;
  gint    i;

  g_return_if_fail (GIMP_IS_LEVELS_CONFIG (config));
  g_return_if_fail (histogram != NULL);

  config->gamma[channel]       = 1.0;
  config->low_output[channel]  = 0.0;
  config->high_output[channel] = 1.0;

  count = gimp_histogram_get_count (histogram, channel, 0, 255);

  if (count == 0.0)
    {
      config->low_input[channel]  = 0.0;
      config->high_input[channel] = 0.0;
    }
  else
    {
      gdouble new_count;
      gdouble percentage;
      gdouble next_percentage;

      /*  Set the low input  */
      new_count = 0.0;

      for (i = 0; i < 255; i++)
        {
          new_count += gimp_histogram_get_value (histogram, channel, i);
          percentage = new_count / count;
          next_percentage = (new_count +
                             gimp_histogram_get_value (histogram,
                                                       channel,
                                                       i + 1)) / count;

          if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
            {
              config->low_input[channel] = (gdouble) (i + 1) / 255.0;
              break;
            }
        }

      /*  Set the high input  */
      new_count = 0.0;

      for (i = 255; i > 0; i--)
        {
          new_count += gimp_histogram_get_value (histogram, channel, i);
          percentage = new_count / count;
          next_percentage = (new_count +
                             gimp_histogram_get_value (histogram,
                                                       channel,
                                                       i - 1)) / count;

          if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
            {
              config->high_input[channel] = (gdouble) (i - 1) / 255.0;
              break;
            }
        }
    }
}

static gdouble
gimp_levels_config_input_from_color (GimpHistogramChannel  channel,
                                     const GimpRGB        *color)
{
  switch (channel)
    {
    case GIMP_HISTOGRAM_VALUE:
      return MAX (MAX (color->r, color->g), color->b);

    case GIMP_HISTOGRAM_RED:
      return color->r;

    case GIMP_HISTOGRAM_GREEN:
      return color->g;

    case GIMP_HISTOGRAM_BLUE:
      return color->b;

    case GIMP_HISTOGRAM_ALPHA:
      return color->a;

    case GIMP_HISTOGRAM_RGB:
      return MIN (MIN (color->r, color->g), color->b);
    }

  return 0.0;
}

void
gimp_levels_config_adjust_by_colors (GimpLevelsConfig     *config,
                                     GimpHistogramChannel  channel,
                                     const GimpRGB        *black,
                                     const GimpRGB        *gray,
                                     const GimpRGB        *white)
{
  g_return_if_fail (GIMP_IS_LEVELS_CONFIG (config));

  if (black)
    config->low_input[channel] = gimp_levels_config_input_from_color (channel,
                                                                      black);

  if (white)
    config->high_input[channel] = gimp_levels_config_input_from_color (channel,
                                                                       white);

  if (gray)
    {
      gdouble input;
      gdouble range;
      gdouble inten;
      gdouble out_light;
      gdouble lightness;

      /* Calculate lightness value */
      lightness = GIMP_RGB_LUMINANCE (gray->r, gray->g, gray->b);

      input = gimp_levels_config_input_from_color (channel, gray);

      range = config->high_input[channel] - config->low_input[channel];
      if (range <= 0)
        return;

      input -= config->low_input[channel];
      if (input < 0)
        return;

      /* Normalize input and lightness */
      inten = input / range;
      out_light = lightness/ range;

      if (out_light <= 0)
        return;

      /* Map selected color to corresponding lightness */
      config->gamma[channel] = log (inten) / log (out_light);
    }
}
