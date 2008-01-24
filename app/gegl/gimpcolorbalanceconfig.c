/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcolorbalanceconfig.c
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

/*  temp cruft  */
#include "base/color-balance.h"

#include "gimpcolorbalanceconfig.h"


enum
{
  PROP_0,
  PROP_RANGE,
  PROP_CYAN_RED,
  PROP_MAGENTA_GREEN,
  PROP_YELLOW_BLUE,
  PROP_PRESERVE_LUMINOSITY
};


static void   gimp_color_balance_config_get_property (GObject      *object,
                                                      guint         property_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);
static void   gimp_color_balance_config_set_property (GObject      *object,
                                                      guint         property_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);


G_DEFINE_TYPE (GimpColorBalanceConfig, gimp_color_balance_config,
               G_TYPE_OBJECT)

#define parent_class gimp_color_balance_config_parent_class


static void
gimp_color_balance_config_class_init (GimpColorBalanceConfigClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_color_balance_config_set_property;
  object_class->get_property = gimp_color_balance_config_get_property;

  g_object_class_install_property (object_class, PROP_RANGE,
                                   g_param_spec_enum ("range",
                                                      "range",
                                                      "The affected range",
                                                      GIMP_TYPE_TRANSFER_MODE,
                                                      GIMP_MIDTONES,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CYAN_RED,
                                   g_param_spec_double ("cyan-red",
                                                        "Cyan-Red",
                                                        "Cyan-Red",
                                                        -1.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_MAGENTA_GREEN,
                                   g_param_spec_double ("magenta-green",
                                                        "Magenta-Green",
                                                        "Magenta-Green",
                                                        -1.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_YELLOW_BLUE,
                                   g_param_spec_double ("yellow-blue",
                                                        "Yellow-Blue",
                                                        "Yellow-Blue",
                                                        -1.0, 1.0, 1.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PRESERVE_LUMINOSITY,
                                   g_param_spec_boolean ("preserve-luminosity",
                                                         "Preserve Luminosity",
                                                         "Preserve Luminosity",
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_color_balance_config_init (GimpColorBalanceConfig *self)
{
  gimp_color_balance_config_reset (self);
}

static void
gimp_color_balance_config_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpColorBalanceConfig *self = GIMP_COLOR_BALANCE_CONFIG (object);

  switch (property_id)
    {
    case PROP_RANGE:
      g_value_set_enum (value, self->range);
      break;

    case PROP_CYAN_RED:
      g_value_set_double (value, self->cyan_red[self->range]);
      break;

    case PROP_MAGENTA_GREEN:
      g_value_set_double (value, self->magenta_green[self->range]);
      break;

    case PROP_YELLOW_BLUE:
      g_value_set_double (value, self->yellow_blue[self->range]);
      break;

    case PROP_PRESERVE_LUMINOSITY:
      g_value_set_boolean (value, self->preserve_luminosity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_balance_config_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpColorBalanceConfig *self = GIMP_COLOR_BALANCE_CONFIG (object);

  switch (property_id)
    {
    case PROP_RANGE:
      self->range = g_value_get_enum (value);
      break;

    case PROP_CYAN_RED:
      self->cyan_red[self->range] = g_value_get_double (value);
      break;

    case PROP_MAGENTA_GREEN:
      self->magenta_green[self->range] = g_value_get_double (value);
      break;

    case PROP_YELLOW_BLUE:
      self->yellow_blue[self->range] = g_value_get_double (value);
      break;

    case PROP_PRESERVE_LUMINOSITY:
      self->preserve_luminosity = g_value_get_boolean (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

void
gimp_color_balance_config_reset (GimpColorBalanceConfig *config)
{
  GimpTransferMode range;

  g_return_if_fail (GIMP_IS_COLOR_BALANCE_CONFIG (config));

  config->range = GIMP_MIDTONES;

  for (range = GIMP_SHADOWS; range <= GIMP_HIGHLIGHTS; range++)
    {
      gimp_color_balance_config_reset_range (config, range);
    }

  config->preserve_luminosity = TRUE;
}

void
gimp_color_balance_config_reset_range (GimpColorBalanceConfig *config,
                                       GimpTransferMode        range)
{
  g_return_if_fail (GIMP_IS_COLOR_BALANCE_CONFIG (config));

  config->cyan_red[range]      = 0.0;
  config->magenta_green[range] = 0.0;
  config->yellow_blue[range]   = 0.0;
}


/*  temp cruft  */

void
gimp_color_balance_config_to_cruft (GimpColorBalanceConfig *config,
                                    ColorBalance           *cruft)
{
  GimpTransferMode range;

  g_return_if_fail (GIMP_IS_COLOR_BALANCE_CONFIG (config));
  g_return_if_fail (cruft != NULL);

  for (range = GIMP_SHADOWS; range <= GIMP_HIGHLIGHTS; range++)
    {
      cruft->cyan_red[range]      = config->cyan_red[range]      * 100.0;
      cruft->magenta_green[range] = config->magenta_green[range] * 100.0;
      cruft->yellow_blue[range]   = config->yellow_blue[range]   * 100.0;
    }

  cruft->preserve_luminosity = config->preserve_luminosity;

  color_balance_create_lookup_tables (cruft);
}
