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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "operations-types.h"

#include "gimpcolorbalanceconfig.h"
#include "gimpoperationcolorbalance.h"

#include "gimp-intl.h"


static gboolean gimp_operation_color_balance_process (GeglOperation       *operation,
                                                      void                *in_buf,
                                                      void                *out_buf,
                                                      glong                samples,
                                                      const GeglRectangle *roi,
                                                      gint                 level);


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

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:color-balance",
                                 "categories",  "color",
                                 "description", _("Adjust color distribution"),
                                 NULL);

  point_class->process = gimp_operation_color_balance_process;

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
gimp_operation_color_balance_process (GeglOperation       *operation,
                                      void                *in_buf,
                                      void                *out_buf,
                                      glong                samples,
                                      const GeglRectangle *roi,
                                      gint                 level)
{
  GimpOperationPointFilter *point  = GIMP_OPERATION_POINT_FILTER (operation);
  GimpColorBalanceConfig   *config = GIMP_COLOR_BALANCE_CONFIG (point->config);
  gfloat                   *src    = in_buf;
  gfloat                   *dest   = out_buf;
  gint                      total;
  const Babl               *space;
  const Babl               *format;
  const Babl               *hsl_format;
  gfloat                   *hsl;
  gfloat                   *rgb_n;
  gint                      i;

  if (! config)
    return FALSE;

  total      = samples * 4;
  format     = gegl_operation_get_format (operation, "input");
  space      = gegl_operation_get_source_space (operation, "input");
  hsl_format = babl_format_with_space ("HSLA float", space);

  hsl   = g_new0 (gfloat, sizeof (gfloat) * total);
  rgb_n = g_new0 (gfloat, sizeof (gfloat) * total);

  /* Create HSL buffer */
  babl_process (babl_fish (format, hsl_format), src, hsl, samples);

  /* Create normalized RGB buffer */
  for (i = 0; i < total; i += 4)
    {
      rgb_n[i] =
        gimp_operation_color_balance_map (src[i], hsl[i + 2],
                                          config->cyan_red[GIMP_TRANSFER_SHADOWS],
                                          config->cyan_red[GIMP_TRANSFER_MIDTONES],
                                          config->cyan_red[GIMP_TRANSFER_HIGHLIGHTS]);

      rgb_n[i + 1] =
        gimp_operation_color_balance_map (src[i + 1], hsl[i + 2],
                                          config->magenta_green[GIMP_TRANSFER_SHADOWS],
                                          config->magenta_green[GIMP_TRANSFER_MIDTONES],
                                          config->magenta_green[GIMP_TRANSFER_HIGHLIGHTS]);

      rgb_n[i + 2] =
        gimp_operation_color_balance_map (src[i + 2], hsl[i + 2],
                                          config->yellow_blue[GIMP_TRANSFER_SHADOWS],
                                          config->yellow_blue[GIMP_TRANSFER_MIDTONES],
                                          config->yellow_blue[GIMP_TRANSFER_HIGHLIGHTS]);

      rgb_n[i + 3] = src[i + 3];
    }

  if (config->preserve_luminosity)
    {
      gfloat *hsl_n = g_new0 (gfloat, sizeof (gfloat) * total);

      babl_process (babl_fish (format, hsl_format), rgb_n, hsl_n, samples);

      /* Copy the original lightness value */
      for (i = 0; i < total; i += 4)
        hsl_n[i + 2] = hsl[i + 2];

      babl_process (babl_fish (hsl_format, format), hsl_n, rgb_n, samples);
      g_free (hsl_n);
    }

  /* Copy normalized RGB back to destination */
  for (i = 0; i < total; i++)
    dest[i] = rgb_n[i];

  g_free (hsl);
  g_free (rgb_n);

  return TRUE;
}
