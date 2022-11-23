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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-apply-operation.h"
#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligma-gegl-utils.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "ligma-utils.h"
#include "ligmabezierdesc.h"
#include "ligmachannel.h"
#include "ligmadrawable-fill.h"
#include "ligmaerror.h"
#include "ligmafilloptions.h"
#include "ligmaimage.h"
#include "ligmapattern.h"
#include "ligmapickable.h"
#include "ligmascanconvert.h"

#include "vectors/ligmavectors.h"

#include "ligma-intl.h"


/*  public functions  */

void
ligma_drawable_fill (LigmaDrawable *drawable,
                    LigmaContext  *context,
                    LigmaFillType  fill_type)
{
  LigmaRGB      color;
  LigmaPattern *pattern;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  if (fill_type == LIGMA_FILL_TRANSPARENT &&
      ! ligma_drawable_has_alpha (drawable))
    {
      fill_type = LIGMA_FILL_BACKGROUND;
    }

  if (! ligma_get_fill_params (context, fill_type, &color, &pattern, NULL))
    return;

  ligma_drawable_fill_buffer (drawable,
                             ligma_drawable_get_buffer (drawable),
                             &color, pattern, 0, 0);

  ligma_drawable_update (drawable, 0, 0, -1, -1);
}

void
ligma_drawable_fill_buffer (LigmaDrawable  *drawable,
                           GeglBuffer    *buffer,
                           const LigmaRGB *color,
                           LigmaPattern   *pattern,
                           gint           pattern_offset_x,
                           gint           pattern_offset_y)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (color != NULL || pattern != NULL);
  g_return_if_fail (pattern == NULL || LIGMA_IS_PATTERN (pattern));

  if (pattern)
    {
      GeglBuffer       *src_buffer;
      GeglBuffer       *dest_buffer;
      LigmaColorProfile *src_profile;
      LigmaColorProfile *dest_profile;

      src_buffer = ligma_pattern_create_buffer (pattern);

      src_profile  = ligma_babl_format_get_color_profile (
                       gegl_buffer_get_format (src_buffer));
      dest_profile = ligma_color_managed_get_color_profile (
                       LIGMA_COLOR_MANAGED (drawable));

      if (ligma_color_transform_can_gegl_copy (src_profile, dest_profile))
        {
          dest_buffer = g_object_ref (src_buffer);
        }
      else
        {
          dest_buffer = gegl_buffer_new (gegl_buffer_get_extent (src_buffer),
                                         gegl_buffer_get_format (buffer));

          ligma_gegl_convert_color_profile (
            src_buffer,  NULL, src_profile,
            dest_buffer, NULL, dest_profile,
            LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL,
            TRUE,
            NULL);
        }

      g_object_unref (src_profile);

      gegl_buffer_set_pattern (buffer, NULL, dest_buffer,
                               pattern_offset_x, pattern_offset_y);

      g_object_unref (src_buffer);
      g_object_unref (dest_buffer);
    }
  else
    {
      LigmaRGB    image_color;
      GeglColor *gegl_color;

      ligma_pickable_srgb_to_image_color (LIGMA_PICKABLE (drawable),
                                         color, &image_color);

      if (! ligma_drawable_has_alpha (drawable))
        ligma_rgb_set_alpha (&image_color, 1.0);

      gegl_color = ligma_gegl_color_new (&image_color,
                                        ligma_drawable_get_space (drawable));
      gegl_buffer_set_color (buffer, NULL, gegl_color);
      g_object_unref (gegl_color);
    }
}

void
ligma_drawable_fill_boundary (LigmaDrawable       *drawable,
                             LigmaFillOptions    *options,
                             const LigmaBoundSeg *bound_segs,
                             gint                n_bound_segs,
                             gint                offset_x,
                             gint                offset_y,
                             gboolean            push_undo)
{
  LigmaScanConvert *scan_convert;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (LIGMA_IS_FILL_OPTIONS (options));
  g_return_if_fail (bound_segs == NULL || n_bound_segs != 0);
  g_return_if_fail (ligma_fill_options_get_style (options) !=
                    LIGMA_FILL_STYLE_PATTERN ||
                    ligma_context_get_pattern (LIGMA_CONTEXT (options)) != NULL);

  scan_convert = ligma_scan_convert_new_from_boundary (bound_segs, n_bound_segs,
                                                      offset_x, offset_y);

  if (scan_convert)
    {
      ligma_drawable_fill_scan_convert (drawable, options,
                                       scan_convert, push_undo);
      ligma_scan_convert_free (scan_convert);
    }
}

gboolean
ligma_drawable_fill_vectors (LigmaDrawable     *drawable,
                            LigmaFillOptions  *options,
                            LigmaVectors      *vectors,
                            gboolean          push_undo,
                            GError          **error)
{
  const LigmaBezierDesc *bezier;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)), FALSE);
  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (options), FALSE);
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (ligma_fill_options_get_style (options) !=
                        LIGMA_FILL_STYLE_PATTERN ||
                        ligma_context_get_pattern (LIGMA_CONTEXT (options)) != NULL,
                        FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  bezier = ligma_vectors_get_bezier (vectors);

  if (bezier && bezier->num_data > 4)
    {
      LigmaScanConvert *scan_convert = ligma_scan_convert_new ();

      ligma_scan_convert_add_bezier (scan_convert, bezier);
      ligma_drawable_fill_scan_convert (drawable, options,
                                       scan_convert, push_undo);

      ligma_scan_convert_free (scan_convert);

      return TRUE;
    }

  g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                       _("Not enough points to fill"));

  return FALSE;
}

void
ligma_drawable_fill_scan_convert (LigmaDrawable    *drawable,
                                 LigmaFillOptions *options,
                                 LigmaScanConvert *scan_convert,
                                 gboolean         push_undo)
{
  LigmaContext *context;
  GeglBuffer  *buffer;
  GeglBuffer  *mask_buffer;
  gint         x, y, w, h;
  gint         off_x;
  gint         off_y;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (LIGMA_IS_FILL_OPTIONS (options));
  g_return_if_fail (scan_convert != NULL);
  g_return_if_fail (ligma_fill_options_get_style (options) !=
                    LIGMA_FILL_STYLE_PATTERN ||
                    ligma_context_get_pattern (LIGMA_CONTEXT (options)) != NULL);

  context = LIGMA_CONTEXT (options);

  if (! ligma_item_mask_intersect (LIGMA_ITEM (drawable), &x, &y, &w, &h))
    return;

  /* fill a 1-bpp GeglBuffer with black, this will describe the shape
   * of the stroke.
   */
  mask_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, w, h),
                                 babl_format ("Y u8"));

  /* render the stroke into it */
  ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

  ligma_scan_convert_render (scan_convert, mask_buffer,
                            x + off_x, y + off_y,
                            ligma_fill_options_get_antialias (options));

  buffer = ligma_fill_options_create_buffer (options, drawable,
                                            GEGL_RECTANGLE (0, 0, w, h),
                                            -x, -y);

  ligma_gegl_apply_opacity (buffer, NULL, NULL, buffer,
                           mask_buffer, 0, 0, 1.0);
  g_object_unref (mask_buffer);

  /* Apply to drawable */
  ligma_drawable_apply_buffer (drawable, buffer,
                              GEGL_RECTANGLE (0, 0, w, h),
                              push_undo, C_("undo-type", "Render Stroke"),
                              ligma_context_get_opacity (context),
                              ligma_context_get_paint_mode (context),
                              LIGMA_LAYER_COLOR_SPACE_AUTO,
                              LIGMA_LAYER_COLOR_SPACE_AUTO,
                              ligma_layer_mode_get_paint_composite_mode (
                                ligma_context_get_paint_mode (context)),
                              NULL, x, y);

  g_object_unref (buffer);

  ligma_drawable_update (drawable, x, y, w, h);
}
