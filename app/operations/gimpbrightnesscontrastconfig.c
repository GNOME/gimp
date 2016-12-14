/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrightnesscontrastconfig.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "operations-types.h"

#include "gimpbrightnesscontrastconfig.h"
#include "gimplevelsconfig.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_BRIGHTNESS,
  PROP_CONTRAST
};


static void     gimp_brightness_contrast_config_iface_init   (GimpConfigInterface *iface);

static void     gimp_brightness_contrast_config_get_property (GObject      *object,
                                                              guint         property_id,
                                                              GValue       *value,
                                                              GParamSpec   *pspec);
static void     gimp_brightness_contrast_config_set_property (GObject      *object,
                                                              guint         property_id,
                                                              const GValue *value,
                                                              GParamSpec   *pspec);

static gboolean gimp_brightness_contrast_config_equal        (GimpConfig   *a,
                                                              GimpConfig   *b);


G_DEFINE_TYPE_WITH_CODE (GimpBrightnessContrastConfig,
                         gimp_brightness_contrast_config,
                         GIMP_TYPE_SETTINGS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_brightness_contrast_config_iface_init))

#define parent_class gimp_brightness_contrast_config_parent_class


static void
gimp_brightness_contrast_config_class_init (GimpBrightnessContrastConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->set_property        = gimp_brightness_contrast_config_set_property;
  object_class->get_property        = gimp_brightness_contrast_config_get_property;

  viewable_class->default_icon_name = "gimp-tool-brightness-contrast";

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_BRIGHTNESS,
                           "brightness",
                           _("Brightness"),
                           _("Brightness"),
                           -1.0, 1.0, 0.0, 0);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_CONTRAST,
                           "contrast",
                           _("Contrast"),
                           _("Contrast"),
                           -1.0, 1.0, 0.0, 0);
}

static void
gimp_brightness_contrast_config_iface_init (GimpConfigInterface *iface)
{
  iface->equal = gimp_brightness_contrast_config_equal;
}

static void
gimp_brightness_contrast_config_init (GimpBrightnessContrastConfig *self)
{
}

static void
gimp_brightness_contrast_config_get_property (GObject    *object,
                                              guint       property_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  GimpBrightnessContrastConfig *self = GIMP_BRIGHTNESS_CONTRAST_CONFIG (object);

  switch (property_id)
    {
    case PROP_BRIGHTNESS:
      g_value_set_double (value, self->brightness);
      break;

    case PROP_CONTRAST:
      g_value_set_double (value, self->contrast);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_brightness_contrast_config_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  GimpBrightnessContrastConfig *self = GIMP_BRIGHTNESS_CONTRAST_CONFIG (object);

  switch (property_id)
    {
    case PROP_BRIGHTNESS:
      self->brightness = g_value_get_double (value);
      break;

    case PROP_CONTRAST:
      self->contrast = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_brightness_contrast_config_equal (GimpConfig *a,
                                       GimpConfig *b)
{
  GimpBrightnessContrastConfig *config_a = GIMP_BRIGHTNESS_CONTRAST_CONFIG (a);
  GimpBrightnessContrastConfig *config_b = GIMP_BRIGHTNESS_CONTRAST_CONFIG (b);

  if (config_a->brightness != config_b->brightness ||
      config_a->contrast   != config_b->contrast)
    {
      return FALSE;
    }

  return TRUE;
}


/*  public functions  */

GimpLevelsConfig *
gimp_brightness_contrast_config_to_levels_config (GimpBrightnessContrastConfig *config)
{
  GimpLevelsConfig *levels;
  gdouble           brightness;
  gdouble           slant;
  gdouble           value;

  g_return_val_if_fail (GIMP_IS_BRIGHTNESS_CONTRAST_CONFIG (config), NULL);

  levels = g_object_new (GIMP_TYPE_LEVELS_CONFIG, NULL);

  brightness = config->brightness / 2.0;
  slant = tan ((config->contrast + 1) * G_PI_4);

  if (config->brightness >= 0)
    {
      value = -0.18 * slant + brightness * slant + 0.18;

      if (value < 0.0)
        {
          value = 0.0;

          /* this slightly convoluted math follows by inverting the
           * calculation of the brightness/contrast LUT in base/lut-funcs.h */

          levels->low_input[GIMP_HISTOGRAM_VALUE] =
            (- brightness * slant + 0.18 * slant - 0.18) / (slant - brightness * slant);
        }

      levels->low_output[GIMP_HISTOGRAM_VALUE] = value;

      value = 0.82 * slant + 0.18;

      if (value > 1.0)
        {
          value = 1.0;

          levels->high_input[GIMP_HISTOGRAM_VALUE] =
            (- brightness * slant + 0.18 * slant + 0.82) / (slant - brightness * slant);
        }

      levels->high_output[GIMP_HISTOGRAM_VALUE] = value;
    }
  else
    {
      value = 0.18 - 0.18 * slant;

      if (value < 0.0)
        {
          value = 0.0;

          levels->low_input[GIMP_HISTOGRAM_VALUE] =
            (0.18 * slant - 0.18) / (slant + brightness * slant);
        }

      levels->low_output[GIMP_HISTOGRAM_VALUE] = value;

      value = slant * brightness + slant * 0.82 + 0.18;

      if (value > 1.0)
        {
          value = 1.0;

          levels->high_input[GIMP_HISTOGRAM_VALUE] =
            (0.18 * slant + 0.82) / (slant + brightness * slant);
        }

      levels->high_output[GIMP_HISTOGRAM_VALUE] = value;
    }

  return levels;
}
