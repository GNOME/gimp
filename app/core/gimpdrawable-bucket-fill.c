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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-mask.h"
#include "gegl/gimp-gegl-mask-combine.h"
#include "gegl/gimp-gegl-utils.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpdrawable.h"
#include "gimpdrawable-bucket-fill.h"
#include "gimpfilloptions.h"
#include "gimpimage.h"
#include "gimppickable.h"
#include "gimppickable-contiguous-region.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_drawable_bucket_fill (GimpDrawable         *drawable,
                           GimpFillOptions      *options,
                           gboolean              fill_transparent,
                           GimpSelectCriterion   fill_criterion,
                           gdouble               threshold,
                           gboolean              sample_merged,
                           gboolean              diagonal_neighbors,
                           gdouble               seed_x,
                           gdouble               seed_y)
{
  GimpImage    *image;
  GimpPickable *pickable;
  GeglBuffer   *buffer;
  GeglBuffer   *mask_buffer;
  gboolean      antialias;
  gint          x, y, width, height;
  gint          mask_offset_x = 0;
  gint          mask_offset_y = 0;
  gint          sel_x, sel_y, sel_width, sel_height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_FILL_OPTIONS (options));

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                  &sel_x, &sel_y, &sel_width, &sel_height))
    return;

  gimp_set_busy (image->gimp);

  if (sample_merged)
    pickable = GIMP_PICKABLE (image);
  else
    pickable = GIMP_PICKABLE (drawable);

  antialias = gimp_fill_options_get_antialias (options);

  /*  Do a seed bucket fill...To do this, calculate a new
   *  contiguous region.
   */
  mask_buffer = gimp_pickable_contiguous_region_by_seed (pickable,
                                                         antialias,
                                                         threshold,
                                                         fill_transparent,
                                                         fill_criterion,
                                                         diagonal_neighbors,
                                                         (gint) seed_x,
                                                         (gint) seed_y);

  gimp_gegl_mask_bounds (mask_buffer, &x, &y, &width, &height);
  width  -= x;
  height -= y;

  /*  If there is a selection, inersect the region bounds
   *  with the selection bounds, to avoid processing areas
   *  that are going to be masked out anyway.  The actual
   *  intersection of the fill region with the mask data
   *  happens when combining the fill buffer, in
   *  gimp_drawable_apply_buffer().
   */
  if (! gimp_channel_is_empty (gimp_image_get_mask (image)))
    {
      gint off_x = 0;
      gint off_y = 0;

      if (sample_merged)
        gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

      if (! gimp_rectangle_intersect (x, y, width, height,

                                      sel_x + off_x, sel_y + off_y,
                                      sel_width,     sel_height,

                                      &x, &y, &width, &height))
        {
          /*  The fill region and the selection are disjoint; bail.  */

          g_object_unref (mask_buffer);

          gimp_unset_busy (image->gimp);

          return;
        }
    }

  /*  make sure we handle the mask correctly if it was sample-merged  */
  if (sample_merged)
    {
      GimpItem *item = GIMP_ITEM (drawable);
      gint      off_x, off_y;

      /*  Limit the channel bounds to the drawable's extents  */
      gimp_item_get_offset (item, &off_x, &off_y);

      gimp_rectangle_intersect (x, y, width, height,

                                off_x, off_y,
                                gimp_item_get_width (item),
                                gimp_item_get_height (item),

                                &x, &y, &width, &height);

      mask_offset_x = x;
      mask_offset_y = y;

     /*  translate mask bounds to drawable coords  */
      x -= off_x;
      y -= off_y;
    }
  else
    {
      mask_offset_x = x;
      mask_offset_y = y;
    }

  buffer = gimp_fill_options_create_buffer (options, drawable,
                                            GEGL_RECTANGLE (0, 0,
                                                            width, height));

  gimp_gegl_apply_opacity (buffer, NULL, NULL, buffer,
                           mask_buffer,
                           -mask_offset_x,
                           -mask_offset_y,
                           1.0);
  g_object_unref (mask_buffer);

  /*  Apply it to the image  */
  gimp_drawable_apply_buffer (drawable, buffer,
                              GEGL_RECTANGLE (0, 0, width, height),
                              TRUE, C_("undo-type", "Bucket Fill"),
                              gimp_context_get_opacity (GIMP_CONTEXT (options)),
                              gimp_context_get_paint_mode (GIMP_CONTEXT (options)),
                              GIMP_LAYER_COLOR_SPACE_AUTO,
                              GIMP_LAYER_COLOR_SPACE_AUTO,
                              gimp_layer_mode_get_paint_composite_mode (
                                gimp_context_get_paint_mode (GIMP_CONTEXT (options))),
                              NULL, x, y);

  g_object_unref (buffer);

  gimp_drawable_update (drawable, x, y, width, height);

  gimp_unset_busy (image->gimp);
}
