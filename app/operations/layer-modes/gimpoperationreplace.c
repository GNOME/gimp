/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationreplace.c
 * Copyright (C) 2008 Michael Natterer <mitch@ligma.org>
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

#include "ligma-layer-modes.h"
#include "ligmaoperationreplace.h"


static GeglRectangle              ligma_operation_replace_get_bounding_box    (GeglOperation        *op);

static gboolean                   ligma_operation_replace_parent_process      (GeglOperation        *op,
                                                                              GeglOperationContext *context,
                                                                              const gchar          *output_prop,
                                                                              const GeglRectangle  *result,
                                                                              gint                  level);
static gboolean                   ligma_operation_replace_process             (GeglOperation          *op,
                                                                              void                   *in,
                                                                              void                   *layer,
                                                                              void                   *mask,
                                                                              void                   *out,
                                                                              glong                   samples,
                                                                              const GeglRectangle    *roi,
                                                                              gint                    level);
static LigmaLayerCompositeRegion   ligma_operation_replace_get_affected_region (LigmaOperationLayerMode *layer_mode);


G_DEFINE_TYPE (LigmaOperationReplace, ligma_operation_replace,
               LIGMA_TYPE_OPERATION_LAYER_MODE)

#define parent_class ligma_operation_replace_parent_class


static void
ligma_operation_replace_class_init (LigmaOperationReplaceClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  LigmaOperationLayerModeClass *layer_mode_class = LIGMA_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "ligma:replace",
                                 "description", "LIGMA replace mode operation",
                                 NULL);

  operation_class->get_bounding_box     = ligma_operation_replace_get_bounding_box;

  layer_mode_class->parent_process      = ligma_operation_replace_parent_process;
  layer_mode_class->process             = ligma_operation_replace_process;
  layer_mode_class->get_affected_region = ligma_operation_replace_get_affected_region;
}

static void
ligma_operation_replace_init (LigmaOperationReplace *self)
{
}

static GeglRectangle
ligma_operation_replace_get_bounding_box (GeglOperation *op)
{
  LigmaOperationLayerMode   *self     = (gpointer) op;
  GeglRectangle            *in_rect;
  GeglRectangle            *aux_rect;
  GeglRectangle            *aux2_rect;
  GeglRectangle             src_rect = {};
  GeglRectangle             dst_rect = {};
  GeglRectangle             result;
  LigmaLayerCompositeRegion  included_region;

  in_rect   = gegl_operation_source_get_bounding_box (op, "input");
  aux_rect  = gegl_operation_source_get_bounding_box (op, "aux");
  aux2_rect = gegl_operation_source_get_bounding_box (op, "aux2");

  if (in_rect)
    dst_rect = *in_rect;

  if (aux_rect)
    {
      src_rect = *aux_rect;

      if (aux2_rect)
        gegl_rectangle_intersect (&src_rect, &src_rect, aux2_rect);
    }

  if (self->is_last_node)
    {
      included_region = LIGMA_LAYER_COMPOSITE_REGION_SOURCE;
    }
  else
    {
      included_region = ligma_layer_mode_get_included_region (self->layer_mode,
                                                             self->composite_mode);
    }

  if (self->prop_opacity == 0.0)
    included_region &= ~LIGMA_LAYER_COMPOSITE_REGION_SOURCE;
  else if (self->prop_opacity == 1.0 && ! aux2_rect)
    included_region &= ~LIGMA_LAYER_COMPOSITE_REGION_DESTINATION;

  gegl_rectangle_intersect (&result, &src_rect, &dst_rect);

  if (included_region & LIGMA_LAYER_COMPOSITE_REGION_SOURCE)
    gegl_rectangle_bounding_box (&result, &result, &src_rect);

  if (included_region & LIGMA_LAYER_COMPOSITE_REGION_DESTINATION)
    gegl_rectangle_bounding_box (&result, &result, &dst_rect);

  return result;
}

static gboolean
ligma_operation_replace_parent_process (GeglOperation        *op,
                                       GeglOperationContext *context,
                                       const gchar          *output_prop,
                                       const GeglRectangle  *result,
                                       gint                  level)
{
  LigmaOperationLayerMode   *layer_mode = (gpointer) op;
  LigmaLayerCompositeRegion  included_region;

  included_region = ligma_layer_mode_get_included_region
    (layer_mode->layer_mode, layer_mode->composite_mode);

  /* if the layer's opacity is 100%, it has no mask, and its composite mode
   * contains "aux" (the latter should always be the case in practice,
   * currently,) we can just pass "aux" directly as output.
   */
  if (layer_mode->opacity == 1.0                       &&
      ! gegl_operation_context_get_object (context, "aux2") &&
      (included_region & LIGMA_LAYER_COMPOSITE_REGION_SOURCE))
    {
      GObject *aux;

      aux = gegl_operation_context_get_object (context, "aux");

      gegl_operation_context_set_object (context, "output", aux);

      return TRUE;
    }
  /* the opposite case, where the opacity is 0%, is handled by
   * LigmaOperationLayerMode.
   */
  else if (layer_mode->opacity == 0.0)
    {
    }
  /* if both buffers are included in the result, and if both of them have the
   * same content -- i.e., if they share the same storage, same alignment, and
   * same abyss (or if the abyss is irrelevant) -- we can just pass either of
   * them directly as output.
   */
  else if (included_region == LIGMA_LAYER_COMPOSITE_REGION_UNION)
    {
      GObject *input;
      GObject *aux;

      input = gegl_operation_context_get_object (context, "input");
      aux   = gegl_operation_context_get_object (context, "aux");

      if (input && aux &&
          gegl_buffer_share_storage (GEGL_BUFFER (input), GEGL_BUFFER (aux)))
        {
          gint input_shift_x;
          gint input_shift_y;
          gint aux_shift_x;
          gint aux_shift_y;

          g_object_get (input,
                        "shift-x", &input_shift_x,
                        "shift-y", &input_shift_y,
                        NULL);
          g_object_get (aux,
                        "shift-x", &aux_shift_x,
                        "shift-y", &aux_shift_y,
                        NULL);

          if (input_shift_x == aux_shift_x && input_shift_y == aux_shift_y)
            {
              const GeglRectangle *input_abyss;
              const GeglRectangle *aux_abyss;

              input_abyss = gegl_buffer_get_abyss (GEGL_BUFFER (input));
              aux_abyss   = gegl_buffer_get_abyss (GEGL_BUFFER (aux));

              if (gegl_rectangle_equal (input_abyss, aux_abyss)  ||
                  (gegl_rectangle_contains (input_abyss, result) &&
                   gegl_rectangle_contains (aux_abyss,   result)))
                {
                  gegl_operation_context_set_object (context, "output", input);

                  return TRUE;
                }
            }
        }
    }

  return LIGMA_OPERATION_LAYER_MODE_CLASS (parent_class)->parent_process (
    op, context, output_prop, result, level);
}

static gboolean
ligma_operation_replace_process (GeglOperation       *op,
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

    case LIGMA_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
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

    case LIGMA_LAYER_COMPOSITE_CLIP_TO_LAYER:
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

    case LIGMA_LAYER_COMPOSITE_INTERSECTION:
      memset (out, 0, 4 * samples * sizeof (gfloat));
      break;
    }

  return TRUE;
}

static LigmaLayerCompositeRegion
ligma_operation_replace_get_affected_region (LigmaOperationLayerMode *layer_mode)
{
  LigmaLayerCompositeRegion affected_region = LIGMA_LAYER_COMPOSITE_REGION_INTERSECTION;

  if (layer_mode->prop_opacity != 0.0)
    affected_region |= LIGMA_LAYER_COMPOSITE_REGION_DESTINATION;

  /* if opacity != 1.0, or we have a mask, then we also affect SOURCE, but this
   * is considered the case anyway, so no need for special handling.
   */

  return affected_region;
}
