/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationantierase.c
 * Copyright (C) 2008 Michael Natterer <mitch@ligma.org>
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

#include "ligmaoperationantierase.h"



static gboolean                   ligma_operation_anti_erase_process             (GeglOperation          *op,
                                                                                 void                   *in,
                                                                                 void                   *layer,
                                                                                 void                   *mask,
                                                                                 void                   *out,
                                                                                 glong                   samples,
                                                                                 const GeglRectangle    *roi,
                                                                                 gint                    level);
static LigmaLayerCompositeRegion   ligma_operation_anti_erase_get_affected_region (LigmaOperationLayerMode *layer_mode);


G_DEFINE_TYPE (LigmaOperationAntiErase, ligma_operation_anti_erase,
               LIGMA_TYPE_OPERATION_LAYER_MODE)


static void
ligma_operation_anti_erase_class_init (LigmaOperationAntiEraseClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  LigmaOperationLayerModeClass *layer_mode_class = LIGMA_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "ligma:anti-erase",
                                 "description", "LIGMA anti erase mode operation",
                                 NULL);

  layer_mode_class->process             = ligma_operation_anti_erase_process;
  layer_mode_class->get_affected_region = ligma_operation_anti_erase_get_affected_region;
}

static void
ligma_operation_anti_erase_init (LigmaOperationAntiErase *self)
{
}

static gboolean
ligma_operation_anti_erase_process (GeglOperation       *op,
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
    case LIGMA_LAYER_COMPOSITE_AUTO:
      while (samples--)
        {
          gfloat value = opacity;
          gint   b;

          if (has_mask)
            value *= *mask;

          out[ALPHA] = in[ALPHA] + (1.0 - in[ALPHA]) * layer[ALPHA] * value;

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = in[b];
            }

          in    += 4;
          layer += 4;
          out   += 4;

          if (has_mask)
            mask++;
        }
      break;

    case LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
      while (samples--)
        {
          gint b;

          out[ALPHA] = in[ALPHA];

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = in[b];
            }

          in  += 4;
          out += 4;
        }
      break;

    case LIGMA_LAYER_COMPOSITE_CLIP_TO_LAYER:
      while (samples--)
        {
          gfloat value = opacity;
          gint   b;

          if (has_mask)
            value *= *mask;

          out[ALPHA] = layer[ALPHA] * value;

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = in[b];
            }

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
          gfloat value = opacity;
          gint   b;

          if (has_mask)
            value *= *mask;

          out[ALPHA] = in[ALPHA] * layer[ALPHA] * value;

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = in[b];
            }

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

static LigmaLayerCompositeRegion
ligma_operation_anti_erase_get_affected_region (LigmaOperationLayerMode *layer_mode)
{
  return LIGMA_LAYER_COMPOSITE_REGION_SOURCE;
}
