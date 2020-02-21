/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationmerge.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2017 Ell
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

#include "gimpoperationmerge.h"


static gboolean   gimp_operation_merge_process (GeglOperation       *op,
                                                void                *in,
                                                void                *layer,
                                                void                *mask,
                                                void                *out,
                                                glong                samples,
                                                const GeglRectangle *roi,
                                                gint                 level);


G_DEFINE_TYPE (GimpOperationMerge, gimp_operation_merge,
               GIMP_TYPE_OPERATION_LAYER_MODE)


static void
gimp_operation_merge_class_init (GimpOperationMergeClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  GimpOperationLayerModeClass *layer_mode_class = GIMP_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:merge",
                                 "description", "GIMP merge mode operation",
                                 NULL);

  layer_mode_class->process = gimp_operation_merge_process;
}

static void
gimp_operation_merge_init (GimpOperationMerge *self)
{
}

static gboolean
gimp_operation_merge_process (GeglOperation       *op,
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
          gfloat in_alpha    = in[ALPHA];
          gfloat layer_alpha = layer[ALPHA] * opacity;
          gfloat new_alpha;
          gint   b;

          if (has_mask)
            layer_alpha *= *mask;

          in_alpha = MIN (in_alpha, 1.0f - layer_alpha);
          new_alpha  = in_alpha + layer_alpha;

          if (new_alpha)
            {
              gfloat ratio = layer_alpha / new_alpha;

              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = in[b] + (layer[b] - in[b]) * ratio;
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
            mask++;
        }
      break;

    case GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
      while (samples--)
        {
          gfloat in_alpha    = in[ALPHA];
          gfloat layer_alpha = layer[ALPHA] * opacity;
          gint   b;

          if (has_mask)
            layer_alpha *= *mask;

          layer_alpha -= 1.0f - in_alpha;

          if (layer_alpha > 0.0f)
            {
              gfloat ratio = layer_alpha / in_alpha;

              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = in[b] + (layer[b] - in[b]) * ratio;
                }
            }
          else
            {
              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = in[b];
                }
            }

          out[ALPHA] = in_alpha;

          in    += 4;
          layer += 4;
          out   += 4;

          if (has_mask)
            mask++;
        }
      break;

    case GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER:
      while (samples--)
        {
          gfloat layer_alpha = layer[ALPHA] * opacity;
          gint   b;

          if (has_mask)
            layer_alpha *= *mask;

          if (layer_alpha != 0.0f)
            {
              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = layer[b];
                }
            }
          else
            {
              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = in[b];
                }
            }

          out[ALPHA] = layer_alpha;

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
          gfloat in_alpha    = in[ALPHA];
          gfloat layer_alpha = layer[ALPHA] * opacity;
          gint   b;

          if (has_mask)
            layer_alpha *= *mask;

          layer_alpha -= 1.0f - in_alpha;
          layer_alpha = MAX (layer_alpha, 0.0f);

          if (layer_alpha != 0.0f)
            {
              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = layer[b];
                }
            }
          else
            {
              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = in[b];
                }
            }

          out[ALPHA] = layer_alpha;

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
