/* The GIMP -- an image manipulation program
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

#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpitem.h"
#include "gimpitem-linked.h"
#include "gimplist.h"
#include "gimpprogress.h"


/*  public functions  */

void
gimp_item_linked_translate (GimpItem *item,
                            gint      offset_x,
                            gint      offset_y,
                            gboolean  push_undo)
{
  GList *linked_list;
  GList *list;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));

  linked_list = gimp_item_linked_get_list (item, GIMP_ITEM_LINKED_ALL);

  for (list = linked_list; list; list = g_list_next (list))
    gimp_item_translate (GIMP_ITEM (list->data),
                         offset_x, offset_y, push_undo);

  g_list_free (linked_list);
}

void
gimp_item_linked_flip (GimpItem            *item,
                       GimpContext         *context,
                       GimpOrientationType  flip_type,
                       gdouble              axis,
                       gboolean             clip_result)
{
  GList *linked_list;
  GList *list;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));

  linked_list = gimp_item_linked_get_list (item, GIMP_ITEM_LINKED_ALL);

  for (list = linked_list; list; list = g_list_next (list))
    gimp_item_flip (GIMP_ITEM (list->data), context,
                    flip_type, axis, clip_result);

  g_list_free (linked_list);
}

void
gimp_item_linked_rotate (GimpItem         *item,
                         GimpContext      *context,
                         GimpRotationType  rotate_type,
                         gdouble           center_x,
                         gdouble           center_y,
                         gboolean          clip_result)
{
  GList *linked_list;
  GList *list;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));

  linked_list = gimp_item_linked_get_list (item,
                                           GIMP_ITEM_LINKED_LAYERS |
                                           GIMP_ITEM_LINKED_VECTORS);

  for (list = linked_list; list; list = g_list_next (list))
    gimp_item_rotate (GIMP_ITEM (list->data), context,
                      rotate_type, center_x, center_y, clip_result);

  g_list_free (linked_list);

  linked_list = gimp_item_linked_get_list (item, GIMP_ITEM_LINKED_CHANNELS);

  for (list = linked_list; list; list = g_list_next (list))
    gimp_item_rotate (GIMP_ITEM (list->data), context,
                      rotate_type, center_x, center_y, TRUE);

  g_list_free (linked_list);
}

void
gimp_item_linked_transform (GimpItem               *item,
                            GimpContext            *context,
                            const GimpMatrix3      *matrix,
                            GimpTransformDirection  direction,
                            GimpInterpolationType   interpolation_type,
                            gboolean                supersample,
                            gint                    recursion_level,
                            gboolean                clip_result,
                            GimpProgress           *progress)
{
  GList *linked_list;
  GList *list;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  linked_list = gimp_item_linked_get_list (item, GIMP_ITEM_LINKED_ALL);

  for (list = linked_list; list; list = g_list_next (list))
    gimp_item_transform (GIMP_ITEM (list->data), context,
                         matrix, direction,
                         interpolation_type,
                         supersample, recursion_level,
                         clip_result, progress);

  g_list_free (linked_list);
}

/**
 * gimp_item_linked_get_list:
 * @item:  A linked @item.
 * @which: Which items to return.
 *
 * This function returns a #GList og #GimpItem's for which the
 * "linked" property is #TRUE. Note that the passed in @item
 * must be linked too.
 *
 * Return value: The list of linked items, excluding the passed @item.
 **/
GList *
gimp_item_linked_get_list (GimpItem           *item,
                           GimpItemLinkedMask  which)
{
  GimpImage *image;
  GimpItem  *linked_item;
  GList     *list;
  GList     *linked_list = NULL;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (gimp_item_get_linked (item), NULL);
  g_return_val_if_fail (gimp_item_is_attached (item), NULL);

  image = gimp_item_get_image (item);

  if (which & GIMP_ITEM_LINKED_LAYERS)
    {
      for (list = GIMP_LIST (image->layers)->list;
           list;
           list = g_list_next (list))
        {
          linked_item = list->data;

          if (linked_item != item && gimp_item_get_linked (linked_item))
            linked_list = g_list_prepend (linked_list, linked_item);
        }
    }

  if (which & GIMP_ITEM_LINKED_CHANNELS)
    {
      for (list = GIMP_LIST (image->channels)->list;
           list;
           list = g_list_next (list))
        {
          linked_item = list->data;

          if (linked_item != item && gimp_item_get_linked (linked_item))
            linked_list = g_list_prepend (linked_list, linked_item);
        }
    }

  if (which & GIMP_ITEM_LINKED_VECTORS)
    {
      for (list = GIMP_LIST (image->vectors)->list;
           list;
           list = g_list_next (list))
        {
          linked_item = list->data;

          if (linked_item != item && gimp_item_get_linked (linked_item))
            linked_list = g_list_prepend (linked_list, linked_item);
        }
    }

  return g_list_reverse (linked_list);
}
