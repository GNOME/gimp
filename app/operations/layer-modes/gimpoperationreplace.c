/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationreplace.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl-plugin.h>

#include "../operations-types.h"

#include "gimp-layer-modes.h"
#include "gimpoperationreplace.h"


static gboolean                   gimp_operation_replace_parent_process      (GeglOperation        *op,
                                                                              GeglOperationContext *context,
                                                                              const gchar          *output_prop,
                                                                              const GeglRectangle  *result,
                                                                              gint                  level);

static gboolean                   gimp_operation_replace_process             (GeglOperation          *op,
                                                                              void                   *in,
                                                                              void                   *layer,
                                                                              void                   *mask,
                                                                              void                   *out,
                                                                              glong                   samples,
                                                                              const GeglRectangle    *roi,
                                                                              gint                    level);
static GimpLayerCompositeRegion   gimp_operation_replace_get_affected_region (GimpOperationLayerMode *layer_mode);


G_DEFINE_TYPE (GimpOperationReplace, gimp_operation_replace,
               GIMP_TYPE_OPERATION_LAYER_MODE)

#define parent_class gimp_operation_replace_parent_class


static void
gimp_operation_replace_class_init (GimpOperationReplaceClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  GimpOperationLayerModeClass *layer_mode_class = GIMP_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:replace",
                                 "description", "GIMP replace mode operation",
                                 NULL);

  operation_class->process              = gimp_operation_replace_parent_process;

  layer_mode_class->process             = gimp_operation_replace_process;
  layer_mode_class->get_affected_region = gimp_operation_replace_get_affected_region;
}

static void
gimp_operation_replace_init (GimpOperationReplace *self)
{
}

static gboolean
gimp_operation_replace_parent_process (GeglOperation        *op,
                                       GeglOperationContext *context,
                                       const gchar          *output_prop,
                                       const GeglRectangle  *result,
                                       gint                  level)
{
  GimpOperationLayerMode *layer_mode = (gpointer) op;

  /* if the layer's opacity is 100%, it has no mask, and its composite mode
   * contains "aux" (the latter should always be the case in practice,
   * currently,) we can just pass "aux" directly as output.
   */
  if (layer_mode->opacity == 1.0                            &&
      ! gegl_operation_context_get_object (context, "aux2") &&
      (gimp_layer_mode_get_included_region (layer_mode->layer_mode,
                                            layer_mode->real_composite_mode) &
       GIMP_LAYER_COMPOSITE_REGION_SOURCE))
    {
      GObject *aux;

      aux = gegl_operation_context_get_object (context, "aux");

      gegl_operation_context_set_object (context, "output", aux);

      return TRUE;
    }
  /* the opposite case, where the opacity is 0%, is handled by
   * GimpOperationLayerMode.
   */

  return GEGL_OPERATION_CLASS (parent_class)->process (op, context, output_prop,
                                                       result, level);
}

static gboolean
gimp_operation_replace_process (GeglOperation       *op,
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

  switch (layer_mode->real_composite_mode)
    {
    case GIMP_LAYER_COMPOSITE_UNION:
    case GIMP_LAYER_COMPOSITE_AUTO:
      while (samples--)
        {
          gfloat opacity_value = opacity;
          gfloat new_alpha;
          gfloat ratio;
          gint   b;

          if (has_mask)
            opacity_value *= *mask;

          new_alpha = (layer[ALPHA] - in[ALPHA]) * opacity_value + in[ALPHA];

          ratio = opacity_value;

          if (new_alpha)
            ratio *= layer[ALPHA] / new_alpha;

          for (b = RED; b < ALPHA; b++)
            out[b] = (layer[b] - in[b]) * ratio + in[b];

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
          gfloat opacity_value = opacity;
          gfloat new_alpha;
          gint   b;

          if (has_mask)
            opacity_value *= *mask;

          new_alpha = in[ALPHA] * (1.0f - opacity_value);

          for (b = RED; b < ALPHA; b++)
            out[b] = in[b];

          out[ALPHA] = new_alpha;

          in  += 4;
          out += 4;

          if (has_mask)
            mask++;
        }
      break;

    case GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER:
      while (samples--)
        {
          gfloat opacity_value = opacity;
          gfloat new_alpha;
          gint   b;

          if (has_mask)
            opacity_value *= *mask;

          new_alpha = layer[ALPHA] * opacity_value;

          for (b = RED; b < ALPHA; b++)
            out[b] = layer[b];

          out[ALPHA] = new_alpha;

          in    += 4;
          layer += 4;
          out   += 4;

          if (has_mask)
            mask++;
        }
      break;

    case GIMP_LAYER_COMPOSITE_INTERSECTION:
      memset (out, 0, 4 * samples * sizeof (gfloat));
      break;
    }

  return TRUE;
}

static GimpLayerCompositeRegion
gimp_operation_replace_get_affected_region (GimpOperationLayerMode *layer_mode)
{
  GimpLayerCompositeRegion affected_region = GIMP_LAYER_COMPOSITE_REGION_INTERSECTION;

  if (layer_mode->opacity != 0.0)
    affected_region |= GIMP_LAYER_COMPOSITE_REGION_DESTINATION;

  /* if opacity != 1.0, or we have a mask, then we also affect SOURCE, but this
   * is considered the case anyway, so no need for special handling.
   */

  return affected_region;
}
