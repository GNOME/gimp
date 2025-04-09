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

  switch (layer_mode->composite_mode)
    {
    case GIMP_LAYER_COMPOSITE_UNION:
    case GIMP_LAYER_COMPOSITE_AUTO:
      while (samples--)
        {
          if (has_mask)
            {
              out[ALPHA] = in[ALPHA] + (*mask) * (opacity - in[ALPHA]);

              if (opacity + in[ALPHA] > -1e-5)
                {
                  gfloat C;
                  gfloat lerp;
                  gint   b;

                  /* RGB interpolation */

                  C = in[ALPHA] / ( opacity + in[ALPHA] );

                  if (*mask < C)
                    lerp = *mask * ((1 - C) / C);
                  else
                    lerp = (*mask - C) * (C / (1 - C)) + 1 - C;

                  for (b = RED; b < ALPHA; b++)
                    {
                      out[b] = in[b] + (lerp) *  (layer[b] - in[b]);
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

              mask++;
            }

          in    += 4;
          layer += 4;
          out   += 4;
        }
      break;

    case GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
      while (samples--)
        {
          gfloat layer_alpha;

          layer_alpha = layer[ALPHA] * opacity;
          if (has_mask)
            layer_alpha *= *mask;

          out[ALPHA] = in[ALPHA];

          if (out[ALPHA])
            {
              gint b;

              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = in[b] + (layer[b] - in[b]) * layer_alpha;
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
          gfloat layer_alpha;

          layer_alpha = layer[ALPHA] * opacity;
          if (has_mask)
            layer_alpha *= *mask;

          out[ALPHA] = layer_alpha;

          if (out[ALPHA])
            {
              gint b;

              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = layer[b];
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
          gfloat layer_alpha;

          layer_alpha = layer[ALPHA] * opacity;
          if (has_mask)
            layer_alpha *= *mask;

          out[ALPHA] = in[ALPHA] * layer_alpha;

          if (out[ALPHA])
            {
              gint b;

              for (b = RED; b < ALPHA; b++)
                {
                  out[b] = layer[b];
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
