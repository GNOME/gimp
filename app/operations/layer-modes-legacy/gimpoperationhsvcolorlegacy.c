/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationcolormode.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2012 Ville Sokk <ville.sokk@gmail.com>
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

#include <gegl-plugin.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"

#include "../operations-types.h"

#include "gimpoperationhsvcolorlegacy.h"


static gboolean gimp_operation_hsv_color_legacy_process (GeglOperation       *operation,
                                                         void                *in_buf,
                                                         void                *aux_buf,
                                                         void                *aux2_buf,
                                                         void                *out_buf,
                                                         glong                samples,
                                                         const GeglRectangle *roi,
                                                         gint                 level);


G_DEFINE_TYPE (GimpOperationHsvColorLegacy, gimp_operation_hsv_color_legacy,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)


static void
gimp_operation_hsv_color_legacy_class_init (GimpOperationHsvColorLegacyClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:hsv-color-legacy",
                                 "description", "GIMP color mode operation",
                                 NULL);

  point_class->process = gimp_operation_hsv_color_legacy_process;
}

static void
gimp_operation_hsv_color_legacy_init (GimpOperationHsvColorLegacy *self)
{
}

static gboolean
gimp_operation_hsv_color_legacy_process (GeglOperation       *operation,
                                         void                *in_buf,
                                         void                *aux_buf,
                                         void                *aux2_buf,
                                         void                *out_buf,
                                         glong                samples,
                                         const GeglRectangle *roi,
                                         gint                 level)
{
  GimpOperationPointLayerMode *layer_mode = (gpointer) operation;

  return gimp_operation_hsv_color_legacy_process_pixels (in_buf, aux_buf, aux2_buf,
                                                         out_buf,
                                                         layer_mode->opacity,
                                                         samples, roi, level,
                                                         layer_mode->blend_trc,
                                                         layer_mode->composite_trc,
                                                         layer_mode->composite_mode);
}

gboolean
gimp_operation_hsv_color_legacy_process_pixels (gfloat                *in,
                                                gfloat                *layer,
                                                gfloat                *mask,
                                                gfloat                *out,
                                                gfloat                 opacity,
                                                glong                  samples,
                                                const GeglRectangle   *roi,
                                                gint                   level,
                                                GimpLayerBlendTRC      blend_trc,
                                                GimpLayerBlendTRC      composite_trc,
                                                GimpLayerCompositeMode composite_mode)
{
  const gboolean has_mask = mask != NULL;

  while (samples--)
    {
      GimpHSL layer_hsl, out_hsl;
      GimpRGB layer_rgb = {layer[0], layer[1], layer[2]};
      GimpRGB out_rgb   = {in[0], in[1], in[2]};
      gfloat  comp_alpha, new_alpha;

      comp_alpha = MIN (in[ALPHA], layer[ALPHA]) * opacity;
      if (has_mask)
        comp_alpha *= *mask;

      new_alpha = in[ALPHA] + (1.0 - in[ALPHA]) * comp_alpha;

      if (comp_alpha && new_alpha)
        {
          gint   b;
          gfloat out_tmp[3];
          gfloat ratio = comp_alpha / new_alpha;

          gimp_rgb_to_hsl (&layer_rgb, &layer_hsl);
          gimp_rgb_to_hsl (&out_rgb, &out_hsl);

          out_hsl.h = layer_hsl.h;
          out_hsl.s = layer_hsl.s;
          gimp_hsl_to_rgb (&out_hsl, &out_rgb);

          out_tmp[0] = out_rgb.r;
          out_tmp[1] = out_rgb.g;
          out_tmp[2] = out_rgb.b;

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = out_tmp[b] * ratio + in[b] * (1.0 - ratio);
            }
        }
      else
        {
          gint b;

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = in[b];
            }
        }

      out[ALPHA] = in[ALPHA];

      in    += 4;
      layer += 4;
      out   += 4;

      if (has_mask)
        mask++;
    }

  return TRUE;
}
