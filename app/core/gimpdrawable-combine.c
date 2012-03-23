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

#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimpchannel.h"
#include "gimpdrawable-combine.h"
#include "gimpdrawableundo.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"


void
gimp_drawable_real_apply_buffer (GimpDrawable         *drawable,
                                 GeglBuffer           *buffer,
                                 const GeglRectangle  *buffer_region,
                                 gboolean              push_undo,
                                 const gchar          *undo_desc,
                                 gdouble               opacity,
                                 GimpLayerModeEffects  mode,
                                 GeglBuffer           *base_buffer,
                                 PixelRegion          *destPR,
                                 gint                  dest_x,
                                 gint                  dest_y)
{
  GimpItem        *item  = GIMP_ITEM (drawable);
  GimpImage       *image = gimp_item_get_image (item);
  GimpChannel     *mask  = gimp_image_get_mask (image);
  TempBuf         *temp_buf;
  PixelRegion      src2PR;
  gint             x, y, width, height;
  gint             offset_x, offset_y;
  PixelRegion      src1PR, my_destPR;
  CombinationMode  operation;
  gboolean         active_components[MAX_CHANNELS];

  temp_buf = gimp_gegl_buffer_get_temp_buf (buffer);

  if (temp_buf)
    {
      pixel_region_init_temp_buf (&src2PR, temp_buf,
                                  buffer_region->x, buffer_region->y,
                                  buffer_region->width, buffer_region->height);
    }
  else
    {
      pixel_region_init (&src2PR, gimp_gegl_buffer_get_tiles (buffer),
                         buffer_region->x, buffer_region->y,
                         buffer_region->width, buffer_region->height,
                         FALSE);
    }

  if (! base_buffer)
    base_buffer = gimp_drawable_get_buffer (drawable);

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (GIMP_DRAWABLE (mask) == drawable || gimp_channel_is_empty (mask))
    mask = NULL;

  /*  configure the active channel array  */
  gimp_drawable_get_active_components (drawable, active_components);

  /*  determine what sort of operation is being attempted and
   *  if it's actually legal...
   */
  operation = gimp_image_get_combination_mode (gimp_drawable_type (drawable),
                                               src2PR.bytes);
  if (operation == -1)
    {
      g_warning ("%s: illegal parameters.", G_STRFUNC);
      return;
    }

  /*  get the layer offsets  */
  gimp_item_get_offset (item, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within drawable bounds  */
  gimp_rectangle_intersect (dest_x, dest_y, src2PR.w, src2PR.h,
                            0, 0,
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item),
                            &x, &y, &width, &height);

  if (mask)
    {
      GimpItem *mask_item = GIMP_ITEM (mask);

      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      gimp_rectangle_intersect (x, y, width, height,
                                -offset_x, -offset_y,
                                gimp_item_get_width  (mask_item),
                                gimp_item_get_height (mask_item),
                                &x, &y, &width, &height);
    }

  /*  If the calling procedure specified an undo step...  */
  if (push_undo)
    {
      GimpDrawableUndo *undo;

      gimp_drawable_push_undo (drawable, undo_desc,
                               NULL, x, y, width, height);

      undo = GIMP_DRAWABLE_UNDO (gimp_image_undo_get_fadeable (image));

      if (undo)
        {
          undo->paint_mode = mode;
          undo->opacity    = opacity;

          undo->applied_buffer =
            gimp_gegl_buffer_new (GIMP_GEGL_RECT (0, 0, width, height),
                                  gegl_buffer_get_format (buffer));

          gegl_buffer_copy (buffer,
                            GIMP_GEGL_RECT (buffer_region->x + (x - dest_x),
                                            buffer_region->y + (y - dest_y),
                                            width, height),
                            undo->applied_buffer,
                            GIMP_GEGL_RECT (0, 0, width, height));
        }
    }

  /* configure the pixel regions */

  pixel_region_init (&src1PR, gimp_gegl_buffer_get_tiles (base_buffer),
                     x, y, width, height,
                     FALSE);

  /* check if an alternative to using the drawable's data as dest was
   * provided...
   */
  if (!destPR)
    {
      pixel_region_init (&my_destPR, gimp_drawable_get_tiles (drawable),
                         x, y, width, height,
                         TRUE);
      destPR = &my_destPR;
    }

  pixel_region_resize (&src2PR,
                       src2PR.x + (x - dest_x), src2PR.y + (y - dest_y),
                       width, height);

  if (mask)
    {
      PixelRegion maskPR;

      pixel_region_init (&maskPR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                         x + offset_x,
                         y + offset_y,
                         width, height,
                         FALSE);

      combine_regions (&src1PR, &src2PR, destPR, &maskPR, NULL,
                       opacity * 255.999,
                       mode,
                       active_components,
                       operation);
    }
  else
    {
      combine_regions (&src1PR, &src2PR, destPR, NULL, NULL,
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
gimp_drawable_real_replace_buffer (GimpDrawable        *drawable,
                                   GeglBuffer          *buffer,
                                   const GeglRectangle *buffer_region,
                                   gboolean             push_undo,
                                   const gchar         *undo_desc,
                                   gdouble              opacity,
                                   PixelRegion         *maskPR,
                                   gint                 dest_x,
                                   gint                 dest_y)
{
  GimpItem        *item  = GIMP_ITEM (drawable);
  GimpImage       *image = gimp_item_get_image (item);
  GimpChannel     *mask  = gimp_image_get_mask (image);
  TempBuf         *temp_buf;
  PixelRegion      src2PR;
  gint             x, y, width, height;
  gint             offset_x, offset_y;
  PixelRegion      src1PR, destPR;
  CombinationMode  operation;
  gboolean         active_components[MAX_CHANNELS];

  temp_buf = gimp_gegl_buffer_get_temp_buf (buffer);

  if (temp_buf)
    {
      pixel_region_init_temp_buf (&src2PR, temp_buf,
                                  buffer_region->x, buffer_region->y,
                                  buffer_region->width, buffer_region->height);
    }
  else
    {
      pixel_region_init (&src2PR, gimp_gegl_buffer_get_tiles (buffer),
                         buffer_region->x, buffer_region->y,
                         buffer_region->width, buffer_region->height,
                         FALSE);
    }

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (GIMP_DRAWABLE (mask) == drawable || gimp_channel_is_empty (mask))
    mask = NULL;

  /*  configure the active channel array  */
  gimp_drawable_get_active_components (drawable, active_components);

  /*  determine what sort of operation is being attempted and
   *  if it's actually legal...
   */
  operation = gimp_image_get_combination_mode (gimp_drawable_type (drawable),
                                               src2PR.bytes);
  if (operation == -1)
    {
      g_warning ("%s: illegal parameters.", G_STRFUNC);
      return;
    }

  /*  get the layer offsets  */
  gimp_item_get_offset (item, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within drawable bounds  */
  gimp_rectangle_intersect (dest_x, dest_y, src2PR.w, src2PR.h,
                            0, 0,
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item),
                            &x, &y, &width, &height);

  if (mask)
    {
      GimpItem *mask_item = GIMP_ITEM (mask);

      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      gimp_rectangle_intersect (x, y, width, height,
                                -offset_x, -offset_y,
                                gimp_item_get_width  (mask_item),
                                gimp_item_get_height (mask_item),
                                &x, &y, &width, &height);
    }

  /*  If the calling procedure specified an undo step...  */
  if (push_undo)
    gimp_drawable_push_undo (drawable, undo_desc,
                             NULL, x, y, width, height);

  /* configure the pixel regions */
  pixel_region_init (&src1PR, gimp_drawable_get_tiles (drawable),
                     x, y, width, height,
                     FALSE);
  pixel_region_init (&destPR, gimp_drawable_get_tiles (drawable),
                     x, y, width, height,
                     TRUE);
  pixel_region_resize (&src2PR,
                       src2PR.x + (x - dest_x), src2PR.y + (y - dest_y),
                       width, height);

  if (mask)
    {
      PixelRegion  mask2PR, tempPR;
      guchar      *temp_data;

      pixel_region_init (&mask2PR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                         x + offset_x, y + offset_y, width, height,
                         FALSE);

      temp_data = g_malloc (width * height);

      pixel_region_init_data (&tempPR, temp_data, 1, width,
                              0, 0, width, height);

      copy_region (&mask2PR, &tempPR);

      pixel_region_init_data (&tempPR, temp_data, 1, width,
                              0, 0, width, height);

      apply_mask_to_region (&tempPR, maskPR, OPAQUE_OPACITY);

      pixel_region_init_data (&tempPR, temp_data, 1, width,
                              0, 0, width, height);

      combine_regions_replace (&src1PR, &src2PR, &destPR, &tempPR, NULL,
                               opacity * 255.999,
                               active_components,
                               operation);

      g_free (temp_data);
    }
  else
    {
      combine_regions_replace (&src1PR, &src2PR, &destPR, maskPR, NULL,
                               opacity * 255.999,
                               active_components,
                               operation);
    }
}
