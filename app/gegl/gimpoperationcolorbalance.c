/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationcolorbalance.c
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "gimpcolorbalanceconfig.h"
#include "gimpoperationcolorbalance.h"


static gboolean gimp_operation_color_balance_process (GeglOperation       *operation,
                                                      void                *in_buf,
                                                      void                *out_buf,
                                                      glong                samples,
                                                      const GeglRectangle *roi);


G_DEFINE_TYPE (GimpOperationColorBalance, gimp_operation_color_balance,
               GIMP_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_color_balance_parent_class


static void
gimp_operation_color_balance_class_init (GimpOperationColorBalanceClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property   = gimp_operation_point_filter_set_property;
  object_class->get_property   = gimp_operation_point_filter_get_property;

  operation_class->name        = "gimp:color-balance";
  operation_class->categories  = "color";
  operation_class->description = "GIMP Color Balance operation";

  point_class->process         = gimp_operation_color_balance_process;

  g_object_class_install_property (object_class,
                                   GIMP_OPERATION_POINT_FILTER_PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "The config object",
                                                        GIMP_TYPE_COLOR_BALANCE_CONFIG,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_operation_color_balance_init (GimpOperationColorBalance *self)
{
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
gimp_operation_color_balance_process (GeglOperation       *operation,
                                      void                *in_buf,
                                      void                *out_buf,
                                      glong                samples,
                                      const GeglRectangle *roi)
{
  GimpOperationPointFilter *point  = GIMP_OPERATION_POINT_FILTER (operation);
  GimpColorBalanceConfig   *config = GIMP_COLOR_BALANCE_CONFIG (point->config);
  gfloat                   *src    = in_buf;
  gfloat                   *dest   = out_buf;

  if (! config)
    return FALSE;

  while (samples--)
    {
      gfloat r = src[RED];
      gfloat g = src[GREEN];
      gfloat b = src[BLUE];
      gfloat r_n;
      gfloat g_n;
      gfloat b_n;

      r_n = gimp_operation_color_balance_map (r,
                                              config->cyan_red[GIMP_SHADOWS],
                                              config->cyan_red[GIMP_MIDTONES],
                                              config->cyan_red[GIMP_HIGHLIGHTS]);

      g_n = gimp_operation_color_balance_map (g,
                                              config->magenta_green[GIMP_SHADOWS],
                                              config->magenta_green[GIMP_MIDTONES],
                                              config->magenta_green[GIMP_HIGHLIGHTS]);

      b_n = gimp_operation_color_balance_map (b,
                                              config->yellow_blue[GIMP_SHADOWS],
                                              config->yellow_blue[GIMP_MIDTONES],
                                              config->yellow_blue[GIMP_HIGHLIGHTS]);

      if (config->preserve_luminosity)
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

      dest[RED]   = r_n;
      dest[GREEN] = g_n;
      dest[BLUE]  = b_n;
      dest[ALPHA] = src[ALPHA];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
