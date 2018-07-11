/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationsaturationmode.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl-plugin.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"

#include "../operations-types.h"

#include "gimpoperationhsvsaturationlegacy.h"


static gboolean   gimp_operation_hsv_saturation_legacy_process (GeglOperation       *op,
                                                                void                *in,
                                                                void                *layer,
                                                                void                *mask,
                                                                void                *out,
                                                                glong                samples,
                                                                const GeglRectangle *roi,
                                                                gint                 level);


G_DEFINE_TYPE (GimpOperationHsvSaturationLegacy, gimp_operation_hsv_saturation_legacy,
               GIMP_TYPE_OPERATION_LAYER_MODE)


static void
gimp_operation_hsv_saturation_legacy_class_init (GimpOperationHsvSaturationLegacyClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  GimpOperationLayerModeClass *layer_mode_class = GIMP_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:hsv-saturation-legacy",
                                 "description", "GIMP saturation mode operation",
                                 NULL);

  layer_mode_class->process = gimp_operation_hsv_saturation_legacy_process;
}

static void
gimp_operation_hsv_saturation_legacy_init (GimpOperationHsvSaturationLegacy *self)
{
}

static gboolean
gimp_operation_hsv_saturation_legacy_process (GeglOperation       *op,
                                              void                *in_p,
                                              void                *layer_p,
                                              void                *mask_p,
                                              void                *out_p,
                                              glong                samples,
                                              const GeglRectangle *roi,
                                              gint                 level)
{
  GimpOperationLayerMode *layer_mode = (gpointer) op;
  gfloat                 *in         = in_p;
  gfloat                 *out        = out_p;
  gfloat                 *layer      = layer_p;
  gfloat                 *mask       = mask_p;
  gfloat                  opacity    = layer_mode->opacity;

  while (samples--)
    {
      GimpHSV layer_hsv, out_hsv;
      GimpRGB layer_rgb = {layer[0], layer[1], layer[2]};
      GimpRGB out_rgb   = {in[0], in[1], in[2]};
      gfloat  comp_alpha, new_alpha;

      comp_alpha = MIN (in[ALPHA], layer[ALPHA]) * opacity;
      if (mask)
        comp_alpha *= *mask;

      new_alpha = in[ALPHA] + (1.0f - in[ALPHA]) * comp_alpha;

      if (comp_alpha && new_alpha)
        {
          gint   b;
          gfloat out_tmp[3];
          gfloat ratio = comp_alpha / new_alpha;

          gimp_rgb_to_hsv (&layer_rgb, &layer_hsv);
          gimp_rgb_to_hsv (&out_rgb, &out_hsv);

          out_hsv.s = layer_hsv.s;
          gimp_hsv_to_rgb (&out_hsv, &out_rgb);

          out_tmp[0] = out_rgb.r;
          out_tmp[1] = out_rgb.g;
          out_tmp[2] = out_rgb.b;

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = out_tmp[b] * ratio + in[b] * (1.0f - ratio);
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

      if (mask)
        mask++;
    }

  return TRUE;
}
