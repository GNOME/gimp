/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationsplit.c
 * Copyright (C) 2008 Michael Natterer <mitch@ligma.org>
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

#include "ligmaoperationsplit.h"


static gboolean   ligma_operation_split_process (GeglOperation       *op,
                                                void                *in,
                                                void                *layer,
                                                void                *mask,
                                                void                *out,
                                                glong                samples,
                                                const GeglRectangle *roi,
                                                gint                 level);


G_DEFINE_TYPE (LigmaOperationSplit, ligma_operation_split,
               LIGMA_TYPE_OPERATION_LAYER_MODE)


static void
ligma_operation_split_class_init (LigmaOperationSplitClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  LigmaOperationLayerModeClass *layer_mode_class = LIGMA_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "ligma:split",
                                 "description", "LIGMA split mode operation",
                                 NULL);

  layer_mode_class->process = ligma_operation_split_process;
}

static void
ligma_operation_split_init (LigmaOperationSplit *self)
{
}

static gboolean
ligma_operation_split_process (GeglOperation       *op,
                              void                *in_p,
                              void                *layer_p,
                              void                *mask_p,
                              void                *out_p,
                              glong                samples,
                              const GeglRectangle *roi,
                              gint                 level)
{
  LigmaOperationLayerMode *layer_mode = (gpointer) op;
  gfloat                 *in         = in_p;
  gfloat                 *out        = out_p;
  gfloat                 *layer      = layer_p;
  gfloat                 *mask       = mask_p;
  gfloat                  opacity    = layer_mode->opacity;
  const gboolean          has_mask   = mask != NULL;

  switch (layer_mode->composite_mode)
    {
    case LIGMA_LAYER_COMPOSITE_UNION:
      while (samples--)
        {
          gfloat in_alpha    = in[ALPHA];
          gfloat layer_alpha = layer[ALPHA] * opacity;
          gfloat new_alpha;
          gint   b;

          if (has_mask)
            layer_alpha *= *mask;

          if (layer_alpha <= in_alpha)
            {
              new_alpha = in_alpha - layer_alpha;

              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = in[b];
                }
            }
          else
            {
              new_alpha = layer_alpha - in_alpha;

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

    case LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
    case LIGMA_LAYER_COMPOSITE_AUTO:
      while (samples--)
        {
          gfloat in_alpha    = in[ALPHA];
          gfloat layer_alpha = layer[ALPHA] * opacity;
          gfloat new_alpha;
          gint   b;

          if (has_mask)
            layer_alpha *= *mask;

          new_alpha = MAX (in_alpha - layer_alpha, 0.0f);

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = in[b];
            }

          out[ALPHA] = new_alpha;

          in    += 4;
          layer += 4;
          out   += 4;

          if (has_mask)
            mask++;
        }
      break;

    case LIGMA_LAYER_COMPOSITE_CLIP_TO_LAYER:
      while (samples--)
        {
          gfloat in_alpha    = in[ALPHA];
          gfloat layer_alpha = layer[ALPHA] * opacity;
          gfloat new_alpha;
          gint   b;

          if (has_mask)
            layer_alpha *= *mask;

          new_alpha = MAX (layer_alpha - in_alpha, 0.0f);

          if (new_alpha != 0.0f)
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

          out[ALPHA] = new_alpha;

          in    += 4;
          layer += 4;
          out   += 4;

          if (has_mask)
            mask++;
        }
      break;

    case LIGMA_LAYER_COMPOSITE_INTERSECTION:
      while (samples--)
        {
          gint   b;

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
