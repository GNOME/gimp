/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationoverwrite.c
 * Copyright (C) 2025 Woynert <woynertgamer@gmail.com>
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

#include "../operations-types.h"

#include "gimpoperationoverwrite.h"


static gboolean
gimp_operation_overwrite_process (GeglOperation       *op,
                                  void                *in,
                                  void                *layer,
                                  void                *mask,
                                  void                *out,
                                  glong                samples,
                                  const GeglRectangle *roi,
                                  gint                 level);


G_DEFINE_TYPE (GimpOperationOverwrite, gimp_operation_overwrite,
               GIMP_TYPE_OPERATION_LAYER_MODE)


static void
gimp_operation_overwrite_class_init (GimpOperationOverwriteClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  GimpOperationLayerModeClass *layer_mode_class = GIMP_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:overwrite",
                                 "description", "GIMP overwrite mode operation",
                                 NULL);

  layer_mode_class->process = gimp_operation_overwrite_process;
}

static void
gimp_operation_overwrite_init (GimpOperationOverwrite *self)
{
}

static gboolean
gimp_operation_overwrite_process (GeglOperation       *op,
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
  const gboolean          has_mask   = mask != NULL;
  gfloat                  mask_value;
  gfloat                  layer_alpha;

  switch (layer_mode->composite_mode)
    {
    case GIMP_LAYER_COMPOSITE_UNION:
    case GIMP_LAYER_COMPOSITE_AUTO:
      while (samples--)
        {
          if (has_mask)
            mask_value = *(mask++);
          else
            mask_value = 1.0;

          layer_alpha = layer[ALPHA] * opacity * mask_value;

          if (layer_alpha > 0.0f)
            {
              gint b;

              /* We still interpolate both the alpha and RGB channels,
               * using the mask value which gives us smoother transition
               * in paintbrush style tools while the pencil tool
               * provides hard pixel replacement as expected.
               *
               * Note that mask_value is used both as part of
               * layer_alpha and as interpolation variable. This is on
               * purpose because we are interpolating between the
               * input's opacity and the applied one (based on layer
               * opacity, tool opacity and mask value). In particular,
               * without this, painting with dynamics changing opacity
               * could not lower the pixel's opacity.
               */
              out[ALPHA] = in[ALPHA] + mask_value * (layer_alpha - in[ALPHA]);

              for (b = RED; b < ALPHA; b++)
                out[b] = in[b] + mask_value * (layer[b] - in[b]);
            }
          else
            {
              gint b;

              for (b = RED; b <= ALPHA; b++)
                out[b] = in[b];
            }

          in    += 4;
          layer += 4;
          out   += 4;
        }
      break;

    case GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
    case GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER:
    case GIMP_LAYER_COMPOSITE_INTERSECTION:
      g_return_val_if_reached (FALSE);
    }

  return TRUE;
}
