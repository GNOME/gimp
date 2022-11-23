/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadrawable-stroke.c
 * Copyright (C) 2003 Simon Budig  <simon@ligma.org>
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligmachannel.h"
#include "ligmadrawable-fill.h"
#include "ligmadrawable-stroke.h"
#include "ligmaerror.h"
#include "ligmaimage.h"
#include "ligmascanconvert.h"
#include "ligmastrokeoptions.h"

#include "vectors/ligmavectors.h"

#include "ligma-intl.h"


/*  public functions  */

void
ligma_drawable_stroke_boundary (LigmaDrawable       *drawable,
                               LigmaStrokeOptions  *options,
                               const LigmaBoundSeg *bound_segs,
                               gint                n_bound_segs,
                               gint                offset_x,
                               gint                offset_y,
                               gboolean            push_undo)
{
  LigmaScanConvert *scan_convert;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (LIGMA_IS_STROKE_OPTIONS (options));
  g_return_if_fail (bound_segs == NULL || n_bound_segs != 0);
  g_return_if_fail (ligma_fill_options_get_style (LIGMA_FILL_OPTIONS (options)) !=
                    LIGMA_FILL_STYLE_PATTERN ||
                    ligma_context_get_pattern (LIGMA_CONTEXT (options)) != NULL);

  scan_convert = ligma_scan_convert_new_from_boundary (bound_segs, n_bound_segs,
                                                      offset_x, offset_y);

  if (scan_convert)
    {
      ligma_drawable_stroke_scan_convert (drawable, options,
                                         scan_convert, push_undo);
      ligma_scan_convert_free (scan_convert);
    }
}

gboolean
ligma_drawable_stroke_vectors (LigmaDrawable       *drawable,
                              LigmaStrokeOptions  *options,
                              LigmaVectors        *vectors,
                              gboolean            push_undo,
                              GError            **error)
{
  const LigmaBezierDesc *bezier;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)), FALSE);
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (options), FALSE);
  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), FALSE);
  g_return_val_if_fail (ligma_fill_options_get_style (LIGMA_FILL_OPTIONS (options)) !=
                        LIGMA_FILL_STYLE_PATTERN ||
                        ligma_context_get_pattern (LIGMA_CONTEXT (options)) != NULL,
                        FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  bezier = ligma_vectors_get_bezier (vectors);

  if (bezier && bezier->num_data >= 2)
    {
      LigmaScanConvert *scan_convert = ligma_scan_convert_new ();

      ligma_scan_convert_add_bezier (scan_convert, bezier);
      ligma_drawable_stroke_scan_convert (drawable, options,
                                         scan_convert, push_undo);

      ligma_scan_convert_free (scan_convert);

      return TRUE;
    }

  g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                       _("Not enough points to stroke"));

  return FALSE;
}

void
ligma_drawable_stroke_scan_convert (LigmaDrawable      *drawable,
                                   LigmaStrokeOptions *options,
                                   LigmaScanConvert   *scan_convert,
                                   gboolean           push_undo)
{
  gdouble  width;
  LigmaUnit unit;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (LIGMA_IS_STROKE_OPTIONS (options));
  g_return_if_fail (scan_convert != NULL);
  g_return_if_fail (ligma_fill_options_get_style (LIGMA_FILL_OPTIONS (options)) !=
                    LIGMA_FILL_STYLE_PATTERN ||
                    ligma_context_get_pattern (LIGMA_CONTEXT (options)) != NULL);

  if (! ligma_item_mask_intersect (LIGMA_ITEM (drawable), NULL, NULL, NULL, NULL))
    return;

  width = ligma_stroke_options_get_width (options);
  unit  = ligma_stroke_options_get_unit (options);

  if (unit != LIGMA_UNIT_PIXEL)
    {
      LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (drawable));
      gdouble    xres;
      gdouble    yres;

      ligma_image_get_resolution (image, &xres, &yres);

      ligma_scan_convert_set_pixel_ratio (scan_convert, yres / xres);

      width = ligma_units_to_pixels (width, unit, yres);
    }

  ligma_scan_convert_stroke (scan_convert, width,
                            ligma_stroke_options_get_join_style (options),
                            ligma_stroke_options_get_cap_style (options),
                            ligma_stroke_options_get_miter_limit (options),
                            ligma_stroke_options_get_dash_offset (options),
                            ligma_stroke_options_get_dash_info (options));

  ligma_drawable_fill_scan_convert (drawable, LIGMA_FILL_OPTIONS (options),
                                   scan_convert, push_undo);
}
