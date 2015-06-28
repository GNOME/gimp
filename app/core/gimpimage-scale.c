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

#include "core-types.h"

#include "base/tile-manager.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpguide.h"
#include "gimpgrouplayer.h"
#include "gimpimage.h"
#include "gimpimage-guides.h"
#include "gimpimage-item-list.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-scale.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimpprogress.h"
#include "gimpprojection.h"
#include "gimpsamplepoint.h"
#include "gimpsubprogress.h"

#include "gimp-log.h"
#include "gimp-intl.h"


void
gimp_image_scale (GimpImage             *image,
                  gint                   new_width,
                  gint                   new_height,
                  GimpInterpolationType  interpolation_type,
                  GimpProgress          *progress)
{
  GimpProgress *sub_progress;
  GList        *all_layers;
  GList        *all_channels;
  GList        *all_vectors;
  GList        *list;
  gint          old_width;
  gint          old_height;
  gint          offset_x;
  gint          offset_y;
  gdouble       img_scale_w      = 1.0;
  gdouble       img_scale_h      = 1.0;
  gint          progress_steps;
  gint          progress_current = 0;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (new_width > 0 && new_height > 0);
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  gimp_set_busy (image->gimp);

  sub_progress = gimp_sub_progress_new (progress);

  all_layers   = gimp_image_get_layer_list (image);
  all_channels = gimp_image_get_channel_list (image);
  all_vectors  = gimp_image_get_vectors_list (image);

  progress_steps = (g_list_length (all_layers)   +
                    g_list_length (all_channels) +
                    g_list_length (all_vectors)  +
                    1 /* selection */);

  g_object_freeze_notify (G_OBJECT (image));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_SCALE,
                               C_("undo-type", "Scale Image"));

  old_width   = gimp_image_get_width  (image);
  old_height  = gimp_image_get_height (image);
  img_scale_w = (gdouble) new_width  / (gdouble) old_width;
  img_scale_h = (gdouble) new_height / (gdouble) old_height;

  offset_x = (old_width  - new_width)  / 2;
  offset_y = (old_height - new_height) / 2;

  /*  Push the image size to the stack  */
  gimp_image_undo_push_image_size (image,
                                   NULL,
                                   offset_x,
                                   offset_y,
                                   new_width,
                                   new_height);

  /*  Set the new width and height early, so below image item setters
   *  (esp. guides and sample points) don't choke about moving stuff
   *  out of the image
   */
  g_object_set (image,
                "width",  new_width,
                "height", new_height,
                NULL);

  /*  Scale all channels  */
  for (list = all_channels; list; list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_sub_progress_set_step (GIMP_SUB_PROGRESS (sub_progress),
                                  progress_current++, progress_steps);

      gimp_item_scale (item,
                       new_width, new_height, 0, 0,
                       interpolation_type, sub_progress);
    }

  /*  Scale all vectors  */
  for (list = all_vectors; list; list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_sub_progress_set_step (GIMP_SUB_PROGRESS (sub_progress),
                                  progress_current++, progress_steps);

      gimp_item_scale (item,
                       new_width, new_height, 0, 0,
                       interpolation_type, sub_progress);
    }

  /*  Don't forget the selection mask!  */
  gimp_sub_progress_set_step (GIMP_SUB_PROGRESS (sub_progress),
                              progress_current++, progress_steps);

  gimp_item_scale (GIMP_ITEM (gimp_image_get_mask (image)),
                   new_width, new_height, 0, 0,
                   interpolation_type, sub_progress);

  /*  Scale all layers  */
  for (list = all_layers; list; list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_sub_progress_set_step (GIMP_SUB_PROGRESS (sub_progress),
                                  progress_current++, progress_steps);

      /*  group layers are updated automatically  */
      if (gimp_viewable_get_children (GIMP_VIEWABLE (item)))
        {
          gimp_group_layer_suspend_resize (GIMP_GROUP_LAYER (item), FALSE);
          continue;
        }

      if (! gimp_item_scale_by_factors (item,
                                        img_scale_w, img_scale_h,
                                        interpolation_type, sub_progress))
        {
          /* Since 0 < img_scale_w, img_scale_h, failure due to one or more
           * vanishing scaled layer dimensions. Implicit delete implemented
           * here. Upstream warning implemented in resize_check_layer_scaling(),
           * which offers the user the chance to bail out.
           */
          gimp_image_remove_layer (image, GIMP_LAYER (item), TRUE, NULL);
        }
    }

  for (list = all_layers; list; list = g_list_next (list))
    if (gimp_viewable_get_children (list->data))
      gimp_group_layer_resume_resize (list->data, FALSE);


  /*  Scale all Guides  */
  for (list = gimp_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      GimpGuide *guide    = list->data;
      gint       position = gimp_guide_get_position (guide);

      switch (gimp_guide_get_orientation (guide))
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          gimp_image_move_guide (image, guide,
                                 (position * new_height) / old_height,
                                 TRUE);
          break;

        case GIMP_ORIENTATION_VERTICAL:
          gimp_image_move_guide (image, guide,
                                 (position * new_width) / old_width,
                                 TRUE);
          break;

        default:
          break;
        }
    }

  /*  Scale all sample points  */
  for (list = gimp_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      GimpSamplePoint *sample_point = list->data;

      gimp_image_move_sample_point (image, sample_point,
                                    sample_point->x * new_width  / old_width,
                                    sample_point->y * new_height / old_height,
                                    TRUE);
    }

  gimp_image_undo_group_end (image);

  g_list_free (all_layers);
  g_list_free (all_channels);
  g_list_free (all_vectors);

  g_object_unref (sub_progress);

  gimp_image_size_changed_detailed (image,
                                    -offset_x,
                                    -offset_y,
                                    old_width,
                                    old_height);

  g_object_thaw_notify (G_OBJECT (image));

  gimp_unset_busy (image->gimp);
}

/**
 * gimp_image_scale_check:
 * @image:      A #GimpImage.
 * @new_width:   The new width.
 * @new_height:  The new height.
 * @max_memsize: The maximum new memory size.
 * @new_memsize: The new memory size.
 *
 * Inventory the layer list in image and check that it may be
 * scaled to @new_height and @new_width without problems.
 *
 * Return value: #GIMP_IMAGE_SCALE_OK if scaling the image will shrink none
 *               of its layers completely away, and the new image size
 *               is smaller than @max_memsize.
 *               #GIMP_IMAGE_SCALE_TOO_SMALL if scaling would remove some
 *               existing layers.
 *               #GIMP_IMAGE_SCALE_TOO_BIG if the new image size would
 *               exceed the maximum specified in the preferences.
 **/
GimpImageScaleCheckType
gimp_image_scale_check (const GimpImage *image,
                        gint             new_width,
                        gint             new_height,
                        gint64           max_memsize,
                        gint64          *new_memsize)
{
  GList  *drawables;
  GList  *all_layers;
  GList  *list;
  gint64  current_size;
  gint64  scalable_size;
  gint64  scaled_size;
  gint64  undo_size;
  gint64  redo_size;
  gint64  fixed_size;
  gint64  new_size;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), GIMP_IMAGE_SCALE_TOO_SMALL);
  g_return_val_if_fail (new_memsize != NULL, GIMP_IMAGE_SCALE_TOO_SMALL);

  current_size = gimp_object_get_memsize (GIMP_OBJECT (image), NULL);

  /*  the part of the image's memsize that scales linearly with the image  */
  drawables = gimp_image_item_list_get_list (image,
                                             GIMP_ITEM_TYPE_LAYERS |
                                             GIMP_ITEM_TYPE_CHANNELS,
                                             GIMP_ITEM_SET_ALL);

  drawables = gimp_image_item_list_filter (drawables);

  drawables = g_list_prepend (drawables, gimp_image_get_mask (image));

  scalable_size = 0;
  scaled_size   = 0;

  for (list = drawables; list; list = g_list_next (list))
    {
      GimpDrawable *drawable = list->data;
      gdouble       width    = gimp_item_get_width  (GIMP_ITEM (drawable));
      gdouble       height   = gimp_item_get_height (GIMP_ITEM (drawable));

      scalable_size +=
        gimp_drawable_estimate_memsize (drawable,
                                        width, height);

      scaled_size +=
        gimp_drawable_estimate_memsize (drawable,
                                        width * new_width /
                                        gimp_image_get_width (image),
                                        height * new_height /
                                        gimp_image_get_height (image));
    }

  g_list_free (drawables);

  scalable_size +=
    gimp_projection_estimate_memsize (gimp_image_base_type (image),
                                      gimp_image_get_width (image),
                                      gimp_image_get_height (image));

  scaled_size +=
    gimp_projection_estimate_memsize (gimp_image_base_type (image),
                                      new_width, new_height);

  GIMP_LOG (IMAGE_SCALE,
            "scalable_size = %"G_GINT64_FORMAT"  scaled_size = %"G_GINT64_FORMAT,
            scalable_size, scaled_size);

  undo_size = gimp_object_get_memsize (GIMP_OBJECT (gimp_image_get_undo_stack (image)), NULL);
  redo_size = gimp_object_get_memsize (GIMP_OBJECT (gimp_image_get_redo_stack (image)), NULL);

  /*  the fixed part of the image's memsize w/o any undo information  */
  fixed_size = current_size - undo_size - redo_size - scalable_size;

  /*  calculate the new size, which is:  */
  new_size = (fixed_size +   /*  the fixed part                */
              scaled_size);  /*  plus the part that scales...  */

  GIMP_LOG (IMAGE_SCALE,
            "old_size = %"G_GINT64_FORMAT"  new_size = %"G_GINT64_FORMAT,
            current_size - undo_size - redo_size, new_size);

  *new_memsize = new_size;

  if (new_size > current_size && new_size > max_memsize)
    return GIMP_IMAGE_SCALE_TOO_BIG;

  all_layers = gimp_image_get_layer_list (image);

  for (list = all_layers; list; list = g_list_next (list))
    {
      GimpItem *item = list->data;

      /*  group layers are updated automatically  */
      if (gimp_viewable_get_children (GIMP_VIEWABLE (item)))
        continue;

      if (! gimp_item_check_scaling (item, new_width, new_height))
        {
          g_list_free (all_layers);

          return GIMP_IMAGE_SCALE_TOO_SMALL;
        }
    }

  g_list_free (all_layers);

  return GIMP_IMAGE_SCALE_OK;
}
