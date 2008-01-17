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
gimp_levels_config_reset (GimpLevelsConfig *self)
{
  GimpHistogramChannel channel;

  g_return_if_fail (GIMP_IS_LEVELS_CONFIG (self));

  self->channel = GIMP_HISTOGRAM_VALUE;

  for (channel = GIMP_HISTOGRAM_VALUE;
       channel <= GIMP_HISTOGRAM_ALPHA;
       channel++)
    {
      gimp_levels_config_reset_channel (self, channel);
    }
}

void
gimp_levels_config_reset_channel (GimpLevelsConfig     *self,
                                  GimpHistogramChannel  channel)
{
  g_return_if_fail (GIMP_IS_LEVELS_CONFIG (self));

  self->gamma[channel]       = 1.0;
  self->low_input[channel]   = 0.0;
  self->high_input[channel]  = 1.0;
  self->low_output[channel]  = 0.0;
  self->high_output[channel] = 1.0;
}
