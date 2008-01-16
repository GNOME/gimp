/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationcolorbalance.c
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

#include "gimpoperationcolorbalance.h"


enum
{
  PROP_0,
  PROP_RANGE,
  PROP_CYAN_RED,
  PROP_MAGENTA_GREEN,
  PROP_YELLOW_BLUE,
  PROP_PRESERVE_LUMINOSITY
};


static void     gimp_operation_color_balance_get_property (GObject       *object,
                                                           guint          property_id,
                                                           GValue        *value,
                                                           GParamSpec    *pspec);
static void     gimp_operation_color_balance_set_property (GObject       *object,
                                                           guint          property_id,
                                                           const GValue  *value,
                                                           GParamSpec    *pspec);

static gboolean gimp_operation_color_balance_process      (GeglOperation *operation,
                                                           void          *in_buf,
                                                           void          *out_buf,
                                                           glong          samples);


G_DEFINE_TYPE (GimpOperationColorBalance, gimp_operation_color_balance,
               GEGL_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_color_balance_parent_class


static void
gimp_operation_color_balance_class_init (GimpOperationColorBalanceClass * klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property = gimp_operation_color_balance_set_property;
  object_class->get_property = gimp_operation_color_balance_get_property;

  point_class->process       = gimp_operation_color_balance_process;

  gegl_operation_class_set_name (operation_class, "gimp-color-balance");

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
gimp_operation_color_balance_init (GimpOperationColorBalance *self)
{
  GimpTransferMode range;

  self->range = GIMP_MIDTONES;

  for (range = GIMP_SHADOWS; range <= GIMP_HIGHLIGHTS; range++)
    {
      self->cyan_red[range]      = 0.0;
      self->magenta_green[range] = 0.0;
      self->yellow_blue[range]   = 0.0;
    }

  self->preserve_luminosity = TRUE;
}

static void
gimp_operation_color_balance_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  GimpOperationColorBalance *self = GIMP_OPERATION_COLOR_BALANCE (object);

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
gimp_operation_color_balance_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  GimpOperationColorBalance *self = GIMP_OPERATION_COLOR_BALANCE (object);

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

static inline gfloat
gimp_operation_color_balance_map (gfloat  value,
                                  gdouble shadows,
                                  gdouble midtones,
                                  gdouble highlights)
{
  gdouble low = 1.075 - 1.0 / (value / 16.0 + 1.0);
  gdouble mid = 0.667 * (1.0 - SQR (value - 0.5));
  gdouble shadows_add;
  gdouble shadows_sub;
  gdouble midtones_add;
  gdouble midtones_sub;
  gdouble highlights_add;
  gdouble highlights_sub;

  shadows_add    = low + 1.0;
  shadows_sub    = 1.0 - low;

  midtones_add   = mid;
  midtones_sub   = mid;

  highlights_add = 1.0 - low;
  highlights_sub = low + 1.0;

  value += shadows * (shadows > 0 ? shadows_add : shadows_sub);
  value = CLAMP (value, 0.0, 1.0);

  value += midtones * (midtones > 0 ? midtones_add : midtones_sub);
  value = CLAMP (value, 0.0, 1.0);

  value += highlights * (highlights > 0 ? highlights_add : highlights_sub);
  value = CLAMP (value, 0.0, 1.0);

  return value;
}

static gboolean
gimp_operation_color_balance_process (GeglOperation *operation,
                                      void          *in_buf,
                                      void          *out_buf,
                                      glong          samples)
{
  GimpOperationColorBalance *self = GIMP_OPERATION_COLOR_BALANCE (operation);
  gfloat                    *src  = in_buf;
  gfloat                    *dest = out_buf;
  glong                      sample;

  for (sample = 0; sample < samples; sample++)
    {
      gfloat r = src[RED_PIX];
      gfloat g = src[GREEN_PIX];
      gfloat b = src[BLUE_PIX];
      gfloat r_n;
      gfloat g_n;
      gfloat b_n;

      r_n = gimp_operation_color_balance_map (r,
                                              self->cyan_red[GIMP_SHADOWS],
                                              self->cyan_red[GIMP_MIDTONES],
                                              self->cyan_red[GIMP_HIGHLIGHTS]);

      g_n = gimp_operation_color_balance_map (g,
                                              self->magenta_green[GIMP_SHADOWS],
                                              self->magenta_green[GIMP_MIDTONES],
                                              self->magenta_green[GIMP_HIGHLIGHTS]);

      b_n = gimp_operation_color_balance_map (b,
                                              self->yellow_blue[GIMP_SHADOWS],
                                              self->yellow_blue[GIMP_MIDTONES],
                                              self->yellow_blue[GIMP_HIGHLIGHTS]);

      if (self->preserve_luminosity)
        {
          GimpRGB rgb;
          GimpHSL hsl;
          GimpHSL hsl2;

          rgb.r = r_n;
          rgb.g = g_n;
          rgb.b = b_n;
          gimp_rgb_to_hsl (&rgb, &hsl);

          rgb.r = r;
          rgb.g = g;
          rgb.b = b;
          gimp_rgb_to_hsl (&rgb, &hsl2);

          hsl.l = hsl2.l;

          gimp_hsl_to_rgb (&hsl, &rgb);

          r_n = rgb.r;
          g_n = rgb.g;
          b_n = rgb.b;
        }

      dest[RED_PIX]   = r_n;
      dest[GREEN_PIX] = g_n;
      dest[BLUE_PIX]  = b_n;
      dest[ALPHA_PIX] = src[ALPHA_PIX];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
