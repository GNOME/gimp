/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "gegl/ligma-gegl-loops.h"

#include "ligmachannel.h"
#include "ligmadrawable.h"
#include "ligmadrawable-edit.h"
#include "ligmadrawablefilter.h"
#include "ligmacontext.h"
#include "ligmafilloptions.h"
#include "ligmaimage.h"
#include "ligmapattern.h"
#include "ligmatempbuf.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static gboolean   ligma_drawable_edit_can_fill_direct (LigmaDrawable    *drawable,
                                                      LigmaFillOptions *options);
static void       ligma_drawable_edit_fill_direct     (LigmaDrawable    *drawable,
                                                      LigmaFillOptions *options,
                                                      const gchar     *undo_desc);


/*  private functions  */

static gboolean
ligma_drawable_edit_can_fill_direct (LigmaDrawable    *drawable,
                                    LigmaFillOptions *options)
{
  LigmaImage                *image;
  LigmaContext              *context;
  gdouble                   opacity;
  LigmaComponentMask         affect;
  LigmaLayerMode             mode;
  LigmaLayerCompositeMode    composite_mode;
  LigmaLayerCompositeRegion  composite_region;

  image            = ligma_item_get_image (LIGMA_ITEM (drawable));
  context          = LIGMA_CONTEXT (options);
  opacity          = ligma_context_get_opacity (context);
  affect           = ligma_drawable_get_active_mask (drawable);
  mode             = ligma_context_get_paint_mode (context);
  composite_mode   = ligma_layer_mode_get_paint_composite_mode (mode);
  composite_region = ligma_layer_mode_get_included_region (mode, composite_mode);

  if (ligma_channel_is_empty (ligma_image_get_mask (image)) &&
      opacity == LIGMA_OPACITY_OPAQUE                      &&
      affect  == LIGMA_COMPONENT_MASK_ALL                  &&
      ligma_layer_mode_is_trivial (mode)                   &&
      (! ligma_layer_mode_is_subtractive (mode)            ^
       ! (composite_region & LIGMA_LAYER_COMPOSITE_REGION_SOURCE)))
    {
      switch (ligma_fill_options_get_style (options))
        {
        case LIGMA_FILL_STYLE_SOLID:
          return TRUE;

        case LIGMA_FILL_STYLE_PATTERN:
          {
            LigmaPattern *pattern;
            LigmaTempBuf *mask;
            const Babl  *format;

            pattern = ligma_context_get_pattern (context);
            mask    = ligma_pattern_get_mask (pattern);
            format  = ligma_temp_buf_get_format (mask);

            return ! babl_format_has_alpha (format);
          }
        }
    }

  return FALSE;
}

static void
ligma_drawable_edit_fill_direct (LigmaDrawable    *drawable,
                                LigmaFillOptions *options,
                                const gchar     *undo_desc)
{
  GeglBuffer    *buffer;
  LigmaContext   *context;
  LigmaLayerMode  mode;
  gint           width;
  gint           height;

  buffer  = ligma_drawable_get_buffer (drawable);
  context = LIGMA_CONTEXT (options);
  mode    = ligma_context_get_paint_mode (context);
  width   = ligma_item_get_width  (LIGMA_ITEM (drawable));
  height  = ligma_item_get_height (LIGMA_ITEM (drawable));

  ligma_drawable_push_undo (drawable, undo_desc,
                           NULL, 0, 0, width, height);

  if (! ligma_layer_mode_is_subtractive (mode))
    ligma_fill_options_fill_buffer (options, drawable, buffer, 0, 0);
  else
    ligma_gegl_clear (buffer, NULL);
}


/*  public functions  */

void
ligma_drawable_edit_clear (LigmaDrawable *drawable,
                          LigmaContext  *context)
{
  LigmaFillOptions *options;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  options = ligma_fill_options_new (context->ligma, NULL, FALSE);

  if (ligma_drawable_has_alpha (drawable))
    ligma_fill_options_set_by_fill_type (options, context,
                                        LIGMA_FILL_TRANSPARENT, NULL);
  else
    ligma_fill_options_set_by_fill_type (options, context,
                                        LIGMA_FILL_BACKGROUND, NULL);

  ligma_drawable_edit_fill (drawable, options, C_("undo-type", "Clear"));

  g_object_unref (options);
}

void
ligma_drawable_edit_fill (LigmaDrawable    *drawable,
                         LigmaFillOptions *options,
                         const gchar     *undo_desc)
{
  LigmaContext *context;
  gint         x, y, width, height;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (LIGMA_IS_FILL_OPTIONS (options));

  if (! ligma_item_mask_intersect (LIGMA_ITEM (drawable),
                                  &x, &y, &width, &height))
    {
      return;  /*  nothing to do, but the fill succeeded  */
    }

  context = LIGMA_CONTEXT (options);

  if (ligma_layer_mode_is_alpha_only (ligma_context_get_paint_mode (context)))
    {
      if (! ligma_drawable_has_alpha (drawable) ||
          ! (ligma_drawable_get_active_mask (drawable) &
             LIGMA_COMPONENT_MASK_ALPHA))
        {
          return;  /*  nothing to do, but the fill succeeded  */
        }
    }

  if (! undo_desc)
    undo_desc = ligma_fill_options_get_undo_desc (options);

  /* check if we can fill the drawable's buffer directly */
  if (ligma_drawable_edit_can_fill_direct (drawable, options))
    {
      ligma_drawable_edit_fill_direct (drawable, options, undo_desc);

      ligma_drawable_update (drawable, x, y, width, height);
    }
  else
    {
      GeglNode               *operation;
      LigmaDrawableFilter     *filter;
      gdouble                 opacity;
      LigmaLayerMode           mode;
      LigmaLayerCompositeMode  composite_mode;

      opacity        = ligma_context_get_opacity (context);
      mode           = ligma_context_get_paint_mode (context);
      composite_mode = ligma_layer_mode_get_paint_composite_mode (mode);

      operation = gegl_node_new_child (NULL,
                                       "operation",        "ligma:fill-source",
                                       "options",          options,
                                       "drawable",         drawable,
                                       "pattern-offset-x", -x,
                                       "pattern-offset-y", -y,
                                       NULL);

      filter = ligma_drawable_filter_new (drawable, undo_desc, operation, NULL);

      ligma_drawable_filter_set_opacity (filter, opacity);
      ligma_drawable_filter_set_mode    (filter,
                                        mode,
                                        LIGMA_LAYER_COLOR_SPACE_AUTO,
                                        LIGMA_LAYER_COLOR_SPACE_AUTO,
                                        composite_mode);

      ligma_drawable_filter_apply  (filter, NULL);
      ligma_drawable_filter_commit (filter, NULL, FALSE);

      g_object_unref (filter);
      g_object_unref (operation);
    }
}
