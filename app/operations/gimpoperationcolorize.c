/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationcolorize.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "operations-types.h"

#include "gimpoperationcolorize.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_HUE,
  PROP_SATURATION,
  PROP_LIGHTNESS,
  PROP_COLOR
};


static void     gimp_operation_colorize_get_property (GObject      *object,
                                                      guint         property_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);
static void     gimp_operation_colorize_set_property (GObject      *object,
                                                      guint         property_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);

static gboolean gimp_operation_colorize_process (GeglOperation       *operation,
                                                 void                *in_buf,
                                                 void                *out_buf,
                                                 glong                samples,
                                                 const GeglRectangle *roi,
                                                 gint                 level);


G_DEFINE_TYPE (GimpOperationColorize, gimp_operation_colorize,
               GIMP_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_colorize_parent_class


static void
gimp_operation_colorize_class_init (GimpOperationColorizeClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);
  GimpHSL                        hsl;
  GimpRGB                        rgb;

  object_class->set_property   = gimp_operation_colorize_set_property;
  object_class->get_property   = gimp_operation_colorize_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:colorize",
                                 "categories",  "color",
                                 "description", _("Colorize the image"),
                                 NULL);

  point_class->process = gimp_operation_colorize_process;

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
                                                        _("Color"),
                                                        _("Color"),
                                                        FALSE, &rgb,
                                                        G_PARAM_READWRITE));
}

static void
gimp_operation_colorize_init (GimpOperationColorize *self)
{
}

static void
gimp_operation_colorize_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpOperationColorize *self = GIMP_OPERATION_COLORIZE (object);

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
gimp_operation_colorize_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpOperationColorize *self = GIMP_OPERATION_COLORIZE (object);

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
gimp_operation_colorize_process (GeglOperation       *operation,
                                 void                *in_buf,
                                 void                *out_buf,
                                 glong                samples,
                                 const GeglRectangle *roi,
                                 gint                 level)
{
  GimpOperationColorize *colorize  = GIMP_OPERATION_COLORIZE (operation);
  gfloat                *src    = in_buf;
  gfloat                *dest   = out_buf;
  GimpHSL                hsl;

  hsl.h = colorize->hue;
  hsl.s = colorize->saturation;

  while (samples--)
    {
      GimpRGB rgb;
      gfloat  lum = GIMP_RGB_LUMINANCE (src[RED],
                                        src[GREEN],
                                        src[BLUE]);

      if (colorize->lightness > 0)
        {
          lum = lum * (1.0 - colorize->lightness);

          lum += 1.0 - (1.0 - colorize->lightness);
        }
      else if (colorize->lightness < 0)
        {
          lum = lum * (colorize->lightness + 1.0);
        }

      hsl.l = lum;

      gimp_hsl_to_rgb (&hsl, &rgb);

      /*  the code in base/colorize.c would multiply r,b,g with lum,
       *  but this is a bug since it should multiply with 255. We
       *  don't repeat this bug here (this is the reason why the gegl
       *  colorize is brighter than the legacy one).
       */
      dest[RED]   = rgb.r; /* * lum */
      dest[GREEN] = rgb.g; /* * lum */
      dest[BLUE]  = rgb.b; /* * lum */
      dest[ALPHA] = src[ALPHA];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
