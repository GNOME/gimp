/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationgrainextractmode.c
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

#include "../operations-types.h"

#include "gimpoperationgrainextractlegacy.h"

G_DEFINE_TYPE (GimpOperationGrainExtractLegacy, gimp_operation_grain_extract_legacy,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)


static void
gimp_operation_grain_extract_legacy_class_init (GimpOperationGrainExtractLegacyClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:grain-extract-legacy",
                                 "description", "GIMP grain extract mode operation",
                                 NULL);

  point_class->process = gimp_operation_grain_extract_legacy_process;
}

static void
gimp_operation_grain_extract_legacy_init (GimpOperationGrainExtractLegacy *self)
{
}


gboolean
gimp_operation_grain_extract_legacy_process (GeglOperation       *op,
                                             void                *in_p,
                                             void                *layer_p,
                                             void                *mask_p,
                                             void                *out_p,
                                             glong                samples,
                                             const GeglRectangle *roi,
                                             gint                 level)
{
  GimpOperationPointLayerMode *layer_mode = (gpointer)op;
  gfloat opacity = layer_mode->opacity;
  gfloat *in = in_p, *out = out_p, *layer = layer_p, *mask  = mask_p;

  while (samples--)
    {
      gfloat comp_alpha, new_alpha;

      comp_alpha = MIN (in[ALPHA], layer[ALPHA]) * opacity;
      if (mask)
        comp_alpha *= *mask;

      new_alpha = in[ALPHA] + (1.0 - in[ALPHA]) * comp_alpha;

      if (comp_alpha && new_alpha)
        {
          gfloat ratio = comp_alpha / new_alpha;
          gint   b;

          for (b = RED; b < ALPHA; b++)
            {
              gfloat comp = in[b] - layer[b] + 0.5;

              out[b] = comp * ratio + in[b] * (1.0 - ratio);
              out[b] = CLAMP (out[b], 0.0, 1.0);
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
