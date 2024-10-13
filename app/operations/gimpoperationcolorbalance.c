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


static void     gimp_operation_color_balance_prepare (GeglOperation       *operation);
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

  operation_class->prepare = gimp_operation_color_balance_prepare;

  point_class->process     = gimp_operation_color_balance_process;

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
gimp_operation_color_balance_map (gfloat value,
                                  gfloat lightness,
                                  gfloat shadows,
                                  gfloat midtones,
                                  gfloat highlights)
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
  static const gfloat a = 0.25f, b = 0.333f, scale = 0.7f;

  shadows    *= CLAMP ((lightness - b) / -a + 0.5f, 0.f, 1.f) * scale;
  midtones   *= CLAMP ((lightness - b) /  a + 0.5f, 0.f, 1.f) *
                CLAMP ((lightness + b - 1.f) / -a + 0.5f, 0.f, 1.f) * scale;
  highlights *= CLAMP ((lightness + b - 1.f) /  a + 0.5f, 0.f, 1.f) * scale;

  value += shadows;
  value += midtones;
  value += highlights;
  /* We are working in sRGB while the image may have a bigger space so
   * this CLAMPing will reduce the target space. I'm not sure if
   * removing it would be right either.
   * This is something to verify and improve in a further version of
   * this operation. TODO.
   */
  value = CLAMP (value, 0.0f, 1.0f);

  return value;
}

static void
gimp_operation_color_balance_prepare (GeglOperation *operation)
{
  /* HSV conversion is much faster from sRGB. This version of the
   * algorithm will work on sRGB to its respective HSL space.
   * TODO: in the future, when babl will have fast conversion for other
   * HSV spaces, let's study if we should work in the source space.
   */
  const Babl *format = babl_format_with_space ("R'G'B'A float", NULL);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
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
  const Babl               *format;
  const Babl               *hsl_format;
  gfloat                   *hsl;
  gfloat                   *hsl_p;
  gfloat                    cyan_red_shadows;
  gfloat                    cyan_red_midtones;
  gfloat                    cyan_red_highlights;
  gfloat                    magenta_green_shadows;
  gfloat                    magenta_green_midtones;
  gfloat                    magenta_green_highlights;
  gfloat                    yellow_blue_shadows;
  gfloat                    yellow_blue_midtones;
  gfloat                    yellow_blue_highlights;
  gint                      i;

  if (! config)
    return FALSE;

  cyan_red_shadows         = (gfloat) config->cyan_red[GIMP_TRANSFER_SHADOWS];
  cyan_red_midtones        = (gfloat) config->cyan_red[GIMP_TRANSFER_MIDTONES];
  cyan_red_highlights      = (gfloat) config->cyan_red[GIMP_TRANSFER_HIGHLIGHTS];
  magenta_green_shadows    = (gfloat) config->magenta_green[GIMP_TRANSFER_SHADOWS];
  magenta_green_midtones   = (gfloat) config->magenta_green[GIMP_TRANSFER_MIDTONES];
  magenta_green_highlights = (gfloat) config->magenta_green[GIMP_TRANSFER_HIGHLIGHTS];
  yellow_blue_shadows      = (gfloat) config->yellow_blue[GIMP_TRANSFER_SHADOWS];
  yellow_blue_midtones     = (gfloat) config->yellow_blue[GIMP_TRANSFER_MIDTONES];
  yellow_blue_highlights   = (gfloat) config->yellow_blue[GIMP_TRANSFER_HIGHLIGHTS];

  total      = samples * 4;
  format     = gegl_operation_get_format (operation, "input");
  hsl_format = babl_format_with_space ("HSLA float", NULL);

  hsl = g_new0 (gfloat, total);

  /* Create HSL buffer */
  babl_process (babl_fish (format, hsl_format), in_buf, hsl, samples);

  /* Create normalized RGB buffer */
  hsl_p = hsl;
  for (i = 0; i < samples; i++)
    {
      gfloat lightness = *(hsl_p + 2);

      *dest =
        gimp_operation_color_balance_map (*src, lightness,
                                          cyan_red_shadows,
                                          cyan_red_midtones,
                                          cyan_red_highlights);
      *(dest + 1) =
        gimp_operation_color_balance_map (*(src + 1), lightness,
                                          magenta_green_shadows,
                                          magenta_green_midtones,
                                          magenta_green_highlights);

      *(dest + 2) =
        gimp_operation_color_balance_map (*(src + 2), lightness,
                                          yellow_blue_shadows,
                                          yellow_blue_midtones,
                                          yellow_blue_highlights);

      *(dest + 3) = *(src + 3);

      src   += 4;
      dest  += 4;
      hsl_p += 4;
    }

  if (config->preserve_luminosity)
    {
      gfloat *hsl2 = g_new0 (gfloat, total);
      gfloat *hsl2_p;

      babl_process (babl_fish (format, hsl_format), out_buf, hsl2, samples);

      /* Copy the original lightness value */
      hsl_p  = hsl;
      hsl2_p = hsl2;
      for (i = 0; i < samples; i++)
        {
          *(hsl2_p + 2) = *(hsl_p + 2);

          hsl_p  += 4;
          hsl2_p += 4;
        }

      babl_process (babl_fish (hsl_format, format), hsl2, out_buf, samples);
      g_free (hsl2);
    }

  g_free (hsl);

  return TRUE;
}
