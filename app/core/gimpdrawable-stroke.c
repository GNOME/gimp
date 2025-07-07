/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawable-stroke.c
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpchannel.h"
#include "gimpdrawable-fill.h"
#include "gimpdrawable-stroke.h"
#include "gimperror.h"
#include "gimpimage.h"
#include "gimpscanconvert.h"
#include "gimpstrokeoptions.h"

#include "path/gimppath.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_drawable_stroke_boundary (GimpDrawable       *drawable,
                               GimpStrokeOptions  *options,
                               const GimpBoundSeg *bound_segs,
                               gint                n_bound_segs,
                               gint                offset_x,
                               gint                offset_y,
                               gboolean            push_undo)
{
  GimpScanConvert *scan_convert;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_STROKE_OPTIONS (options));
  g_return_if_fail (bound_segs == NULL || n_bound_segs != 0);
  g_return_if_fail (gimp_fill_options_get_style (GIMP_FILL_OPTIONS (options)) !=
                    GIMP_FILL_STYLE_PATTERN ||
                    gimp_context_get_pattern (GIMP_CONTEXT (options)) != NULL);

  scan_convert = gimp_scan_convert_new_from_boundary (bound_segs, n_bound_segs,
                                                      offset_x, offset_y);

  if (scan_convert)
    {
      gimp_drawable_stroke_scan_convert (drawable, options,
                                         scan_convert, push_undo);
      gimp_scan_convert_free (scan_convert);
    }
}

gboolean
gimp_drawable_stroke_path (GimpDrawable       *drawable,
                           GimpStrokeOptions  *options,
                           GimpPath           *path,
                           gboolean            push_undo,
                           GError            **error)
{
  const GimpBezierDesc *bezier;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options), FALSE);
  g_return_val_if_fail (GIMP_IS_PATH (path), FALSE);
  g_return_val_if_fail (gimp_fill_options_get_style (GIMP_FILL_OPTIONS (options)) !=
                        GIMP_FILL_STYLE_PATTERN ||
                        gimp_context_get_pattern (GIMP_CONTEXT (options)) != NULL,
                        FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  bezier = gimp_path_get_bezier (path);

  if (bezier && bezier->num_data >= 2)
    {
      GimpScanConvert *scan_convert = gimp_scan_convert_new ();

      gimp_scan_convert_add_bezier (scan_convert, bezier);
      gimp_drawable_stroke_scan_convert (drawable, options,
                                         scan_convert, push_undo);

      gimp_scan_convert_free (scan_convert);

      return TRUE;
    }

  g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                       _("Not enough points to stroke"));

  return FALSE;
}

void
gimp_drawable_stroke_scan_convert (GimpDrawable      *drawable,
                                   GimpStrokeOptions *options,
                                   GimpScanConvert   *scan_convert,
                                   gboolean           push_undo)
{
  gdouble   width;
  GimpUnit *unit;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_STROKE_OPTIONS (options));
  g_return_if_fail (scan_convert != NULL);
  g_return_if_fail (gimp_fill_options_get_style (GIMP_FILL_OPTIONS (options)) !=
                    GIMP_FILL_STYLE_PATTERN ||
                    gimp_context_get_pattern (GIMP_CONTEXT (options)) != NULL);

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable), NULL, NULL, NULL, NULL))
    return;

  width = gimp_stroke_options_get_width (options);
  unit  = gimp_stroke_options_get_unit (options);

  if (unit != gimp_unit_pixel ())
    {
      GimpImage *image = gimp_item_get_image (GIMP_ITEM (drawable));
      gdouble    xres;
      gdouble    yres;

      gimp_image_get_resolution (image, &xres, &yres);

      gimp_scan_convert_set_pixel_ratio (scan_convert, yres / xres);

      width = gimp_units_to_pixels (width, unit, yres);
    }

  gimp_scan_convert_stroke (scan_convert, width,
                            gimp_stroke_options_get_join_style (options),
                            gimp_stroke_options_get_cap_style (options),
                            gimp_stroke_options_get_miter_limit (options),
                            gimp_stroke_options_get_dash_offset (options),
                            gimp_stroke_options_get_dash_info (options));

  gimp_drawable_fill_scan_convert (drawable, GIMP_FILL_OPTIONS (options),
                                   scan_convert, push_undo);
}
