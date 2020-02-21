/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationbehind.c
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

#include "gimpoperationbehind.h"



static gboolean   gimp_operation_behind_process (GeglOperation       *op,
                                                 void                *in,
                                                 void                *layer,
                                                 void                *mask,
                                                 void                *out,
                                                 glong                samples,
                                                 const GeglRectangle *roi,
                                                 gint                 level);


G_DEFINE_TYPE (GimpOperationBehind, gimp_operation_behind,
               GIMP_TYPE_OPERATION_LAYER_MODE)


static void
gimp_operation_behind_class_init (GimpOperationBehindClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  GimpOperationLayerModeClass *layer_mode_class = GIMP_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:behind",
                                 "description", "GIMP behind mode operation",
                                 NULL);

  layer_mode_class->process = gimp_operation_behind_process;
}

static void
gimp_operation_behind_init (GimpOperationBehind *self)
{
}

static gboolean
gimp_operation_behind_process (GeglOperation       *op,
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
    case GIMP_LAYER_COMPOSITE_AUTO:
      while (samples--)
        {
          gfloat src1_alpha = in[ALPHA];
          gfloat src2_alpha = layer[ALPHA] * opacity;
          gfloat new_alpha;
          gint   b;

          if (has_mask)
            src2_alpha *= *mask;

          new_alpha = src2_alpha + (1.0 - src2_alpha) * src1_alpha;

          if (new_alpha)
            {
              gfloat ratio = in[ALPHA] / new_alpha;
              gfloat compl_ratio = 1.0f - ratio;

              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = in[b] * ratio + layer[b] * compl_ratio;
                }
            }
          else
            {
              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = layer[b];
                }
            }

          out[ALPHA] = new_alpha;

          in    += 4;
          layer += 4;
          out   += 4;

          if (has_mask)
            mask++;
        }
      break;

    case GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
      while (samples--)
        {
          gfloat src1_alpha = in[ALPHA];
          gfloat new_alpha;
          gint   b;

          new_alpha = src1_alpha;

          if (new_alpha)
            {
              for (b = RED; b < ALPHA; b++)
                out[b] = in[b];
            }
          else
            {
              for (b = RED; b < ALPHA; b++)
                out[b] = layer[b];
            }

          out[ALPHA] = new_alpha;

          in    += 4;
          layer += 4;
          out   += 4;
        }
      break;

    case GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER:
      while (samples--)
        {
          gfloat src1_alpha = in[ALPHA];
          gfloat src2_alpha = layer[ALPHA] * opacity;
          gfloat new_alpha;
          gint   b;

          if (has_mask)
            src2_alpha *= *mask;

          new_alpha = src2_alpha;

          if (new_alpha)
            {
              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = layer[b] + (in[b] - layer[b]) * src1_alpha;
                }
            }
          else
            {
              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = layer[b];
                }
            }

          out[ALPHA] = new_alpha;

          in    += 4;
          layer += 4;
          out   += 4;

          if (has_mask)
            mask++;
        }
      break;

      case GIMP_LAYER_COMPOSITE_INTERSECTION:
      while (samples--)
        {
          gfloat src1_alpha = in[ALPHA];
          gfloat src2_alpha = layer[ALPHA] * opacity;
          gfloat new_alpha;
          gint   b;

          if (has_mask)
            src2_alpha *= *mask;

          new_alpha = src1_alpha * src2_alpha;

          if (new_alpha)
            {
              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = in[b];
                }
            }
          else
            {
              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = layer[b];
                }
            }

          out[ALPHA] = new_alpha;

          in    += 4;
          layer += 4;
          out   += 4;

          if (has_mask)
            mask++;
        }
      break;
    }

  return TRUE;
}
