/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationcolorbalance.c
 * Copyright (C) 2007 Michael Natterer <mitch@ligma.org>
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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"

#include "operations-types.h"

#include "ligmacolorbalanceconfig.h"
#include "ligmaoperationcolorbalance.h"

#include "ligma-intl.h"


static gboolean ligma_operation_color_balance_process (GeglOperation       *operation,
                                                      void                *in_buf,
                                                      void                *out_buf,
                                                      glong                samples,
                                                      const GeglRectangle *roi,
                                                      gint                 level);


G_DEFINE_TYPE (LigmaOperationColorBalance, ligma_operation_color_balance,
               LIGMA_TYPE_OPERATION_POINT_FILTER)

#define parent_class ligma_operation_color_balance_parent_class


static void
ligma_operation_color_balance_class_init (LigmaOperationColorBalanceClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property   = ligma_operation_point_filter_set_property;
  object_class->get_property   = ligma_operation_point_filter_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "ligma:color-balance",
                                 "categories",  "color",
                                 "description", _("Adjust color distribution"),
                                 NULL);

  point_class->process = ligma_operation_color_balance_process;

  g_object_class_install_property (object_class,
                                   LIGMA_OPERATION_POINT_FILTER_PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "The config object",
                                                        LIGMA_TYPE_COLOR_BALANCE_CONFIG,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_operation_color_balance_init (LigmaOperationColorBalance *self)
{
}

static inline gfloat
ligma_operation_color_balance_map (gfloat  value,
                                  gdouble lightness,
                                  gdouble shadows,
                                  gdouble midtones,
                                  gdouble highlights)
{
  /* Apply masks to the corrections for shadows, midtones and
   * highlights so that each correction affects only one range.
   * Those masks look like this:
   *     ‾\___
   *     _/‾\_
   *     ___/‾
   * with ramps of width a at x = b and x = 1 - b.
   *
   * The sum of these masks equals 1 for x in 0..1, so applying the
   * same correction in the shadows and in the midtones is equivalent
   * to applying this correction on a virtual shadows_and_midtones
   * range.
   */
  static const gdouble a = 0.25, b = 0.333, scale = 0.7;

  shadows    *= CLAMP ((lightness - b) / -a + 0.5, 0, 1) * scale;
  midtones   *= CLAMP ((lightness - b) /  a + 0.5, 0, 1) *
                CLAMP ((lightness + b - 1) / -a + 0.5, 0, 1) * scale;
  highlights *= CLAMP ((lightness + b - 1) /  a + 0.5, 0, 1) * scale;

  value += shadows;
  value += midtones;
  value += highlights;
  value = CLAMP (value, 0.0, 1.0);

  return value;
}

static gboolean
ligma_operation_color_balance_process (GeglOperation       *operation,
                                      void                *in_buf,
                                      void                *out_buf,
                                      glong                samples,
                                      const GeglRectangle *roi,
                                      gint                 level)
{
  LigmaOperationPointFilter *point  = LIGMA_OPERATION_POINT_FILTER (operation);
  LigmaColorBalanceConfig   *config = LIGMA_COLOR_BALANCE_CONFIG (point->config);
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

      LigmaRGB rgb = { r, g, b};
      LigmaHSL hsl;

      ligma_rgb_to_hsl (&rgb, &hsl);

      r_n = ligma_operation_color_balance_map (r, hsl.l,
                                              config->cyan_red[LIGMA_TRANSFER_SHADOWS],
                                              config->cyan_red[LIGMA_TRANSFER_MIDTONES],
                                              config->cyan_red[LIGMA_TRANSFER_HIGHLIGHTS]);

      g_n = ligma_operation_color_balance_map (g, hsl.l,
                                              config->magenta_green[LIGMA_TRANSFER_SHADOWS],
                                              config->magenta_green[LIGMA_TRANSFER_MIDTONES],
                                              config->magenta_green[LIGMA_TRANSFER_HIGHLIGHTS]);

      b_n = ligma_operation_color_balance_map (b, hsl.l,
                                              config->yellow_blue[LIGMA_TRANSFER_SHADOWS],
                                              config->yellow_blue[LIGMA_TRANSFER_MIDTONES],
                                              config->yellow_blue[LIGMA_TRANSFER_HIGHLIGHTS]);

      if (config->preserve_luminosity)
        {
          LigmaHSL hsl2;

          rgb.r = r_n;
          rgb.g = g_n;
          rgb.b = b_n;
          ligma_rgb_to_hsl (&rgb, &hsl);

          rgb.r = r;
          rgb.g = g;
          rgb.b = b;
          ligma_rgb_to_hsl (&rgb, &hsl2);

          hsl.l = hsl2.l;

          ligma_hsl_to_rgb (&hsl, &rgb);

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
