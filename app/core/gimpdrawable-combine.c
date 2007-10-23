/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimpchannel.h"
#include "gimpdrawable-combine.h"
#include "gimpdrawableundo.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"


void
gimp_drawable_real_apply_region (GimpDrawable         *drawable,
                                 PixelRegion          *src2PR,
                                 gboolean              push_undo,
                                 const gchar          *undo_desc,
                                 gdouble               opacity,
                                 GimpLayerModeEffects  mode,
                                 TileManager          *src1_tiles,
                                 gint                  x,
                                 gint                  y)
{
  GimpImage       *image;
  GimpItem        *item;
  GimpChannel     *mask;
  gint             x1, y1, x2, y2;
  gint             offset_x, offset_y;
  PixelRegion      src1PR, destPR;
  CombinationMode  operation;
  gboolean         active_components[MAX_CHANNELS];

  item = GIMP_ITEM (drawable);

  image = gimp_item_get_image (item);

  mask = gimp_image_get_mask (image);

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (GIMP_DRAWABLE (mask) == drawable || gimp_channel_is_empty (mask))
    mask = NULL;

  /*  configure the active channel array  */
  gimp_drawable_get_active_components (drawable, active_components);

  /*  determine what sort of operation is being attempted and
   *  if it's actually legal...
   */
  operation = gimp_image_get_combination_mode (gimp_drawable_type (drawable),
                                               src2PR->bytes);
  if (operation == -1)
    {
      g_warning ("%s: illegal parameters.", G_STRFUNC);
      return;
    }

  /*  get the layer offsets  */
  gimp_item_offsets (item, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within drawable bounds  */
  x1 = CLAMP (x,             0, gimp_item_width (item));
  y1 = CLAMP (y,             0, gimp_item_height (item));
  x2 = CLAMP (x + src2PR->w, 0, gimp_item_width (item));
  y2 = CLAMP (y + src2PR->h, 0, gimp_item_height (item));

  if (mask)
    {
      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      x1 = CLAMP (x1,
                  - offset_x, gimp_item_width  (GIMP_ITEM (mask)) - offset_x);
      y1 = CLAMP (y1,
                  - offset_y, gimp_item_height (GIMP_ITEM (mask)) - offset_y);
      x2 = CLAMP (x2,
                  - offset_x, gimp_item_width  (GIMP_ITEM (mask)) - offset_x);
      y2 = CLAMP (y2,
                  - offset_y, gimp_item_height (GIMP_ITEM (mask)) - offset_y);
    }

  /*  If the calling procedure specified an undo step...  */
  if (push_undo)
    {
      GimpDrawableUndo *undo;

      gimp_drawable_push_undo (drawable,
                               undo_desc, x1, y1, x2, y2, NULL, FALSE);

      undo = GIMP_DRAWABLE_UNDO (gimp_image_undo_get_fadeable (image));

      if (undo)
        {
          PixelRegion tmp_srcPR;
          PixelRegion tmp_destPR;

          undo->paint_mode = mode;
          undo->opacity    = opacity;
          undo->src2_tiles = tile_manager_new (x2 - x1, y2 - y1,
                                               src2PR->bytes);

          tmp_srcPR = *src2PR;
          pixel_region_resize (&tmp_srcPR,
                               src2PR->x + (x1 - x), src2PR->y + (y1 - y),
                               x2 - x1, y2 - y1);
          pixel_region_init (&tmp_destPR, undo->src2_tiles,
                             0, 0,
                             x2 - x1, y2 - y1, TRUE);

          copy_region (&tmp_srcPR, &tmp_destPR);
        }
    }

  /* configure the pixel regions
   *  If an alternative to using the drawable's data as src1 was provided...
   */
  if (src1_tiles)
    pixel_region_init (&src1PR, src1_tiles,
                       x1, y1, (x2 - x1), (y2 - y1), FALSE);
  else
    pixel_region_init (&src1PR, gimp_drawable_get_tiles (drawable),
                       x1, y1, (x2 - x1), (y2 - y1), FALSE);

  pixel_region_init (&destPR, gimp_drawable_get_tiles (drawable),
                     x1, y1, (x2 - x1), (y2 - y1), TRUE);
  pixel_region_resize (src2PR,
                       src2PR->x + (x1 - x), src2PR->y + (y1 - y),
                       (x2 - x1), (y2 - y1));

  if (mask)
    {
      PixelRegion maskPR;

      pixel_region_init (&maskPR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                         x1 + offset_x,
                         y1 + offset_y,
                         (x2 - x1), (y2 - y1),
                         FALSE);

      combine_regions (&src1PR, src2PR, &destPR, &maskPR, NULL,
                       opacity * 255.999,
                       mode,
                       active_components,
                       operation);
    }
  else
    {
      combine_regions (&src1PR, src2PR, &destPR, NULL, NULL,
                       opacity * 255.999,
                       mode,
                       active_components,
                       operation);
    }
}

/*  Similar to gimp_drawable_apply_region but works in "replace" mode (i.e.
 *  transparent pixels in src2 make the result transparent rather than
 *  opaque.
 *
 * Takes an additional mask pixel region as well.
 */
void
gimp_drawable_real_replace_region (GimpDrawable *drawable,
                                   PixelRegion  *src2PR,
                                   gboolean      push_undo,
                                   const gchar  *undo_desc,
                                   gdouble       opacity,
                                   PixelRegion  *maskPR,
                                   gint          x,
                                   gint          y)
{
  GimpImage       *image;
  GimpItem        *item;
  GimpChannel     *mask;
  gint             x1, y1, x2, y2;
  gint             offset_x, offset_y;
  PixelRegion      src1PR, destPR;
  CombinationMode  operation;
  gboolean         active_components[MAX_CHANNELS];

  item = GIMP_ITEM (drawable);

  image = gimp_item_get_image (item);

  mask = gimp_image_get_mask (image);

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (GIMP_DRAWABLE (mask) == drawable || gimp_channel_is_empty (mask))
    mask = NULL;

  /*  configure the active channel array  */
  gimp_drawable_get_active_components (drawable, active_components);

  /*  determine what sort of operation is being attempted and
   *  if it's actually legal...
   */
  operation = gimp_image_get_combination_mode (gimp_drawable_type (drawable),
                                               src2PR->bytes);
  if (operation == -1)
    {
      g_warning ("%s: illegal parameters.", G_STRFUNC);
      return;
    }

  /*  get the layer offsets  */
  gimp_item_offsets (item, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within drawable bounds  */
  x1 = CLAMP (x, 0,             gimp_item_width (item));
  y1 = CLAMP (y, 0,             gimp_item_height (item));
  x2 = CLAMP (x + src2PR->w, 0, gimp_item_width (item));
  y2 = CLAMP (y + src2PR->h, 0, gimp_item_height (item));

  if (mask)
    {
      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      x1 = CLAMP (x1,
                  - offset_x, gimp_item_width  (GIMP_ITEM (mask)) - offset_x);
      y1 = CLAMP (y1,
                  - offset_y, gimp_item_height (GIMP_ITEM (mask)) - offset_y);
      x2 = CLAMP (x2,
                  - offset_x, gimp_item_width  (GIMP_ITEM (mask)) - offset_x);
      y2 = CLAMP (y2,
                  - offset_y, gimp_item_height (GIMP_ITEM (mask)) - offset_y);
    }

  /*  If the calling procedure specified an undo step...  */
  if (push_undo)
    gimp_drawable_push_undo (drawable, undo_desc, x1, y1, x2, y2, NULL, FALSE);

  /* configure the pixel regions
   *  If an alternative to using the drawable's data as src1 was provided...
   */
  pixel_region_init (&src1PR, gimp_drawable_get_tiles (drawable),
                     x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, gimp_drawable_get_tiles (drawable),
                     x1, y1, (x2 - x1), (y2 - y1), TRUE);
  pixel_region_resize (src2PR,
                       src2PR->x + (x1 - x), src2PR->y + (y1 - y),
                       (x2 - x1), (y2 - y1));

  if (mask)
    {
      PixelRegion  mask2PR, tempPR;
      guchar      *temp_data;

      pixel_region_init (&mask2PR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                         x1 + offset_x,
                         y1 + offset_y,
                         (x2 - x1), (y2 - y1),
                         FALSE);

      temp_data = g_malloc ((y2 - y1) * (x2 - x1));

      pixel_region_init_data (&tempPR, temp_data, 1, x2 - x1,
                              0, 0, x2 - x1, y2 - y1);

      copy_region (&mask2PR, &tempPR);

      pixel_region_init_data (&tempPR, temp_data, 1, x2 - x1,
                              0, 0, x2 - x1, y2 - y1);

      apply_mask_to_region (&tempPR, maskPR, OPAQUE_OPACITY);

      pixel_region_init_data (&tempPR, temp_data, 1, x2 - x1,
                              0, 0, x2 - x1, y2 - y1);

      combine_regions_replace (&src1PR, src2PR, &destPR, &tempPR, NULL,
                               opacity * 255.999,
                               active_components,
                               operation);

      g_free (temp_data);
    }
  else
    {
      combine_regions_replace (&src1PR, src2PR, &destPR, maskPR, NULL,
                               opacity * 255.999,
                               active_components,
                               operation);
    }
}
