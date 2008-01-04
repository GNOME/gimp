/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationcolorize.c
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

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "gegl/gegl-types.h"
#include <gegl/buffer/gegl-buffer.h>

#include "gegl-types.h"

#include "gimpoperationcolorize.h"


enum
{
  PROP_0,
  PROP_HUE,
  PROP_SATURATION,
  PROP_LIGHTNESS
};


static void     gimp_operation_colorize_get_property (GObject       *object,
                                                      guint          property_id,
                                                      GValue        *value,
                                                      GParamSpec    *pspec);
static void     gimp_operation_colorize_set_property (GObject       *object,
                                                      guint          property_id,
                                                      const GValue  *value,
                                                      GParamSpec    *pspec);

static gboolean gimp_operation_colorize_process      (GeglOperation *operation,
                                                      void          *in_buf,
                                                      void          *out_buf,
                                                      glong          samples);


G_DEFINE_TYPE (GimpOperationColorize, gimp_operation_colorize,
               GEGL_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_colorize_parent_class


static void
gimp_operation_colorize_class_init (GimpOperationColorizeClass * klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property = gimp_operation_colorize_set_property;
  object_class->get_property = gimp_operation_colorize_get_property;

  point_class->process       = gimp_operation_colorize_process;

  gegl_operation_class_set_name (operation_class, "gimp-colorize");

  g_object_class_install_property (object_class,
                                   PROP_HUE,
                                   g_param_spec_float ("hue",
                                                       "Hue",
                                                       "Hue",
                                                       0.0, 360.0, 180.0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_SATURATION,
                                   g_param_spec_float ("saturation",
                                                       "Saturation",
                                                       "Saturation",
                                                       0.0, 100.0, 50.0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_LIGHTNESS,
                                   g_param_spec_float ("lightness",
                                                       "Lightness",
                                                       "Lightness",
                                                       -100.0, 100.0, 0.0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));
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
      g_value_set_float (value, self->hue);
      break;

    case PROP_SATURATION:
      g_value_set_float (value, self->saturation);
      break;

    case PROP_LIGHTNESS:
      g_value_set_float (value, self->lightness);
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
      self->hue = g_value_get_float (value);
      break;

    case PROP_SATURATION:
      self->saturation = g_value_get_float (value);
      break;

    case PROP_LIGHTNESS:
      self->lightness = g_value_get_float (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_operation_colorize_process (GeglOperation *operation,
                                 void          *in_buf,
                                 void          *out_buf,
                                 glong          samples)
{
  GimpOperationColorize *self = GIMP_OPERATION_COLORIZE (operation);
  gfloat                *src  = in_buf;
  gfloat                *dest = out_buf;
  glong                  sample;

  for (sample = 0; sample < samples; sample++)
    {
      GimpHSL hsl;
      GimpRGB rgb;
      gfloat  lum = GIMP_RGB_LUMINANCE (src[RED_PIX],
                                        src[GREEN_PIX],
                                        src[BLUE_PIX]);

      if (self->lightness > 0)
        {
          lum = lum * (100.0 - self->lightness) / 100.0;

          lum += 1.0 - (100.0 - self->lightness) / 100.0;
        }
      else if (self->lightness < 0)
        {
          lum = lum * (self->lightness + 100.0) / 100.0;
        }

      hsl.h = self->hue        / 360.0;
      hsl.s = self->saturation / 100.0;
      hsl.l = lum;

      gimp_hsl_to_rgb (&hsl, &rgb);

      dest[RED_PIX]   = rgb.r;
      dest[GREEN_PIX] = rgb.g;
      dest[BLUE_PIX]  = rgb.b;
      dest[ALPHA_PIX] = src[ALPHA_PIX];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
