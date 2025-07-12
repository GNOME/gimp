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

#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimage-item-list.h"
#include "gimpimage-undo.h"
#include "gimpitem.h"
#include "gimplayer.h"
#include "gimpobjectqueue.h"
#include "gimpprogress.h"

#include "path/gimppath.h"

#include "gimp-intl.h"


/*  public functions  */

gboolean
gimp_image_item_list_bounds (GimpImage *image,
                             GList     *list,
                             gint      *x,
                             gint      *y,
                             gint      *width,
                             gint      *height)
{
  GList    *l;
  gboolean  bounds = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (x != 0, FALSE);
  g_return_val_if_fail (y != 0, FALSE);
  g_return_val_if_fail (width != 0, FALSE);
  g_return_val_if_fail (height != 0, FALSE);

  for (l = list; l; l = g_list_next (l))
    {
      GimpItem *item = l->data;
      gint      tmp_x, tmp_y;
      gint      tmp_w, tmp_h;

      if (gimp_item_bounds (item, &tmp_x, &tmp_y, &tmp_w, &tmp_h))
        {
          gint off_x, off_y;

          gimp_item_get_offset (item, &off_x, &off_y);

          if (bounds)
            {
              gimp_rectangle_union (*x, *y, *width, *height,
                                    tmp_x + off_x, tmp_y + off_y,
                                    tmp_w, tmp_h,
                                    x, y, width, height);
            }
          else
            {
              *x      = tmp_x + off_x;
              *y      = tmp_y + off_y;
              *width  = tmp_w;
              *height = tmp_h;

              bounds = TRUE;
            }
        }
    }

  if (! bounds)
    {
      *x      = 0;
      *y      = 0;
      *width  = gimp_image_get_width  (image);
      *height = gimp_image_get_height (image);
    }

  return bounds;
}

void
gimp_image_item_list_translate (GimpImage *image,
                                GList     *list,
                                gint       offset_x,
                                gint       offset_y,
                                gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (list)
    {
      GList *l;

      if (list->next)
        {
          if (push_undo)
            {
              gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_DISPLACE,
                                           C_("undo-type", "Translate Items"));
            }

          for (l = list; l; l = g_list_next (l))
            gimp_item_start_transform (GIMP_ITEM (l->data), push_undo);
        }

      for (l = list; l; l = g_list_next (l))
        gimp_item_translate (GIMP_ITEM (l->data),
                             offset_x, offset_y, push_undo);

      if (list->next)
        {
          for (l = list; l; l = g_list_next (l))
            gimp_item_end_transform (GIMP_ITEM (l->data), push_undo);

          if (push_undo)
            gimp_image_undo_group_end (image);
        }
    }
}

void
gimp_image_item_list_flip (GimpImage           *image,
                           GList               *list,
                           GimpContext         *context,
                           GimpOrientationType  flip_type,
                           gdouble              axis,
                           GimpTransformResize  expected_clip_result)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  if (list)
    {
      GList *l;

      if (list->next)
        {
          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM,
                                       C_("undo-type", "Flip Items"));

          for (l = list; l; l = g_list_next (l))
            gimp_item_start_transform (GIMP_ITEM (l->data), TRUE);
        }

      for (l = list; l; l = g_list_next (l))
        {
          GimpItem *item = l->data;

          gimp_item_flip (item, context,
                          flip_type, axis,
                          gimp_item_get_clip (item, expected_clip_result) !=
                          GIMP_TRANSFORM_RESIZE_ADJUST);
        }

      if (list->next)
        {
          for (l = list; l; l = g_list_next (l))
            gimp_item_end_transform (GIMP_ITEM (l->data), TRUE);

          gimp_image_undo_group_end (image);
        }
    }
}

void
gimp_image_item_list_rotate (GimpImage        *image,
                             GList            *list,
                             GimpContext      *context,
                             GimpRotationType  rotate_type,
                             gdouble           center_x,
                             gdouble           center_y,
                             gboolean          clip_result)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  if (list)
    {
      GList *l;

      if (list->next)
        {
          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM,
                                       C_("undo-type", "Rotate Items"));

          for (l = list; l; l = g_list_next (l))
            gimp_item_start_transform (GIMP_ITEM (l->data), TRUE);
        }

      for (l = list; l; l = g_list_next (l))
        {
          GimpItem *item = l->data;

          gimp_item_rotate (item, context,
                            rotate_type, center_x, center_y,
                            gimp_item_get_clip (item, clip_result));
        }

      if (list->next)
        {
          for (l = list; l; l = g_list_next (l))
            gimp_item_end_transform (GIMP_ITEM (l->data), TRUE);

          gimp_image_undo_group_end (image);
        }
    }
}

void
gimp_image_item_list_transform (GimpImage              *image,
                                GList                  *list,
                                GimpContext            *context,
                                const GimpMatrix3      *matrix,
                                GimpTransformDirection  direction,
                                GimpInterpolationType   interpolation_type,
                                GimpTransformResize     clip_result,
                                GimpProgress           *progress)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (list)
    {
      GimpObjectQueue *queue = NULL;
      GList           *l;

      if (progress)
        {
          queue    = gimp_object_queue_new (progress);
          progress = GIMP_PROGRESS (queue);

          gimp_object_queue_push_list (queue, list);
        }

      if (list->next)
        {
          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM,
                                       C_("undo-type", "Transform Items"));

          for (l = list; l; l = g_list_next (l))
            gimp_item_start_transform (GIMP_ITEM (l->data), TRUE);
        }

      for (l = list; l; l = g_list_next (l))
        {
          GimpItem *item = l->data;

          if (queue)
            gimp_object_queue_pop (queue);

          gimp_item_transform (item, context,
                               matrix, direction,
                               interpolation_type,
                               gimp_item_get_clip (item, clip_result),
                               progress);
        }

      if (list->next)
        {
          for (l = list; l; l = g_list_next (l))
            gimp_item_end_transform (GIMP_ITEM (l->data), TRUE);

          gimp_image_undo_group_end (image);
        }

      g_clear_object (&queue);
    }
}

/**
 * gimp_image_item_list_get_list:
 * @image:   An @image.
 * @type:    Which type of items to return.
 * @set:     Set the returned items are part of.
 *
 * This function returns a #GList of #GimpItem<!-- -->s for which the
 * @type and @set criterions match.
 *
 * Returns: The list of items.
 **/
GList *
gimp_image_item_list_get_list (GimpImage        *image,
                               GimpItemTypeMask  type,
                               GimpItemSet       set)
{
  GList *all_items;
  GList *list;
  GList *return_list = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  if (type & GIMP_ITEM_TYPE_LAYERS)
    {
      all_items = gimp_image_get_layer_list (image);

      for (list = all_items; list; list = g_list_next (list))
        {
          GimpItem *item = list->data;

          if (gimp_item_is_in_set (item, set))
            return_list = g_list_prepend (return_list, item);
        }

      g_list_free (all_items);
    }

  if (type & GIMP_ITEM_TYPE_CHANNELS)
    {
      all_items = gimp_image_get_channel_list (image);

      for (list = all_items; list; list = g_list_next (list))
        {
          GimpItem *item = list->data;

          if (gimp_item_is_in_set (item, set))
            return_list = g_list_prepend (return_list, item);
        }

      g_list_free (all_items);
    }

  if (type & GIMP_ITEM_TYPE_PATHS)
    {
      all_items = gimp_image_get_path_list (image);

      for (list = all_items; list; list = g_list_next (list))
        {
          GimpItem *item = list->data;

          if (gimp_item_is_in_set (item, set))
            return_list = g_list_prepend (return_list, item);
        }

      g_list_free (all_items);
    }

  return g_list_reverse (return_list);
}

static GList *
gimp_image_item_list_remove_children (GList    *list,
                                      GimpItem *parent)
{
  GList *l = list;

  while (l)
    {
      GimpItem *item = l->data;

      l = g_list_next (l);

      if (gimp_viewable_is_ancestor (GIMP_VIEWABLE (parent),
                                     GIMP_VIEWABLE (item)))
        {
          list = g_list_remove (list, item);
        }
    }

  return list;
}

/**
 * gimp_image_item_list_filter:
 * @image:
 * @items: the original list of #GimpItem-s.
 *
 * Filter @list by modifying it directly (so the original list should
 * not be used anymore, only its result), removing all children items
 * with ancestors also in @list.
 *
 * Returns: the modified @list where all items which have an ancestor in
 *          @list have been removed.
 */
GList *
gimp_image_item_list_filter (GList *list)
{
  GList *l;

  if (! list)
    return NULL;

  for (l = list; l; l = g_list_next (l))
    {
      GimpItem *item = l->data;
      GList    *next;

      next = gimp_image_item_list_remove_children (g_list_next (l), item);

      l->next = next;
      if (next)
        next->prev = l;
    }

  return list;
}
