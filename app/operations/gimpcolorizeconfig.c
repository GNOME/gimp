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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "operations-types.h"

#include "gimpcolorizeconfig.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_HUE,
  PROP_SATURATION,
  PROP_LIGHTNESS,
  PROP_COLOR
};


static void     gimp_colorize_config_iface_init   (GimpConfigInterface *iface);

static void     gimp_colorize_config_get_property (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);
static void     gimp_colorize_config_set_property (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);

static gboolean gimp_colorize_config_equal        (GimpConfig   *a,
                                                   GimpConfig   *b);


G_DEFINE_TYPE_WITH_CODE (GimpColorizeConfig, gimp_colorize_config,
                         GIMP_TYPE_SETTINGS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_colorize_config_iface_init))

#define parent_class gimp_colorize_config_parent_class


static void
gimp_colorize_config_class_init (GimpColorizeConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);
  GimpHSL            hsl;
  GimpRGB            rgb;

  object_class->set_property        = gimp_colorize_config_set_property;
  object_class->get_property        = gimp_colorize_config_get_property;

  viewable_class->default_icon_name = "gimp-tool-colorize";

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_HUE,
                           "hue",
                           _("Hue"),
                           _("Hue"),
                           0.0, 1.0, 0.5, 0);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_SATURATION,
                           "saturation",
                           _("Saturation"),
                           _("Saturation"),
                           0.0, 1.0, 0.5, 0);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_LIGHTNESS,
                           "lightness",
                           _("Lightness"),
                           _("Lightness"),
                           -1.0, 1.0, 0.0, 0);

  gimp_hsl_set (&hsl, 0.5, 0.5, 0.5);
  gimp_hsl_set_alpha (&hsl, 1.0);
  gimp_hsl_to_rgb (&hsl, &rgb);

  g_object_class_install_property (object_class, PROP_COLOR,
                                   gimp_param_spec_rgb ("color",
                                                        "Color",
                                                        "The color",
                                                        FALSE, &rgb,
                                                        G_PARAM_READWRITE));
}

static void
gimp_colorize_config_iface_init (GimpConfigInterface *iface)
{
  iface->equal = gimp_colorize_config_equal;
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

    case PROP_COLOR:
      {
        GimpHSL hsl;
        GimpRGB rgb;

        gimp_hsl_set (&hsl,
                      self->hue,
                      self->saturation,
                      (self->lightness + 1.0) / 2.0);
        gimp_hsl_set_alpha (&hsl, 1.0);
        gimp_hsl_to_rgb (&hsl, &rgb);
        gimp_value_set_rgb (value, &rgb);
      }
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
      g_object_notify (object, "color");
      break;

    case PROP_SATURATION:
      self->saturation = g_value_get_double (value);
      g_object_notify (object, "color");
      break;

    case PROP_LIGHTNESS:
      self->lightness = g_value_get_double (value);
      g_object_notify (object, "color");
      break;

    case PROP_COLOR:
      {
        GimpRGB rgb;
        GimpHSL hsl;

        gimp_value_get_rgb (value, &rgb);
        gimp_rgb_to_hsl (&rgb, &hsl);

        if (hsl.h == -1)
          hsl.h = self->hue;

        if (hsl.l == 0.0 || hsl.l == 1.0)
          hsl.s = self->saturation;

        g_object_set (self,
                      "hue",        hsl.h,
                      "saturation", hsl.s,
                      "lightness",  hsl.l * 2.0 - 1.0,
                      NULL);
      }
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_colorize_config_equal (GimpConfig *a,
                            GimpConfig *b)
{
  GimpColorizeConfig *config_a = GIMP_COLORIZE_CONFIG (a);
  GimpColorizeConfig *config_b = GIMP_COLORIZE_CONFIG (b);

  if (config_a->hue        != config_b->hue        ||
      config_a->saturation != config_b->saturation ||
      config_a->lightness  != config_b->lightness)
    {
      return FALSE;
    }

  return TRUE;
}

