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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-guides.h"
#include "gimpimage-item-list.h"
#include "gimpimage-resize.h"
#include "gimpimage-sample-points.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimpobjectqueue.h"
#include "gimpprogress.h"
#include "gimpsamplepoint.h"

#include "text/gimptextlayer.h"

#include "gimp-intl.h"


void
gimp_image_resize (GimpImage    *image,
                   GimpContext  *context,
                   gint          new_width,
                   gint          new_height,
                   gint          offset_x,
                   gint          offset_y,
                   GimpProgress *progress)
{
  gimp_image_resize_with_layers (image, context, GIMP_FILL_TRANSPARENT,
                                 new_width, new_height, offset_x, offset_y,
                                 GIMP_ITEM_SET_NONE, TRUE,
                                 progress);
}

void
gimp_image_resize_with_layers (GimpImage    *image,
                               GimpContext  *context,
                               GimpFillType  fill_type,
                               gint          new_width,
                               gint          new_height,
                               gint          offset_x,
                               gint          offset_y,
                               GimpItemSet   layer_set,
                               gboolean      resize_text_layers,
                               GimpProgress *progress)
{
  GimpObjectQueue *queue;
  GList           *resize_layers;
  GList           *list;
  GimpItem        *item;
  gint             old_width, old_height;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (new_width > 0 && new_height > 0);
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  gimp_set_busy (image->gimp);

  g_object_freeze_notify (G_OBJECT (image));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_RESIZE,
                               C_("undo-type", "Resize Image"));

  resize_layers = gimp_image_item_list_get_list (image,
                                                 GIMP_ITEM_TYPE_LAYERS,
                                                 layer_set);

  old_width  = gimp_image_get_width  (image);
  old_height = gimp_image_get_height (image);

  /*  Push the image size to the stack  */
  gimp_image_undo_push_image_size (image, NULL,
                                   -offset_x, -offset_y,
                                   new_width, new_height);

  /*  Set the new width and height  */
  g_object_set (image,
                "width",  new_width,
                "height", new_height,
                NULL);

  /*  Reposition all layers  */
  for (list = gimp_image_get_layer_iter (image);
       list;
       list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_item_translate (item, offset_x, offset_y, TRUE);
    }

  queue    = gimp_object_queue_new (progress);
  progress = GIMP_PROGRESS (queue);

  for (list = resize_layers; list; list = g_list_next (list))
    {
      GimpItem *item = list->data;

      /*  group layers can't be resized here  */
      if (gimp_viewable_get_children (GIMP_VIEWABLE (item)))
        continue;

      if (! resize_text_layers && gimp_item_is_text_layer (item))
        continue;

      /* note that we call gimp_item_start_move(), and not
       * gimp_item_start_transform().  see the comment in gimp_item_resize()
       * for more information.
       */
      gimp_item_start_move (item, TRUE);

      gimp_object_queue_push (queue, item);
    }

  g_list_free (resize_layers);
  
  gimp_object_queue_push (queue, gimp_image_get_mask (image));
  gimp_object_queue_push_container (queue, gimp_image_get_channels (image));
  gimp_object_queue_push_container (queue, gimp_image_get_vectors (image));

  /*  Resize all resize_layers, channels (including selection mask), and
   *  vectors
   */
  while ((item = gimp_object_queue_pop (queue)))
    {
      if (GIMP_IS_LAYER (item))
        {
          gint old_offset_x;
          gint old_offset_y;

          gimp_item_get_offset (item, &old_offset_x, &old_offset_y);

          gimp_item_resize (item, context, fill_type,
                            new_width, new_height,
                            old_offset_x, old_offset_y);

          gimp_item_end_move (item, TRUE);
        }
      else
        {
          gimp_item_resize (item, context, GIMP_FILL_TRANSPARENT,
                            new_width, new_height, offset_x, offset_y);
        }
    }

  /*  Reposition or remove all guides  */
  list = gimp_image_get_guides (image);

  while (list)
    {
      GimpGuide *guide        = list->data;
      gboolean   remove_guide = FALSE;
      gint       new_position = gimp_guide_get_position (guide);

      list = g_list_next (list);

      switch (gimp_guide_get_orientation (guide))
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          new_position += offset_y;
          if (new_position < 0 || new_position > new_height)
            remove_guide = TRUE;
          break;

        case GIMP_ORIENTATION_VERTICAL:
          new_position += offset_x;
          if (new_position < 0 || new_position > new_width)
            remove_guide = TRUE;
          break;

        default:
          break;
        }

      if (remove_guide)
        gimp_image_remove_guide (image, guide, TRUE);
      else if (new_position != gimp_guide_get_position (guide))
        gimp_image_move_guide (image, guide, new_position, TRUE);
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

      new_y = old_y + offset_y;
      if ((new_y < 0) || (new_y >= new_height))
        remove_sample_point = TRUE;

      new_x = old_x + offset_x;
      if ((new_x < 0) || (new_x >= new_width))
        remove_sample_point = TRUE;

      if (remove_sample_point)
        gimp_image_remove_sample_point (image, sample_point, TRUE);
      else if (new_x != old_x || new_y != old_y)
        gimp_image_move_sample_point (image, sample_point,
                                      new_x, new_y, TRUE);
    }

  g_object_unref (queue);

  gimp_image_undo_group_end (image);

  gimp_image_size_changed_detailed (image,
                                    offset_x, offset_y,
                                    old_width, old_height);

  g_object_thaw_notify (G_OBJECT (image));

  gimp_unset_busy (image->gimp);
}

void
gimp_image_resize_to_layers (GimpImage    *image,
                             GimpContext  *context,
                             gint         *offset_x,
                             gint         *offset_y,
                             gint         *new_width,
                             gint         *new_height,
                             GimpProgress *progress)
{
  GList    *list;
  GimpItem *item;
  gint      x, y;
  gint      width, height;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  list = gimp_image_get_layer_iter (image);

  if (! list)
    return;

  /* figure out starting values */
  item = list->data;

  x      = gimp_item_get_offset_x (item);
  y      = gimp_item_get_offset_y (item);
  width  = gimp_item_get_width  (item);
  height = gimp_item_get_height (item);

  /*  Respect all layers  */
  for (list = g_list_next (list); list; list = g_list_next (list))
    {
      item = list->data;

      gimp_rectangle_union (x, y,
                            width, height,
                            gimp_item_get_offset_x (item),
                            gimp_item_get_offset_y (item),
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item),
                            &x, &y,
                            &width, &height);
    }

  gimp_image_resize (image, context,
                     width, height, -x, -y,
                     progress);
  if (offset_x)
    *offset_x = -x;
  if (offset_y)
    *offset_y = -y;
  if (new_width)
    *new_width = width;
  if (new_height)
    *new_height = height;
}

void
gimp_image_resize_to_selection (GimpImage    *image,
                                GimpContext  *context,
                                GimpProgress *progress)
{
  GimpChannel *selection = gimp_image_get_mask (image);
  gint         x, y, w, h;

  if (gimp_item_bounds (GIMP_ITEM (selection), &x, &y, &w, &h))
    {
      gimp_image_resize (image, context,
                         w, h, -x, -y,
                         progress);
    }
}
