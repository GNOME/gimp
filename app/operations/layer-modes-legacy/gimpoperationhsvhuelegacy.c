/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationhuemode.c
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

#include <cairo.h>
#include <gegl-plugin.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"

#include "../operations-types.h"

#include "gimpcolor-legacy.h"
#include "gimpoperationhsvhuelegacy.h"


static gboolean   gimp_operation_hsv_hue_legacy_process (GeglOperation       *op,
                                                         void                *in,
                                                         void                *layer,
                                                         void                *mask,
                                                         void                *out,
                                                         glong                samples,
                                                         const GeglRectangle *roi,
                                                         gint                 level);


G_DEFINE_TYPE (GimpOperationHsvHueLegacy, gimp_operation_hsv_hue_legacy,
               GIMP_TYPE_OPERATION_LAYER_MODE)


static void
gimp_operation_hsv_hue_legacy_class_init (GimpOperationHsvHueLegacyClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  GimpOperationLayerModeClass *layer_mode_class = GIMP_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:hsv-hue-legacy",
                                 "description", "GIMP hue mode operation",
                                 NULL);

  layer_mode_class->process = gimp_operation_hsv_hue_legacy_process;
}

static void
gimp_operation_hsv_hue_legacy_init (GimpOperationHsvHueLegacy *self)
{
}

static gboolean
gimp_operation_hsv_hue_legacy_process (GeglOperation       *op,
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
      gdouble layer_hsv[4], out_hsv[4];
      gdouble layer_rgb[4] = {layer[0], layer[1], layer[2], 1.0};
      gdouble out_rgb[4]   = {in[0], in[1], in[2], 1.0};
      gfloat  comp_alpha, new_alpha;

      comp_alpha = MIN (in[ALPHA], layer[ALPHA]) * opacity;
      if (mask)
        comp_alpha *= *mask;

      new_alpha = in[ALPHA] + (1.0f - in[ALPHA]) * comp_alpha;

      if (comp_alpha && new_alpha)
        {
          gint   b;
          gfloat ratio = comp_alpha / new_alpha;

          gimp_rgb_to_hsv_legacy (layer_rgb, layer_hsv);
          gimp_rgb_to_hsv_legacy (out_rgb, out_hsv);

          /*  Composition should have no effect if saturation is zero.
           *  otherwise, black would be painted red (see bug #123296).
           */
          if (layer_hsv[1])
            out_hsv[0] = layer_hsv[0];

          gimp_hsv_to_rgb_legacy (out_hsv, out_rgb);

          for (b = RED; b < ALPHA; b++)
            out[b] = out_rgb[b] * ratio + in[b] * (1.0f - ratio);
        }
      else
        {
          gint b;

          for (b = RED; b < ALPHA; b++)
            out[b] = in[b];
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
