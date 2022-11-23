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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "ligma.h"
#include "ligmacontainer.h"
#include "ligmaguide.h"
#include "ligmagrouplayer.h"
#include "ligmaimage.h"
#include "ligmaimage-guides.h"
#include "ligmaimage-sample-points.h"
#include "ligmaimage-scale.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmalayer.h"
#include "ligmaobjectqueue.h"
#include "ligmaprogress.h"
#include "ligmaprojection.h"
#include "ligmasamplepoint.h"

#include "ligma-log.h"
#include "ligma-intl.h"


void
ligma_image_scale (LigmaImage             *image,
                  gint                   new_width,
                  gint                   new_height,
                  LigmaInterpolationType  interpolation_type,
                  LigmaProgress          *progress)
{
  LigmaObjectQueue *queue;
  LigmaItem        *item;
  GList           *list;
  gint             old_width;
  gint             old_height;
  gint             offset_x;
  gint             offset_y;
  gdouble          img_scale_w = 1.0;
  gdouble          img_scale_h = 1.0;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (new_width > 0 && new_height > 0);
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  ligma_set_busy (image->ligma);

  queue    = ligma_object_queue_new (progress);
  progress = LIGMA_PROGRESS (queue);

  ligma_object_queue_push_container (queue, ligma_image_get_layers (image));
  ligma_object_queue_push (queue, ligma_image_get_mask (image));
  ligma_object_queue_push_container (queue, ligma_image_get_channels (image));
  ligma_object_queue_push_container (queue, ligma_image_get_vectors (image));

  g_object_freeze_notify (G_OBJECT (image));

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_SCALE,
                               C_("undo-type", "Scale Image"));

  old_width   = ligma_image_get_width  (image);
  old_height  = ligma_image_get_height (image);
  img_scale_w = (gdouble) new_width  / (gdouble) old_width;
  img_scale_h = (gdouble) new_height / (gdouble) old_height;

  offset_x = (old_width  - new_width)  / 2;
  offset_y = (old_height - new_height) / 2;

  /*  Push the image size to the stack  */
  ligma_image_undo_push_image_size (image,
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

  /*  Scale all layers, channels (including selection mask), and vectors  */
  while ((item = ligma_object_queue_pop (queue)))
    {
      if (! ligma_item_scale_by_factors (item,
                                        img_scale_w, img_scale_h,
                                        interpolation_type, progress))
        {
          /* Since 0 < img_scale_w, img_scale_h, failure due to one or more
           * vanishing scaled layer dimensions. Implicit delete implemented
           * here. Upstream warning implemented in resize_check_layer_scaling(),
           * which offers the user the chance to bail out.
           */
          g_return_if_fail (LIGMA_IS_LAYER (item));
          ligma_image_remove_layer (image, LIGMA_LAYER (item), TRUE, NULL);
        }
    }

  /*  Scale all Guides  */
  for (list = ligma_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      LigmaGuide *guide    = list->data;
      gint       position = ligma_guide_get_position (guide);

      switch (ligma_guide_get_orientation (guide))
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          ligma_image_move_guide (image, guide,
                                 (position * new_height) / old_height,
                                 TRUE);
          break;

        case LIGMA_ORIENTATION_VERTICAL:
          ligma_image_move_guide (image, guide,
                                 (position * new_width) / old_width,
                                 TRUE);
          break;

        default:
          break;
        }
    }

  /*  Scale all sample points  */
  for (list = ligma_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      LigmaSamplePoint *sample_point = list->data;
      gint             x;
      gint             y;

      ligma_sample_point_get_position (sample_point, &x, &y);

      ligma_image_move_sample_point (image, sample_point,
                                    x * new_width  / old_width,
                                    y * new_height / old_height,
                                    TRUE);
    }

  ligma_image_undo_group_end (image);

  g_object_unref (queue);

  ligma_image_size_changed_detailed (image,
                                    -offset_x,
                                    -offset_y,
                                    old_width,
                                    old_height);

  g_object_thaw_notify (G_OBJECT (image));

  ligma_unset_busy (image->ligma);
}

/**
 * ligma_image_scale_check:
 * @image:      A #LigmaImage.
 * @new_width:   The new width.
 * @new_height:  The new height.
 * @max_memsize: The maximum new memory size.
 * @new_memsize: The new memory size.
 *
 * Inventory the layer list in image and check that it may be
 * scaled to @new_height and @new_width without problems.
 *
 * Returns: #LIGMA_IMAGE_SCALE_OK if scaling the image will shrink none
 *               of its layers completely away, and the new image size
 *               is smaller than @max_memsize.
 *               #LIGMA_IMAGE_SCALE_TOO_SMALL if scaling would remove some
 *               existing layers.
 *               #LIGMA_IMAGE_SCALE_TOO_BIG if the new image size would
 *               exceed the maximum specified in the preferences.
 **/
LigmaImageScaleCheckType
ligma_image_scale_check (LigmaImage *image,
                        gint       new_width,
                        gint       new_height,
                        gint64     max_memsize,
                        gint64    *new_memsize)
{
  GList  *all_layers;
  GList  *list;
  gint64  current_size;
  gint64  undo_size;
  gint64  redo_size;
  gint64  new_size;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), LIGMA_IMAGE_SCALE_TOO_SMALL);
  g_return_val_if_fail (new_memsize != NULL, LIGMA_IMAGE_SCALE_TOO_SMALL);

  current_size = ligma_object_get_memsize (LIGMA_OBJECT (image), NULL);

  new_size = ligma_image_estimate_memsize (image,
                                          ligma_image_get_component_type (image),
                                          new_width, new_height);

  undo_size = ligma_object_get_memsize (LIGMA_OBJECT (ligma_image_get_undo_stack (image)), NULL);
  redo_size = ligma_object_get_memsize (LIGMA_OBJECT (ligma_image_get_redo_stack (image)), NULL);

  current_size -= undo_size + redo_size;
  new_size     -= undo_size + redo_size;

  LIGMA_LOG (IMAGE_SCALE,
            "old_size = %"G_GINT64_FORMAT"  new_size = %"G_GINT64_FORMAT,
            current_size, new_size);

  *new_memsize = new_size;

  if (new_size > current_size && new_size > max_memsize)
    return LIGMA_IMAGE_SCALE_TOO_BIG;

  all_layers = ligma_image_get_layer_list (image);

  for (list = all_layers; list; list = g_list_next (list))
    {
      LigmaItem *item = list->data;

      /*  group layers are updated automatically  */
      if (ligma_viewable_get_children (LIGMA_VIEWABLE (item)))
        continue;

      if (! ligma_item_check_scaling (item, new_width, new_height))
        {
          g_list_free (all_layers);

          return LIGMA_IMAGE_SCALE_TOO_SMALL;
        }
    }

  g_list_free (all_layers);

  return LIGMA_IMAGE_SCALE_OK;
}
