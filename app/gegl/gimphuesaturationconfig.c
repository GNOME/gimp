/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphuesaturationconfig.c
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

#include "gegl-types.h"

/*  temp cruft  */
#include "base/hue-saturation.h"

#include "gimphuesaturationconfig.h"


enum
{
  PROP_0,
  PROP_RANGE,
  PROP_HUE,
  PROP_SATURATION,
  PROP_LIGHTNESS,
  PROP_OVERLAP
};


static void   gimp_hue_saturation_config_get_property (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);
static void   gimp_hue_saturation_config_set_property (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);


G_DEFINE_TYPE (GimpHueSaturationConfig, gimp_hue_saturation_config,
               G_TYPE_OBJECT)

#define parent_class gimp_hue_saturation_config_parent_class


static void
gimp_hue_saturation_config_class_init (GimpHueSaturationConfigClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_hue_saturation_config_set_property;
  object_class->get_property = gimp_hue_saturation_config_get_property;

  g_object_class_install_property (object_class, PROP_RANGE,
                                   g_param_spec_enum ("range",
                                                      "range",
                                                      "The affected range",
                                                      GIMP_TYPE_HUE_RANGE,
                                                      GIMP_ALL_HUES,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HUE,
                                   g_param_spec_double ("hue",
                                                        "Hue",
                                                        "Hue",
                                                        -1.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SATURATION,
                                   g_param_spec_double ("saturation",
                                                        "Saturation",
                                                        "Saturation",
                                                        -1.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LIGHTNESS,
                                   g_param_spec_double ("lightness",
                                                        "Lightness",
                                                        "Lightness",
                                                        -1.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OVERLAP,
                                   g_param_spec_double ("overlap",
                                                        "Overlap",
                                                        "Overlap",
                                                        0.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_hue_saturation_config_init (GimpHueSaturationConfig *self)
{
  gimp_hue_saturation_config_reset (self);
}

static void
gimp_hue_saturation_config_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  GimpHueSaturationConfig *self = GIMP_HUE_SATURATION_CONFIG (object);

  switch (property_id)
    {
    case PROP_RANGE:
      g_value_set_enum (value, self->range);
      break;

    case PROP_HUE:
      g_value_set_double (value, self->hue[self->range]);
      break;

    case PROP_SATURATION:
      g_value_set_double (value, self->saturation[self->range]);
      break;

    case PROP_LIGHTNESS:
      g_value_set_double (value, self->lightness[self->range]);
      break;

    case PROP_OVERLAP:
      g_value_set_double (value, self->overlap);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_hue_saturation_config_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  GimpHueSaturationConfig *self = GIMP_HUE_SATURATION_CONFIG (object);

  switch (property_id)
    {
    case PROP_RANGE:
      self->range = g_value_get_enum (value);
      break;

    case PROP_HUE:
      self->hue[self->range] = g_value_get_double (value);
      break;

    case PROP_SATURATION:
      self->saturation[self->range] = g_value_get_double (value);
      break;

    case PROP_LIGHTNESS:
      self->lightness[self->range] = g_value_get_double (value);
      break;

    case PROP_OVERLAP:
      self->overlap = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

void
gimp_hue_saturation_config_reset (GimpHueSaturationConfig *config)
{
  GimpHueRange range;

  g_return_if_fail (GIMP_IS_HUE_SATURATION_CONFIG (config));

  config->range = GIMP_ALL_HUES;

  for (range = GIMP_ALL_HUES; range <= GIMP_MAGENTA_HUES; range++)
    {
      gimp_hue_saturation_config_reset_range (config, range);
    }

  config->overlap = 0.0;
}

void
gimp_hue_saturation_config_reset_range (GimpHueSaturationConfig *config,
                                        GimpHueRange             range)
{
  g_return_if_fail (GIMP_IS_HUE_SATURATION_CONFIG (config));

  config->hue[range]        = 0.0;
  config->saturation[range] = 0.0;
  config->lightness[range]  = 0.0;
}


/*  temp cruft  */

void
gimp_hue_saturation_config_to_cruft (GimpHueSaturationConfig *config,
                                     HueSaturation           *hs)
{
  GimpHueRange range;

  g_return_if_fail (GIMP_IS_HUE_SATURATION_CONFIG (config));
  g_return_if_fail (hs != NULL);

  for (range = GIMP_ALL_HUES; range <= GIMP_MAGENTA_HUES; range++)
    {
      hs->hue[range]        = config->hue[range]        * 180;
      hs->saturation[range] = config->saturation[range] * 100;
      hs->lightness[range]  = config->lightness[range]  * 100;
    }

  hs->overlap = config->overlap * 100;
}
