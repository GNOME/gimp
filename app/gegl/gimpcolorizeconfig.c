/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcolorizeconfig.c
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

#include "libgimpconfig/gimpconfig.h"

#include "gegl-types.h"

/*  temp cruft  */
#include "base/colorize.h"

#include "gimpcolorizeconfig.h"


enum
{
  PROP_0,
  PROP_HUE,
  PROP_SATURATION,
  PROP_LIGHTNESS
};


static void   gimp_colorize_config_get_property (GObject      *object,
                                                 guint         property_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);
static void   gimp_colorize_config_set_property (GObject      *object,
                                                 guint         property_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpColorizeConfig, gimp_colorize_config,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

#define parent_class gimp_colorize_config_parent_class


static void
gimp_colorize_config_class_init (GimpColorizeConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_colorize_config_set_property;
  object_class->get_property = gimp_colorize_config_get_property;

  g_object_class_install_property (object_class, PROP_HUE,
                                   g_param_spec_double ("hue",
                                                        "Hue",
                                                        "Hue",
                                                        0.0, 1.0, 0.5,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SATURATION,
                                   g_param_spec_double ("saturation",
                                                        "Saturation",
                                                        "Saturation",
                                                        0.0, 1.0, 0.5,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LIGHTNESS,
                                   g_param_spec_double ("lightness",
                                                        "Lightness",
                                                        "Lightness",
                                                        -1.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_colorize_config_init (GimpColorizeConfig *self)
{
}

static void
gimp_colorize_config_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpColorizeConfig *self = GIMP_COLORIZE_CONFIG (object);

  switch (property_id)
    {
    case PROP_HUE:
      g_value_set_double (value, self->hue);
      break;

    case PROP_SATURATION:
      g_value_set_double (value, self->saturation);
      break;

    case PROP_LIGHTNESS:
      g_value_set_double (value, self->lightness);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_colorize_config_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpColorizeConfig *self = GIMP_COLORIZE_CONFIG (object);

  switch (property_id)
    {
    case PROP_HUE:
      self->hue = g_value_get_double (value);
      break;

    case PROP_SATURATION:
      self->saturation = g_value_get_double (value);
      break;

    case PROP_LIGHTNESS:
      self->lightness = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  temp cruft  */

void
gimp_colorize_config_to_cruft (GimpColorizeConfig *config,
                               Colorize           *cruft)
{
  g_return_if_fail (GIMP_IS_COLORIZE_CONFIG (config));
  g_return_if_fail (cruft != NULL);

  cruft->hue        = config->hue        * 360.0;
  cruft->saturation = config->saturation * 100.0;
  cruft->lightness  = config->lightness  * 100.0;

  colorize_calculate (cruft);
}
