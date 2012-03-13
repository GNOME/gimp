/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcolorizeconfig.c
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

#include <gegl.h>

#include "libgimpconfig/gimpconfig.h"

#include "gimp-gegl-types.h"

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
                         GIMP_TYPE_IMAGE_MAP_CONFIG,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

#define parent_class gimp_colorize_config_parent_class


static void
gimp_colorize_config_class_init (GimpColorizeConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->set_property       = gimp_colorize_config_set_property;
  object_class->get_property       = gimp_colorize_config_get_property;

  viewable_class->default_stock_id = "gimp-tool-colorize";

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_HUE,
                                   "hue",
                                   "Hue",
                                   0.0, 1.0, 0.5, 0);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_SATURATION,
                                   "saturation",
                                   "Saturation",
                                   0.0, 1.0, 0.5, 0);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_LIGHTNESS,
                                   "lightness",
                                   "Lightness",
                                   -1.0, 1.0, 0.0, 0);
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
