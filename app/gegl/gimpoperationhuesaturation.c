/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationhuesaturation.c
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

#include "gimpoperationhuesaturation.h"


enum
{
  PROP_0,
  PROP_RANGE,
  PROP_HUE,
  PROP_SATURATION,
  PROP_LIGHTNESS,
  PROP_OVERLAP
};


static void     gimp_operation_hue_saturation_get_property (GObject       *object,
                                                            guint          property_id,
                                                            GValue        *value,
                                                            GParamSpec    *pspec);
static void     gimp_operation_hue_saturation_set_property (GObject       *object,
                                                            guint          property_id,
                                                            const GValue  *value,
                                                            GParamSpec    *pspec);

static gboolean gimp_operation_hue_saturation_process      (GeglOperation *operation,
                                                            void          *in_buf,
                                                            void          *out_buf,
                                                            glong          samples);


G_DEFINE_TYPE (GimpOperationHueSaturation, gimp_operation_hue_saturation,
               GEGL_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_hue_saturation_parent_class


static void
gimp_operation_hue_saturation_class_init (GimpOperationHueSaturationClass * klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property = gimp_operation_hue_saturation_set_property;
  object_class->get_property = gimp_operation_hue_saturation_get_property;

  point_class->process       = gimp_operation_hue_saturation_process;

  gegl_operation_class_set_name (operation_class, "gimp-hue-saturation");

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
gimp_operation_hue_saturation_init (GimpOperationHueSaturation *self)
{
  GimpHueRange range;

  self->range = GIMP_ALL_HUES;

  for (range = GIMP_ALL_HUES; range <= GIMP_MAGENTA_HUES; range++)
    {
      self->hue[range]        = 0.0;
      self->saturation[range] = 0.0;
      self->lightness[range]  = 0.0;
    }

  self->overlap = 0.0;
}

static void
gimp_operation_hue_saturation_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  GimpOperationHueSaturation *self = GIMP_OPERATION_HUE_SATURATION (object);

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
gimp_operation_hue_saturation_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  GimpOperationHueSaturation *self = GIMP_OPERATION_HUE_SATURATION (object);

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

static inline gdouble
map_hue (GimpOperationHueSaturation *self,
         gint                        hue,
         gdouble                     value)
{
  value += (self->hue[0] + self->hue[hue + 1]) / 2.0;

  if (value < 0)
    return value + 1.0;
  else if (value > 1.0)
    return value - 1.0;
  else
    return value;
}

static inline gdouble
map_saturation (GimpOperationHueSaturation *self,
                gint                        hue,
                gdouble                     value)
{
  gdouble v = self->saturation[0] + self->saturation[hue + 1];

  // v = CLAMP (v, -1.0, 1.0);

  /* This change affects the way saturation is computed. With the old
   * code (different code for value < 0), increasing the saturation
   * affected muted colors very much, and bright colors less. With the
   * new code, it affects muted colors and bright colors more or less
   * evenly. For enhancing the color in photos, the new behavior is
   * exactly what you want. It's hard for me to imagine a case in
   * which the old behavior is better.
  */
  value *= (v + 1.0);

  return CLAMP (value, 0.0, 1.0);
}

static inline gdouble
map_lightness (GimpOperationHueSaturation *self,
               gint                        hue,
               gdouble                     value)
{
  gdouble v = (self->lightness[0] + self->lightness[hue + 1]) / 2.0;

  // v = CLAMP (v, -1.0, 1.0);

  if (v < 0)
    return value * (v + 1.0);
  else
    return value + (v * (1.0 - value));
}

static gboolean
gimp_operation_hue_saturation_process (GeglOperation *operation,
                                       void          *in_buf,
                                       void          *out_buf,
                                       glong          samples)
{
  GimpOperationHueSaturation *self = GIMP_OPERATION_HUE_SATURATION (operation);
  gfloat                     *src  = in_buf;
  gfloat                     *dest = out_buf;
  glong                       sample;

  for (sample = 0; sample < samples; sample++)
    {
      GimpRGB  rgb;
      GimpHSL  hsl;
      gdouble  h;
      gint     hue_counter;
      gint     hue                 = 0;
      gint     secondary_hue       = 0;
      gboolean use_secondary_hue   = FALSE;
      gfloat   primary_intensity   = 0.0;
      gfloat   secondary_intensity = 0.0;
      gfloat   overlap             = self->overlap / 2.0;

      rgb.r = src[RED_PIX];
      rgb.g = src[GREEN_PIX];
      rgb.b = src[BLUE_PIX];

      gimp_rgb_to_hsl (&rgb, &hsl);

      h = hsl.h * 6.0;

      for (hue_counter = 0; hue_counter < 7; hue_counter++)
        {
          gdouble hue_threshold = (gdouble) hue_counter + 0.5;

          if (h < ((gdouble) hue_threshold + overlap))
            {
              hue = hue_counter;

              if (overlap > 0.0 && h > ((gdouble) hue_threshold - overlap))
                {
                  use_secondary_hue = TRUE;

                  secondary_hue = hue_counter + 1;

                  secondary_intensity =
                    (h - (gdouble) hue_threshold + overlap) / (2.0 * overlap);

                  primary_intensity = 1.0 - secondary_intensity;
                }
              else
                {
                  use_secondary_hue = FALSE;
                }

              break;
            }
        }

      if (hue >= 6)
        {
          hue = 0;
          use_secondary_hue = FALSE;
        }

      if (secondary_hue >= 6)
        {
          secondary_hue = 0;
        }

      if (use_secondary_hue)
        {
          hsl.h = (map_hue        (self, hue,           hsl.h) * primary_intensity +
                   map_hue        (self, secondary_hue, hsl.h) * secondary_intensity);

          hsl.s = (map_saturation (self, hue,           hsl.s) * primary_intensity +
                   map_saturation (self, secondary_hue, hsl.s) * secondary_intensity);

          hsl.l = (map_lightness  (self, hue,           hsl.l) * primary_intensity +
                   map_lightness  (self, secondary_hue, hsl.l) * secondary_intensity);
        }
      else
        {
          hsl.h = map_hue        (self, hue, hsl.h);
          hsl.s = map_saturation (self, hue, hsl.s);
          hsl.l = map_lightness  (self, hue, hsl.l);
        }

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
