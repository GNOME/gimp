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
#include "ligmacontext.h"
#include "ligmaguide.h"
#include "ligmaimage.h"
#include "ligmaimage-crop.h"
#include "ligmaimage-guides.h"
#include "ligmaimage-sample-points.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmalayer.h"
#include "ligmasamplepoint.h"

#include "ligma-intl.h"


/*  public functions  */

void
ligma_image_crop (LigmaImage    *image,
                 LigmaContext  *context,
                 LigmaFillType  fill_type,
                 gint          x,
                 gint          y,
                 gint          width,
                 gint          height,
                 gboolean      crop_layers)
{
  GList *list;
  gint   previous_width;
  gint   previous_height;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  previous_width  = ligma_image_get_width  (image);
  previous_height = ligma_image_get_height (image);

  /*  Make sure new width and height are non-zero  */
  if (width < 1 || height < 1)
    return;

  ligma_set_busy (image->ligma);

  g_object_freeze_notify (G_OBJECT (image));

  if (crop_layers)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_CROP,
                                 C_("undo-type", "Crop Image"));
  else
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_RESIZE,
                                 C_("undo-type", "Resize Image"));

  /*  Push the image size to the stack  */
  ligma_image_undo_push_image_size (image, NULL,
                                   x, y, width, height);

  /*  Set the new width and height  */
  g_object_set (image,
                "width",  width,
                "height", height,
                NULL);

  /*  Resize all channels  */
  for (list = ligma_image_get_channel_iter (image);
       list;
       list = g_list_next (list))
    {
      LigmaItem *item = list->data;

      ligma_item_resize (item, context, LIGMA_FILL_TRANSPARENT,
                        width, height, -x, -y);
    }

  /*  Resize all vectors  */
  for (list = ligma_image_get_vectors_iter (image);
       list;
       list = g_list_next (list))
    {
      LigmaItem *item = list->data;

      ligma_item_resize (item, context, LIGMA_FILL_TRANSPARENT,
                        width, height, -x, -y);
    }

  /*  Don't forget the selection mask!  */
  ligma_item_resize (LIGMA_ITEM (ligma_image_get_mask (image)),
                    context, LIGMA_FILL_TRANSPARENT,
                    width, height, -x, -y);

  /*  crop all layers  */
  list = ligma_image_get_layer_iter (image);

  while (list)
    {
      LigmaItem *item = list->data;

      list = g_list_next (list);

      ligma_item_translate (item, -x, -y, TRUE);

      if (crop_layers && ! ligma_item_is_content_locked (item, NULL))
        {
          gint off_x, off_y;
          gint lx1, ly1, lx2, ly2;

          ligma_item_get_offset (item, &off_x, &off_y);

          lx1 = CLAMP (off_x, 0, ligma_image_get_width  (image));
          ly1 = CLAMP (off_y, 0, ligma_image_get_height (image));
          lx2 = CLAMP (ligma_item_get_width  (item) + off_x,
                       0, ligma_image_get_width (image));
          ly2 = CLAMP (ligma_item_get_height (item) + off_y,
                       0, ligma_image_get_height (image));

          width  = lx2 - lx1;
          height = ly2 - ly1;

          if (width > 0 && height > 0)
            {
              ligma_item_resize (item, context, fill_type,
                                width, height,
                                -(lx1 - off_x),
                                -(ly1 - off_y));
            }
          else
            {
              ligma_image_remove_layer (image, LIGMA_LAYER (item),
                                       TRUE, NULL);
            }
        }
    }

  /*  Reposition or remove guides  */
  list = ligma_image_get_guides (image);

  while (list)
    {
      LigmaGuide *guide        = list->data;
      gboolean   remove_guide = FALSE;
      gint       position     = ligma_guide_get_position (guide);

      list = g_list_next (list);

      switch (ligma_guide_get_orientation (guide))
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          position -= y;
          if ((position < 0) || (position > height))
            remove_guide = TRUE;
          break;

        case LIGMA_ORIENTATION_VERTICAL:
          position -= x;
          if ((position < 0) || (position > width))
            remove_guide = TRUE;
          break;

        default:
          break;
        }

      if (remove_guide)
        ligma_image_remove_guide (image, guide, TRUE);
      else if (position != ligma_guide_get_position (guide))
        ligma_image_move_guide (image, guide, position, TRUE);
    }

  /*  Reposition or remove sample points  */
  list = ligma_image_get_sample_points (image);

  while (list)
    {
      LigmaSamplePoint *sample_point        = list->data;
      gboolean         remove_sample_point = FALSE;
      gint             old_x;
      gint             old_y;
      gint             new_x;
      gint             new_y;

      list = g_list_next (list);

      ligma_sample_point_get_position (sample_point, &old_x, &old_y);
      new_x = old_x;
      new_y = old_y;

      new_y -= y;
      if ((new_y < 0) || (new_y > height))
       remove_sample_point = TRUE;

      new_x -= x;
      if ((new_x < 0) || (new_x > width))
        remove_sample_point = TRUE;

      if (remove_sample_point)
        ligma_image_remove_sample_point (image, sample_point, TRUE);
      else if (new_x != old_x || new_y != old_y)
        ligma_image_move_sample_point (image, sample_point,
                                      new_x, new_y, TRUE);
    }

  ligma_image_undo_group_end (image);

  ligma_image_size_changed_detailed (image,
                                    -x, -y,
                                    previous_width, previous_height);

  g_object_thaw_notify (G_OBJECT (image));

  ligma_unset_busy (image->ligma);
}
