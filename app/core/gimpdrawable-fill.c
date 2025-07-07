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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gimp-utils.h"
#include "gimpbezierdesc.h"
#include "gimpchannel.h"
#include "gimpdrawable-fill.h"
#include "gimperror.h"
#include "gimpfilloptions.h"
#include "gimpimage.h"
#include "gimppattern.h"
#include "gimppickable.h"
#include "gimpscanconvert.h"

#include "path/gimppath.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_drawable_fill (GimpDrawable *drawable,
                    GimpContext  *context,
                    GimpFillType  fill_type)
{
  GeglColor   *color;
  GimpPattern *pattern;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  if (fill_type == GIMP_FILL_TRANSPARENT &&
      ! gimp_drawable_has_alpha (drawable))
    {
      fill_type = GIMP_FILL_BACKGROUND;
    }

  if (! gimp_get_fill_params (context, fill_type, &color, &pattern, NULL))
    return;

  gimp_drawable_fill_buffer (drawable,
                             gimp_drawable_get_buffer (drawable),
                             color, pattern, 0, 0);

  gimp_drawable_update (drawable, 0, 0, -1, -1);

  g_clear_object (&color);
}

void
gimp_drawable_fill_buffer (GimpDrawable  *drawable,
                           GeglBuffer    *buffer,
                           GeglColor     *color,
                           GimpPattern   *pattern,
                           gint           pattern_offset_x,
                           gint           pattern_offset_y)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (GEGL_IS_COLOR (color) || pattern != NULL);
  g_return_if_fail (pattern == NULL || GIMP_IS_PATTERN (pattern));

  if (pattern)
    {
      GeglBuffer       *src_buffer;
      GeglBuffer       *dest_buffer;
      GimpColorProfile *src_profile;
      GimpColorProfile *dest_profile;

      src_buffer = gimp_pattern_create_buffer (pattern);

      src_profile  = gimp_babl_format_get_color_profile (
                       gegl_buffer_get_format (src_buffer));
      dest_profile = gimp_color_managed_get_color_profile (
                       GIMP_COLOR_MANAGED (drawable));

      if (gimp_color_transform_can_gegl_copy (src_profile, dest_profile))
        {
          dest_buffer = g_object_ref (src_buffer);
        }
      else
        {
          dest_buffer = gegl_buffer_new (gegl_buffer_get_extent (src_buffer),
                                         gegl_buffer_get_format (buffer));

          gimp_gegl_convert_color_profile (
            src_buffer,  NULL, src_profile,
            dest_buffer, NULL, dest_profile,
            GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
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
      if (! gimp_drawable_has_alpha (drawable))
        {
          color = gegl_color_duplicate (color);
          gimp_color_set_alpha (color, 1.0);
        }

      gegl_buffer_set_color (buffer, NULL, color);

      if (! gimp_drawable_has_alpha (drawable))
        g_object_unref (color);
    }
}

void
gimp_drawable_fill_boundary (GimpDrawable       *drawable,
                             GimpFillOptions    *options,
                             const GimpBoundSeg *bound_segs,
                             gint                n_bound_segs,
                             gint                offset_x,
                             gint                offset_y,
                             gboolean            push_undo)
{
  GimpScanConvert *scan_convert;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_FILL_OPTIONS (options));
  g_return_if_fail (bound_segs == NULL || n_bound_segs != 0);
  g_return_if_fail (gimp_fill_options_get_style (options) !=
                    GIMP_FILL_STYLE_PATTERN ||
                    gimp_context_get_pattern (GIMP_CONTEXT (options)) != NULL);

  scan_convert = gimp_scan_convert_new_from_boundary (bound_segs, n_bound_segs,
                                                      offset_x, offset_y);

  if (scan_convert)
    {
      gimp_drawable_fill_scan_convert (drawable, options,
                                       scan_convert, push_undo);
      gimp_scan_convert_free (scan_convert);
    }
}

gboolean
gimp_drawable_fill_path (GimpDrawable     *drawable,
                         GimpFillOptions  *options,
                         GimpPath         *path,
                         gboolean          push_undo,
                         GError          **error)
{
  const GimpBezierDesc *bezier;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), FALSE);
  g_return_val_if_fail (GIMP_IS_PATH (path), FALSE);
  g_return_val_if_fail (gimp_fill_options_get_style (options) !=
                        GIMP_FILL_STYLE_PATTERN ||
                        gimp_context_get_pattern (GIMP_CONTEXT (options)) != NULL,
                        FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  bezier = gimp_path_get_bezier (path);

  if (bezier && bezier->num_data > 4)
    {
      GimpScanConvert *scan_convert = gimp_scan_convert_new ();

      gimp_scan_convert_add_bezier (scan_convert, bezier);
      gimp_drawable_fill_scan_convert (drawable, options,
                                       scan_convert, push_undo);

      gimp_scan_convert_free (scan_convert);

      return TRUE;
    }

  g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                       _("Not enough points to fill"));

  return FALSE;
}

void
gimp_drawable_fill_scan_convert (GimpDrawable    *drawable,
                                 GimpFillOptions *options,
                                 GimpScanConvert *scan_convert,
                                 gboolean         push_undo)
{
  GimpContext *context;
  GeglBuffer  *buffer;
  GeglBuffer  *mask_buffer;
  gint         x, y, w, h;
  gint         off_x;
  gint         off_y;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_FILL_OPTIONS (options));
  g_return_if_fail (scan_convert != NULL);
  g_return_if_fail (gimp_fill_options_get_style (options) !=
                    GIMP_FILL_STYLE_PATTERN ||
                    gimp_context_get_pattern (GIMP_CONTEXT (options)) != NULL);

  context = GIMP_CONTEXT (options);

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable), &x, &y, &w, &h))
    return;

  /* fill a 1-bpp GeglBuffer with black, this will describe the shape
   * of the stroke.
   */
  mask_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, w, h),
                                 babl_format ("Y u8"));

  /* render the stroke into it */
  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

  gimp_scan_convert_render (scan_convert, mask_buffer,
                            x + off_x, y + off_y,
                            gimp_fill_options_get_antialias (options));

  buffer = gimp_fill_options_create_buffer (options, drawable,
                                            GEGL_RECTANGLE (0, 0, w, h),
                                            -x, -y);

  gimp_gegl_apply_opacity (buffer, NULL, NULL, buffer,
                           mask_buffer, 0, 0, 1.0);
  g_object_unref (mask_buffer);

  /* Apply to drawable */
  gimp_drawable_apply_buffer (drawable, buffer,
                              GEGL_RECTANGLE (0, 0, w, h),
                              push_undo, C_("undo-type", "Render Stroke"),
                              gimp_context_get_opacity (context),
                              gimp_context_get_paint_mode (context),
                              GIMP_LAYER_COLOR_SPACE_AUTO,
                              GIMP_LAYER_COLOR_SPACE_AUTO,
                              gimp_layer_mode_get_paint_composite_mode (
                                gimp_context_get_paint_mode (context)),
                              NULL, x, y);

  g_object_unref (buffer);

  gimp_drawable_update (drawable, x, y, w, h);
}
