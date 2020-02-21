/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationerase.c
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

#include "../operations-types.h"

#include "gimpoperationerase.h"


static gboolean   gimp_operation_erase_process (GeglOperation       *op,
                                                void                *in,
                                                void                *layer,
                                                void                *mask,
                                                void                *out,
                                                glong                samples,
                                                const GeglRectangle *roi,
                                                gint                 level);


G_DEFINE_TYPE (GimpOperationErase, gimp_operation_erase,
               GIMP_TYPE_OPERATION_LAYER_MODE)


static void
gimp_operation_erase_class_init (GimpOperationEraseClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  GimpOperationLayerModeClass *layer_mode_class = GIMP_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:erase",
                                 "description", "GIMP erase mode operation",
                                 NULL);

  layer_mode_class->process = gimp_operation_erase_process;
}

static void
gimp_operation_erase_init (GimpOperationErase *self)
{
}

static gboolean
gimp_operation_erase_process (GeglOperation       *op,
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

  switch (layer_mode->composite_mode)
    {
    case GIMP_LAYER_COMPOSITE_UNION:
      while (samples--)
        {
          gfloat layer_alpha;
          gfloat new_alpha;
          gint   b;

          layer_alpha = layer[ALPHA] * opacity;

          if (has_mask)
            layer_alpha *= (*mask);

          new_alpha = in[ALPHA] + layer_alpha - 2.0f * in[ALPHA] * layer_alpha;

          if (new_alpha != 0.0f)
            {
              gfloat ratio;

              ratio = (1.0f - in[ALPHA]) * layer_alpha / new_alpha;

              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = ratio * layer[b] + (1.0f - ratio) * in[b];
                }
            }
          else
            {
              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = in[b];
                }
            }

          out[ALPHA] = new_alpha;

          in    += 4;
          layer += 4;
          out   += 4;

          if (has_mask)
            mask ++;
        }
      break;

    case GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
    case GIMP_LAYER_COMPOSITE_AUTO:
      while (samples--)
        {
          gfloat layer_alpha;
          gfloat new_alpha;
          gint   b;

          layer_alpha = layer[ALPHA] * opacity;

          if (has_mask)
            layer_alpha *= (*mask);

          new_alpha = (1.0f - layer_alpha) * in[ALPHA];

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = in[b];
            }

          out[ALPHA] = new_alpha;

          in    += 4;
          layer += 4;
          out   += 4;

          if (has_mask)
            mask ++;
        }
      break;

    case GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER:
      while (samples--)
        {
          gfloat        layer_alpha;
          gfloat        new_alpha;
          const gfloat *src;
          gint          b;

          layer_alpha = layer[ALPHA] * opacity;

          if (has_mask)
            layer_alpha *= (*mask);

          new_alpha = (1.0f - in[ALPHA]) * layer_alpha;

          src = layer;

          if (new_alpha == 0.0f)
            src = in;

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = src[b];
            }

          out[ALPHA] = new_alpha;

          in    += 4;
          layer += 4;
          out   += 4;

          if (has_mask)
            mask ++;
        }
      break;

    case GIMP_LAYER_COMPOSITE_INTERSECTION:
      while (samples--)
        {
          gint b;

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = in[b];
            }

          out[ALPHA] = 0.0f;

          in    += 4;
          out   += 4;
        }
      break;
    }

  return TRUE;
}
