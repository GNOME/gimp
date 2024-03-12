/* GIMP - The GNU Image Manipulation Program
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

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gegl/gimp-gegl-loops.h"

#include "gimpchannel.h"
#include "core/gimpcontainer.h"
#include "gimpdrawable.h"
#include "gimpdrawable-edit.h"
#include "core/gimpdrawable-filters.h"
#include "gimpdrawablefilter.h"
#include "gimpcontext.h"
#include "gimpfilloptions.h"
#include "gimpimage.h"
#include "gimppattern.h"
#include "gimptempbuf.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static gboolean   gimp_drawable_edit_can_fill_direct (GimpDrawable    *drawable,
                                                      GimpFillOptions *options);
static void       gimp_drawable_edit_fill_direct     (GimpDrawable    *drawable,
                                                      GimpFillOptions *options,
                                                      const gchar     *undo_desc);


/*  private functions  */

static gboolean
gimp_drawable_edit_can_fill_direct (GimpDrawable    *drawable,
                                    GimpFillOptions *options)
{
  GimpImage                *image;
  GimpContext              *context;
  gdouble                   opacity;
  GimpComponentMask         affect;
  GimpLayerMode             mode;
  GimpLayerCompositeMode    composite_mode;
  GimpLayerCompositeRegion  composite_region;

  image            = gimp_item_get_image (GIMP_ITEM (drawable));
  context          = GIMP_CONTEXT (options);
  opacity          = gimp_context_get_opacity (context);
  affect           = gimp_drawable_get_active_mask (drawable);
  mode             = gimp_context_get_paint_mode (context);
  composite_mode   = gimp_layer_mode_get_paint_composite_mode (mode);
  composite_region = gimp_layer_mode_get_included_region (mode, composite_mode);

  if (gimp_channel_is_empty (gimp_image_get_mask (image)) &&
      opacity == GIMP_OPACITY_OPAQUE                      &&
      affect  == GIMP_COMPONENT_MASK_ALL                  &&
      gimp_layer_mode_is_trivial (mode)                   &&
      (! gimp_layer_mode_is_subtractive (mode)            ^
       ! (composite_region & GIMP_LAYER_COMPOSITE_REGION_SOURCE)))
    {
      switch (gimp_fill_options_get_style (options))
        {
        case GIMP_FILL_STYLE_FG_COLOR:
        case GIMP_FILL_STYLE_BG_COLOR:
          return TRUE;

        case GIMP_FILL_STYLE_PATTERN:
          {
            GimpPattern *pattern;
            GimpTempBuf *mask;
            const Babl  *format;

            pattern = gimp_context_get_pattern (context);
            mask    = gimp_pattern_get_mask (pattern);
            format  = gimp_temp_buf_get_format (mask);

            return ! babl_format_has_alpha (format);
          }
        }
    }

  return FALSE;
}

static void
gimp_drawable_edit_fill_direct (GimpDrawable    *drawable,
                                GimpFillOptions *options,
                                const gchar     *undo_desc)
{
  GeglBuffer    *buffer;
  GimpContext   *context;
  GimpLayerMode  mode;
  gint           width;
  gint           height;

  buffer  = gimp_drawable_get_buffer (drawable);
  context = GIMP_CONTEXT (options);
  mode    = gimp_context_get_paint_mode (context);
  width   = gimp_item_get_width  (GIMP_ITEM (drawable));
  height  = gimp_item_get_height (GIMP_ITEM (drawable));

  gimp_drawable_push_undo (drawable, undo_desc,
                           NULL, 0, 0, width, height);

  if (! gimp_layer_mode_is_subtractive (mode))
    gimp_fill_options_fill_buffer (options, drawable, buffer, 0, 0);
  else
    gimp_gegl_clear (buffer, NULL);
}


/*  public functions  */

void
gimp_drawable_edit_clear (GimpDrawable *drawable,
                          GimpContext  *context)
{
  GimpFillOptions *options;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  options = gimp_fill_options_new (context->gimp, NULL, FALSE);

  if (gimp_drawable_has_alpha (drawable))
    gimp_fill_options_set_by_fill_type (options, context,
                                        GIMP_FILL_TRANSPARENT, NULL);
  else
    gimp_fill_options_set_by_fill_type (options, context,
                                        GIMP_FILL_BACKGROUND, NULL);

  gimp_drawable_edit_fill (drawable, options, C_("undo-type", "Clear"));

  g_object_unref (options);
}

void
gimp_drawable_edit_fill (GimpDrawable    *drawable,
                         GimpFillOptions *options,
                         const gchar     *undo_desc)
{
  GimpContext *context;
  gint         x, y, width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_FILL_OPTIONS (options));

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                  &x, &y, &width, &height))
    {
      return;  /*  nothing to do, but the fill succeeded  */
    }

  context = GIMP_CONTEXT (options);

  if (gimp_layer_mode_is_alpha_only (gimp_context_get_paint_mode (context)))
    {
      if (! gimp_drawable_has_alpha (drawable) ||
          ! (gimp_drawable_get_active_mask (drawable) &
             GIMP_COMPONENT_MASK_ALPHA))
        {
          return;  /*  nothing to do, but the fill succeeded  */
        }
    }

  if (! undo_desc)
    undo_desc = gimp_fill_options_get_undo_desc (options);

  /* check if we can fill the drawable's buffer directly */
  if (gimp_drawable_edit_can_fill_direct (drawable, options))
    {
      gimp_drawable_edit_fill_direct (drawable, options, undo_desc);

      gimp_drawable_update (drawable, x, y, width, height);
    }
  else
    {
      GeglNode               *operation;
      GimpDrawableFilter     *filter;
      gdouble                 opacity;
      GimpLayerMode           mode;
      GimpLayerCompositeMode  composite_mode;
      GimpContainer          *filter_stack;

      opacity        = gimp_context_get_opacity (context);
      mode           = gimp_context_get_paint_mode (context);
      composite_mode = gimp_layer_mode_get_paint_composite_mode (mode);

      operation = gegl_node_new_child (NULL,
                                       "operation",        "gimp:fill-source",
                                       "options",          options,
                                       "drawable",         drawable,
                                       "pattern-offset-x", -x,
                                       "pattern-offset-y", -y,
                                       NULL);

      filter = gimp_drawable_filter_new (drawable, undo_desc, operation, NULL);

      gimp_drawable_filter_set_opacity (filter, opacity);
      gimp_drawable_filter_set_mode    (filter,
                                        mode,
                                        GIMP_LAYER_COLOR_SPACE_AUTO,
                                        GIMP_LAYER_COLOR_SPACE_AUTO,
                                        composite_mode);

      gimp_drawable_filter_apply  (filter, NULL);
      /* Move to bottom of filter stack */
      filter_stack = gimp_drawable_get_filters (drawable);
      if (filter_stack)
        gimp_container_reorder (filter_stack, GIMP_OBJECT (filter),
                                gimp_container_get_n_children (filter_stack) - 1);

      gimp_drawable_filter_commit (filter, FALSE, NULL, FALSE);

      g_object_unref (filter);
      g_object_unref (operation);
    }
}
