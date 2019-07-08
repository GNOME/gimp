/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationhuesaturation.c
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

#include "gimphuesaturationconfig.h"
#include "gimpoperationhuesaturation.h"

#include "gimp-intl.h"


static gboolean gimp_operation_hue_saturation_process (GeglOperation       *operation,
                                                       void                *in_buf,
                                                       void                *out_buf,
                                                       glong                samples,
                                                       const GeglRectangle *roi,
                                                       gint                 level);


G_DEFINE_TYPE (GimpOperationHueSaturation, gimp_operation_hue_saturation,
               GIMP_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_hue_saturation_parent_class


static void
gimp_operation_hue_saturation_class_init (GimpOperationHueSaturationClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property   = gimp_operation_point_filter_set_property;
  object_class->get_property   = gimp_operation_point_filter_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:hue-saturation",
                                 "categories",  "color",
                                 "description", _("Adjust hue, saturation, and lightness"),
                                 NULL);

  point_class->process = gimp_operation_hue_saturation_process;

  g_object_class_install_property (object_class,
                                   GIMP_OPERATION_POINT_FILTER_PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "The config object",
                                                        GIMP_TYPE_HUE_SATURATION_CONFIG,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_operation_hue_saturation_init (GimpOperationHueSaturation *self)
{
}

static inline gdouble
map_hue (GimpHueSaturationConfig *config,
         GimpHueRange             range,
         gdouble                  value)
{
  value += (config->hue[GIMP_HUE_RANGE_ALL] + config->hue[range]) / 2.0;

  if (value < 0)
    return value + 1.0;
  else if (value > 1.0)
    return value - 1.0;
  else
    return value;
}

static inline gdouble
map_hue_overlap (GimpHueSaturationConfig *config,
                 GimpHueRange             primary_range,
                 GimpHueRange             secondary_range,
                 gdouble                  value,
                 gdouble                  primary_intensity,
                 gdouble                  secondary_intensity)
{
  /*  When calculating an overlap between two ranges, interpolate the
   *  hue adjustment from config->hue[primary_range] and
   *  config->hue[secondary_range] BEFORE mapping it to the input
   *  value.  This fixes odd edge cases where only one of the ranges
   *  crosses the red/magenta wraparound (bug #527085), or if
   *  adjustments to different channels yield more than 180 degree
   *  difference from each other. (Why anyone would do that is beyond
   *  me, but still.)
   *
   *  See bugs #527085 and #644032 for examples of such cases.
   */
  gdouble v = config->hue[primary_range]   * primary_intensity +
              config->hue[secondary_range] * secondary_intensity;

  value += (config->hue[GIMP_HUE_RANGE_ALL] + v) / 2.0;

  if (value < 0)
    return value + 1.0;
  else if (value > 1.0)
    return value - 1.0;
  else
    return value;
}

static inline gdouble
map_saturation (GimpHueSaturationConfig *config,
                GimpHueRange             range,
                gdouble                  value)
{
  gdouble v = config->saturation[GIMP_HUE_RANGE_ALL] + config->saturation[range];

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
map_lightness (GimpHueSaturationConfig *config,
               GimpHueRange             range,
               gdouble                  value)
{
  gdouble v = (config->lightness[GIMP_HUE_RANGE_ALL] + config->lightness[range]) / 2.0;

  if (v < 0)
    return value * (v + 1.0);
  else
    return value + (v * (1.0 - value));
}

static gboolean
gimp_operation_hue_saturation_process (GeglOperation       *operation,
                                       void                *in_buf,
                                       void                *out_buf,
                                       glong                samples,
                                       const GeglRectangle *roi,
                                       gint                 level)
{
  GimpOperationPointFilter *point  = GIMP_OPERATION_POINT_FILTER (operation);
  GimpHueSaturationConfig  *config = GIMP_HUE_SATURATION_CONFIG (point->config);
  gfloat                   *src    = in_buf;
  gfloat                   *dest   = out_buf;
  gfloat                    overlap;

  if (! config)
    return FALSE;

  overlap = config->overlap / 2.0;

  while (samples--)
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

      rgb.r = src[RED];
      rgb.g = src[GREEN];
      rgb.b = src[BLUE];
      rgb.a = src[ALPHA];

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

      /*  transform into GimpHueRange values  */
      hue++;
      secondary_hue++;

      if (use_secondary_hue)
        {
          hsl.h = map_hue_overlap (config, hue, secondary_hue, hsl.h,
                                   primary_intensity, secondary_intensity);

          hsl.s = (map_saturation (config, hue,           hsl.s) * primary_intensity +
                   map_saturation (config, secondary_hue, hsl.s) * secondary_intensity);

          hsl.l = (map_lightness (config, hue,           hsl.l) * primary_intensity +
                   map_lightness (config, secondary_hue, hsl.l) * secondary_intensity);
        }
      else
        {
          hsl.h = map_hue        (config, hue, hsl.h);
          hsl.s = map_saturation (config, hue, hsl.s);
          hsl.l = map_lightness  (config, hue, hsl.l);
        }

      gimp_hsl_to_rgb (&hsl, &rgb);

      dest[RED]   = rgb.r;
      dest[GREEN] = rgb.g;
      dest[BLUE]  = rgb.b;
      dest[ALPHA] = rgb.a;

      src  += 4;
      dest += 4;
    }

  return TRUE;
}


/*  public functions  */

void
gimp_operation_hue_saturation_map (GimpHueSaturationConfig *config,
                                   const GimpRGB           *color,
                                   GimpHueRange             range,
                                   GimpRGB                 *result)
{
  GimpHSL hsl;

  g_return_if_fail (GIMP_IS_HUE_SATURATION_CONFIG (config));
  g_return_if_fail (color != NULL);
  g_return_if_fail (result != NULL);

  gimp_rgb_to_hsl (color, &hsl);

  hsl.h = map_hue        (config, range, hsl.h);
  hsl.s = map_saturation (config, range, hsl.s);
  hsl.l = map_lightness  (config, range, hsl.l);

  gimp_hsl_to_rgb (&hsl, result);
}
