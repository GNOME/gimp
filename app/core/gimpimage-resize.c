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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma.h"
#include "ligmacontainer.h"
#include "ligmacontext.h"
#include "ligmaguide.h"
#include "ligmaimage.h"
#include "ligmaimage-guides.h"
#include "ligmaimage-item-list.h"
#include "ligmaimage-resize.h"
#include "ligmaimage-sample-points.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmalayer.h"
#include "ligmaobjectqueue.h"
#include "ligmaprogress.h"
#include "ligmasamplepoint.h"

#include "text/ligmatextlayer.h"

#include "ligma-intl.h"


void
ligma_image_resize (LigmaImage    *image,
                   LigmaContext  *context,
                   gint          new_width,
                   gint          new_height,
                   gint          offset_x,
                   gint          offset_y,
                   LigmaProgress *progress)
{
  ligma_image_resize_with_layers (image, context, LIGMA_FILL_TRANSPARENT,
                                 new_width, new_height, offset_x, offset_y,
                                 LIGMA_ITEM_SET_NONE, TRUE,
                                 progress);
}

void
ligma_image_resize_with_layers (LigmaImage    *image,
                               LigmaContext  *context,
                               LigmaFillType  fill_type,
                               gint          new_width,
                               gint          new_height,
                               gint          offset_x,
                               gint          offset_y,
                               LigmaItemSet   layer_set,
                               gboolean      resize_text_layers,
                               LigmaProgress *progress)
{
  LigmaObjectQueue *queue;
  GList           *resize_layers;
  GList           *list;
  LigmaItem        *item;
  gint             old_width, old_height;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (new_width > 0 && new_height > 0);
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  ligma_set_busy (image->ligma);

  g_object_freeze_notify (G_OBJECT (image));

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_RESIZE,
                               C_("undo-type", "Resize Image"));

  resize_layers = ligma_image_item_list_get_list (image,
                                                 LIGMA_ITEM_TYPE_LAYERS,
                                                 layer_set);

  old_width  = ligma_image_get_width  (image);
  old_height = ligma_image_get_height (image);

  /*  Push the image size to the stack  */
  ligma_image_undo_push_image_size (image, NULL,
                                   -offset_x, -offset_y,
                                   new_width, new_height);

  /*  Set the new width and height  */
  g_object_set (image,
                "width",  new_width,
                "height", new_height,
                NULL);

  /*  Reposition all layers  */
  for (list = ligma_image_get_layer_iter (image);
       list;
       list = g_list_next (list))
    {
      LigmaItem *item = list->data;

      ligma_item_translate (item, offset_x, offset_y, TRUE);
    }

  queue    = ligma_object_queue_new (progress);
  progress = LIGMA_PROGRESS (queue);

  for (list = resize_layers; list; list = g_list_next (list))
    {
      LigmaItem *item = list->data;

      /*  group layers can't be resized here  */
      if (ligma_viewable_get_children (LIGMA_VIEWABLE (item)))
        continue;

      if (! resize_text_layers && ligma_item_is_text_layer (item))
        continue;

      /* note that we call ligma_item_start_move(), and not
       * ligma_item_start_transform().  see the comment in ligma_item_resize()
       * for more information.
       */
      ligma_item_start_move (item, TRUE);

      ligma_object_queue_push (queue, item);
    }

  g_list_free (resize_layers);
  
  ligma_object_queue_push (queue, ligma_image_get_mask (image));
  ligma_object_queue_push_container (queue, ligma_image_get_channels (image));
  ligma_object_queue_push_container (queue, ligma_image_get_vectors (image));

  /*  Resize all resize_layers, channels (including selection mask), and
   *  vectors
   */
  while ((item = ligma_object_queue_pop (queue)))
    {
      if (LIGMA_IS_LAYER (item))
        {
          gint old_offset_x;
          gint old_offset_y;

          ligma_item_get_offset (item, &old_offset_x, &old_offset_y);

          ligma_item_resize (item, context, fill_type,
                            new_width, new_height,
                            old_offset_x, old_offset_y);

          ligma_item_end_move (item, TRUE);
        }
      else
        {
          ligma_item_resize (item, context, LIGMA_FILL_TRANSPARENT,
                            new_width, new_height, offset_x, offset_y);
        }
    }

  /*  Reposition or remove all guides  */
  list = ligma_image_get_guides (image);

  while (list)
    {
      LigmaGuide *guide        = list->data;
      gboolean   remove_guide = FALSE;
      gint       new_position = ligma_guide_get_position (guide);

      list = g_list_next (list);

      switch (ligma_guide_get_orientation (guide))
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          new_position += offset_y;
          if (new_position < 0 || new_position > new_height)
            remove_guide = TRUE;
          break;

        case LIGMA_ORIENTATION_VERTICAL:
          new_position += offset_x;
          if (new_position < 0 || new_position > new_width)
            remove_guide = TRUE;
          break;

        default:
          break;
        }

      if (remove_guide)
        ligma_image_remove_guide (image, guide, TRUE);
      else if (new_position != ligma_guide_get_position (guide))
        ligma_image_move_guide (image, guide, new_position, TRUE);
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

      new_y = old_y + offset_y;
      if ((new_y < 0) || (new_y >= new_height))
        remove_sample_point = TRUE;

      new_x = old_x + offset_x;
      if ((new_x < 0) || (new_x >= new_width))
        remove_sample_point = TRUE;

      if (remove_sample_point)
        ligma_image_remove_sample_point (image, sample_point, TRUE);
      else if (new_x != old_x || new_y != old_y)
        ligma_image_move_sample_point (image, sample_point,
                                      new_x, new_y, TRUE);
    }

  g_object_unref (queue);

  ligma_image_undo_group_end (image);

  ligma_image_size_changed_detailed (image,
                                    offset_x, offset_y,
                                    old_width, old_height);

  g_object_thaw_notify (G_OBJECT (image));

  ligma_unset_busy (image->ligma);
}

void
ligma_image_resize_to_layers (LigmaImage    *image,
                             LigmaContext  *context,
                             gint         *offset_x,
                             gint         *offset_y,
                             gint         *new_width,
                             gint         *new_height,
                             LigmaProgress *progress)
{
  GList    *list;
  LigmaItem *item;
  gint      x, y;
  gint      width, height;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  list = ligma_image_get_layer_iter (image);

  if (! list)
    return;

  /* figure out starting values */
  item = list->data;

  x      = ligma_item_get_offset_x (item);
  y      = ligma_item_get_offset_y (item);
  width  = ligma_item_get_width  (item);
  height = ligma_item_get_height (item);

  /*  Respect all layers  */
  for (list = g_list_next (list); list; list = g_list_next (list))
    {
      item = list->data;

      ligma_rectangle_union (x, y,
                            width, height,
                            ligma_item_get_offset_x (item),
                            ligma_item_get_offset_y (item),
                            ligma_item_get_width  (item),
                            ligma_item_get_height (item),
                            &x, &y,
                            &width, &height);
    }

  ligma_image_resize (image, context,
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
ligma_image_resize_to_selection (LigmaImage    *image,
                                LigmaContext  *context,
                                LigmaProgress *progress)
{
  LigmaChannel *selection = ligma_image_get_mask (image);
  gint         x, y, w, h;

  if (ligma_item_bounds (LIGMA_ITEM (selection), &x, &y, &w, &h))
    {
      ligma_image_resize (image, context,
                         w, h, -x, -y,
                         progress);
    }
}
