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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-mask.h"
#include "gegl/gimp-gegl-mask-combine.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimp-palettes.h"
#include "gimp-utils.h"
#include "gimpchannel-combine.h"
#include "gimpcontext.h"
#include "gimpdrawable.h"
#include "gimpdrawable-bucket-fill.h"
#include "gimpimage.h"
#include "gimppattern.h"
#include "gimppickable.h"
#include "gimppickable-contiguous-region.h"

#include "gimp-intl.h"


static void   gimp_drawable_bucket_fill_internal (GimpDrawable        *drawable,
                                                  GimpFillType         fill_type,
                                                  gint                 paint_mode,
                                                  gdouble              opacity,
                                                  gboolean             fill_transparent,
                                                  GimpSelectCriterion  fill_criterion,
                                                  gdouble              threshold,
                                                  gboolean             sample_merged,
                                                  gdouble              x,
                                                  gdouble              y,
                                                  const GimpRGB       *color,
                                                  GimpPattern         *pattern);


/*  public functions  */

gboolean
gimp_drawable_bucket_fill (GimpDrawable         *drawable,
                           GimpContext          *context,
                           GimpFillType          fill_type,
                           gint                  paint_mode,
                           gdouble               opacity,
                           gboolean              fill_transparent,
                           GimpSelectCriterion   fill_criterion,
                           gdouble               threshold,
                           gboolean              sample_merged,
                           gdouble               x,
                           gdouble               y,
                           GError              **error)
{
  GimpPattern *pattern;
  GimpRGB      color;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! gimp_get_fill_params (context, fill_type, &color, &pattern, error))
    return FALSE;

  gimp_palettes_add_color_history (context->gimp,
                                   &color);

  gimp_drawable_bucket_fill_internal (drawable,
                                      fill_type,
                                      paint_mode, opacity,
                                      fill_transparent, fill_criterion,
                                      threshold, sample_merged,
                                      x, y,
                                      &color, pattern);

  return TRUE;
}


/*  private functions  */

static void
gimp_drawable_bucket_fill_internal (GimpDrawable        *drawable,
                                    GimpFillType         fill_type,
                                    gint                 paint_mode,
                                    gdouble              opacity,
                                    gboolean             fill_transparent,
                                    GimpSelectCriterion  fill_criterion,
                                    gdouble              threshold,
                                    gboolean             sample_merged,
                                    gdouble              x,
                                    gdouble              y,
                                    const GimpRGB       *color,
                                    GimpPattern         *pattern)
{
  GimpImage    *image;
  GimpPickable *pickable;
  GeglBuffer   *buffer;
  GeglBuffer   *mask_buffer;
  gint          x1, y1, x2, y2;
  gint          mask_offset_x = 0;
  gint          mask_offset_y = 0;
  gboolean      selection;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (fill_type != GIMP_FILL_PATTERN ||
                    GIMP_IS_PATTERN (pattern));
  g_return_if_fail (fill_type == GIMP_FILL_PATTERN ||
                    color != NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  selection = gimp_item_mask_bounds (GIMP_ITEM (drawable), &x1, &y1, &x2, &y2);

  if ((x1 == x2) || (y1 == y2))
    return;

  gimp_set_busy (image->gimp);

  if (sample_merged)
    pickable = GIMP_PICKABLE (image);
  else
    pickable = GIMP_PICKABLE (drawable);

  /*  Do a seed bucket fill...To do this, calculate a new
   *  contiguous region. If there is a selection, calculate the
   *  intersection of this region with the existing selection.
   */
  mask_buffer = gimp_pickable_contiguous_region_by_seed (pickable,
                                                         TRUE,
                                                         threshold,
                                                         fill_transparent,
                                                         fill_criterion,
                                                         FALSE /* no diagonal neighbors */,
                                                         (gint) x,
                                                         (gint) y);

  if (selection)
    {
      GimpDrawable *sel;
      gint          off_x = 0;
      gint          off_y = 0;

      if (! sample_merged)
        gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

      sel = GIMP_DRAWABLE (gimp_image_get_mask (image));

      gimp_gegl_mask_combine_buffer (mask_buffer,
                                     gimp_drawable_get_buffer (sel),
                                     GIMP_CHANNEL_OP_INTERSECT,
                                     -off_x, -off_y);
    }

  gimp_gegl_mask_bounds (mask_buffer, &x1, &y1, &x2, &y2);

  /*  make sure we handle the mask correctly if it was sample-merged  */
  if (sample_merged)
    {
      GimpItem *item = GIMP_ITEM (drawable);
      gint      off_x, off_y;

      /*  Limit the channel bounds to the drawable's extents  */
      gimp_item_get_offset (item, &off_x, &off_y);

      x1 = CLAMP (x1, off_x, (off_x + gimp_item_get_width (item)));
      y1 = CLAMP (y1, off_y, (off_y + gimp_item_get_height (item)));
      x2 = CLAMP (x2, off_x, (off_x + gimp_item_get_width (item)));
      y2 = CLAMP (y2, off_y, (off_y + gimp_item_get_height (item)));

      mask_offset_x = x1;
      mask_offset_y = y1;

     /*  translate mask bounds to drawable coords  */
      x1 -= off_x;
      y1 -= off_y;
      x2 -= off_x;
      y2 -= off_y;
    }
  else
    {
      mask_offset_x = x1;
      mask_offset_y = y1;
    }

  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, x2 - x1, y2 - y1),
                            gimp_drawable_get_format_with_alpha (drawable));

  switch (fill_type)
    {
    case GIMP_FILL_FOREGROUND:
    case GIMP_FILL_BACKGROUND:
    case GIMP_FILL_WHITE:
    case GIMP_FILL_TRANSPARENT:
      {
        GeglColor *gegl_color = gimp_gegl_color_new (color);

        gegl_buffer_set_color (buffer, NULL, gegl_color);
        g_object_unref (gegl_color);
      }
      break;

    case GIMP_FILL_PATTERN:
      {
        GeglBuffer *pattern_buffer = gimp_pattern_create_buffer (pattern);

        gegl_buffer_set_pattern (buffer, NULL, pattern_buffer, -x1, -y1);
        g_object_unref (pattern_buffer);
      }
      break;
    }

  gimp_gegl_apply_opacity (buffer, NULL, NULL, buffer,
                           mask_buffer,
                           -mask_offset_x,
                           -mask_offset_y,
                           1.0);
  g_object_unref (mask_buffer);

  /*  Apply it to the image  */
  gimp_drawable_apply_buffer (drawable, buffer,
                              GEGL_RECTANGLE (0, 0, x2 - x1, y2 - y1),
                              TRUE, C_("undo-type", "Bucket Fill"),
                              opacity, paint_mode,
                              NULL, x1, y1);

  g_object_unref (buffer);

  gimp_drawable_update (drawable, x1, y1, x2 - x1, y2 - y1);

  gimp_unset_busy (image->gimp);
}
