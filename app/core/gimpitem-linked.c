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

#include "gimpimage.h"
#include "gimpitem.h"
#include "gimpitem-linked.h"
#include "gimplist.h"


void
gimp_item_linked_translate (GimpItem *item,
                            gint      offset_x,
                            gint      offset_y,
                            gboolean  push_undo)
{
  GimpImage *gimage;
  GimpItem  *linked_item;
  GList     *list;

  g_return_if_fail (GIMP_IS_ITEM (item));

  gimage = gimp_item_get_image (item);

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      linked_item = (GimpItem *) list->data;

      if (linked_item != item && gimp_item_get_linked (linked_item))
        gimp_item_translate (linked_item, offset_x, offset_y, push_undo);
    }

  for (list = GIMP_LIST (gimage->channels)->list;
       list;
       list = g_list_next (list))
    {
      linked_item = (GimpItem *) list->data;

      if (linked_item != item && gimp_item_get_linked (linked_item))
        gimp_item_translate (linked_item, offset_x, offset_y, push_undo);
    }

  for (list = GIMP_LIST (gimage->vectors)->list;
       list;
       list = g_list_next (list))
    {
      linked_item = (GimpItem *) list->data;

      if (linked_item != item && gimp_item_get_linked (linked_item))
        gimp_item_translate (linked_item, offset_x, offset_y, push_undo);
    }
}

void
gimp_item_linked_flip (GimpItem            *item,
                       GimpOrientationType  flip_type,
                       gdouble              axis)
{
  GimpImage *gimage;
  GimpItem  *linked_item;
  GList     *list;

  g_return_if_fail (GIMP_IS_ITEM (item));

  gimage = gimp_item_get_image (item);

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      linked_item = (GimpItem *) list->data;

      if (linked_item != item && gimp_item_get_linked (linked_item))
        gimp_item_flip (linked_item, flip_type, axis);
    }

  for (list = GIMP_LIST (gimage->channels)->list;
       list;
       list = g_list_next (list))
    {
      linked_item = (GimpItem *) list->data;

      if (linked_item != item && gimp_item_get_linked (linked_item))
        gimp_item_flip (linked_item, flip_type, axis);
    }

  for (list = GIMP_LIST (gimage->vectors)->list;
       list;
       list = g_list_next (list))
    {
      linked_item = (GimpItem *) list->data;

      if (linked_item != item && gimp_item_get_linked (linked_item))
        gimp_item_flip (linked_item, flip_type, axis);
    }
}

void
gimp_item_linked_transform (GimpItem               *item,
                            GimpMatrix3             matrix,
                            GimpTransformDirection  direction,
                            GimpInterpolationType   interpolation_type,
                            gboolean                clip_result,
                            GimpProgressFunc        progress_callback,
                            gpointer                progress_data)
{
  GimpImage *gimage;
  GimpItem  *linked_item;
  GList     *list;

  g_return_if_fail (GIMP_IS_ITEM (item));

  gimage = gimp_item_get_image (item);

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      linked_item = (GimpItem *) list->data;

      if (linked_item != item && gimp_item_get_linked (linked_item))
        gimp_item_transform (linked_item, matrix, direction,
                             interpolation_type, clip_result,
                             progress_callback, progress_data);
    }

  for (list = GIMP_LIST (gimage->channels)->list;
       list;
       list = g_list_next (list))
    {
      linked_item = (GimpItem *) list->data;

      if (linked_item != item && gimp_item_get_linked (linked_item))
        gimp_item_transform (linked_item, matrix, direction,
                             interpolation_type, clip_result,
                             progress_callback, progress_data);
    }

  for (list = GIMP_LIST (gimage->vectors)->list;
       list;
       list = g_list_next (list))
    {
      linked_item = (GimpItem *) list->data;

      if (linked_item != item && gimp_item_get_linked (linked_item))
        gimp_item_transform (linked_item, matrix, direction,
                             interpolation_type, clip_result,
                             progress_callback, progress_data);
    }
}
