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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-crop.h"
#include "gimpimage-guides.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimpsamplepoint.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_image_crop (GimpImage    *image,
                 GimpContext  *context,
                 GimpFillType  fill_type,
                 gint          x,
                 gint          y,
                 gint          width,
                 gint          height,
                 gboolean      crop_layers)
{
  GList *list;
  gint   previous_width;
  gint   previous_height;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  previous_width  = gimp_image_get_width  (image);
  previous_height = gimp_image_get_height (image);

  /*  Make sure new width and height are non-zero  */
  if (width < 1 || height < 1)
    return;

  gimp_set_busy (image->gimp);

  g_object_freeze_notify (G_OBJECT (image));

  if (crop_layers)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_CROP,
                                 C_("undo-type", "Crop Image"));
  else
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_RESIZE,
                                 C_("undo-type", "Resize Image"));

  /*  Push the image size to the stack  */
  gimp_image_undo_push_image_size (image, NULL,
                                   x, y, width, height);

  /*  Set the new width and height  */
  g_object_set (image,
                "width",  width,
                "height", height,
                NULL);

  /*  Resize all channels  */
  for (list = gimp_image_get_channel_iter (image);
       list;
       list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_item_resize (item, context, GIMP_FILL_TRANSPARENT,
                        width, height, -x, -y);
    }

  /*  Resize all vectors  */
  for (list = gimp_image_get_path_iter (image);
       list;
       list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_item_resize (item, context, GIMP_FILL_TRANSPARENT,
                        width, height, -x, -y);
    }

  /*  Don't forget the selection mask!  */
  gimp_item_resize (GIMP_ITEM (gimp_image_get_mask (image)),
                    context, GIMP_FILL_TRANSPARENT,
                    width, height, -x, -y);

  /*  crop all layers  */
  list = gimp_image_get_layer_iter (image);

  while (list)
    {
      GimpItem *item = list->data;

      list = g_list_next (list);

      gimp_item_translate (item, -x, -y, TRUE);

      if (crop_layers && ! gimp_item_is_content_locked (item, NULL))
        {
          gint off_x, off_y;
          gint lx1, ly1, lx2, ly2;

          gimp_item_get_offset (item, &off_x, &off_y);

          lx1 = CLAMP (off_x, 0, gimp_image_get_width  (image));
          ly1 = CLAMP (off_y, 0, gimp_image_get_height (image));
          lx2 = CLAMP (gimp_item_get_width  (item) + off_x,
                       0, gimp_image_get_width (image));
          ly2 = CLAMP (gimp_item_get_height (item) + off_y,
                       0, gimp_image_get_height (image));

          width  = lx2 - lx1;
          height = ly2 - ly1;

          gimp_drawable_enable_resize_undo (GIMP_DRAWABLE (item));

          if (width > 0 && height > 0)
            {
              gimp_item_resize (item, context, fill_type,
                                width, height,
                                -(lx1 - off_x),
                                -(ly1 - off_y));
            }
          else
            {
              gimp_image_remove_layer (image, GIMP_LAYER (item),
                                       TRUE, NULL);
            }
        }
    }

  /*  Reposition or remove guides  */
  list = gimp_image_get_guides (image);

  while (list)
    {
      GimpGuide *guide        = list->data;
      gboolean   remove_guide = FALSE;
      gint       position     = gimp_guide_get_position (guide);

      list = g_list_next (list);

      switch (gimp_guide_get_orientation (guide))
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          position -= y;
          if ((position < 0) || (position > height))
            remove_guide = TRUE;
          break;

        case GIMP_ORIENTATION_VERTICAL:
          position -= x;
          if ((position < 0) || (position > width))
            remove_guide = TRUE;
          break;

        default:
          break;
        }

      if (remove_guide)
        gimp_image_remove_guide (image, guide, TRUE);
      else if (position != gimp_guide_get_position (guide))
        gimp_image_move_guide (image, guide, position, TRUE);
    }

  /*  Reposition or remove sample points  */
  list = gimp_image_get_sample_points (image);

  while (list)
    {
      GimpSamplePoint *sample_point        = list->data;
      gboolean         remove_sample_point = FALSE;
      gint             old_x;
      gint             old_y;
      gint             new_x;
      gint             new_y;

      list = g_list_next (list);

      gimp_sample_point_get_position (sample_point, &old_x, &old_y);
      new_x = old_x;
      new_y = old_y;

      new_y -= y;
      if ((new_y < 0) || (new_y > height))
       remove_sample_point = TRUE;

      new_x -= x;
      if ((new_x < 0) || (new_x > width))
        remove_sample_point = TRUE;

      if (remove_sample_point)
        gimp_image_remove_sample_point (image, sample_point, TRUE);
      else if (new_x != old_x || new_y != old_y)
        gimp_image_move_sample_point (image, sample_point,
                                      new_x, new_y, TRUE);
    }

  gimp_image_undo_group_end (image);

  gimp_image_size_changed_detailed (image,
                                    -x, -y,
                                    previous_width, previous_height);

  g_object_thaw_notify (G_OBJECT (image));

  gimp_unset_busy (image->gimp);
}
