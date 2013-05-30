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

#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimage-item-list.h"
#include "gimpitem.h"
#include "gimpitem-linked.h"
#include "gimplist.h"
#include "gimpprogress.h"


/*  public functions  */

gboolean
gimp_item_linked_is_locked (const GimpItem *item)
{
  GList    *list;
  GList    *l;
  gboolean  locked = FALSE;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_get_linked (item) == TRUE, FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);

  list = gimp_image_item_list_get_list (gimp_item_get_image (item), item,
                                        GIMP_ITEM_TYPE_ALL,
                                        GIMP_ITEM_SET_LINKED);

  list = gimp_image_item_list_filter (item, list, TRUE, FALSE);

  for (l = list; l && ! locked; l = g_list_next (l))
    {
      GimpItem *item = l->data;

      /*  temporarily set the item to not being linked, or we will
       *  run into a recursion because gimp_item_is_position_locked()
       *  call this function if the item is linked
       */
      gimp_item_set_linked (item, FALSE, FALSE);

      if (gimp_item_is_position_locked (item))
        locked = TRUE;

      gimp_item_set_linked (item, TRUE, FALSE);
    }

  g_list_free (list);

  return locked;
}

void
gimp_item_linked_translate (GimpItem *item,
                            gint      offset_x,
                            gint      offset_y,
                            gboolean  push_undo)
{
  GList *list;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));

  list = gimp_image_item_list_get_list (gimp_item_get_image (item), item,
                                        GIMP_ITEM_TYPE_ALL,
                                        GIMP_ITEM_SET_LINKED);

  list = gimp_image_item_list_filter (item, list, TRUE, FALSE);

  gimp_image_item_list_translate (gimp_item_get_image (item), list,
                                  offset_x, offset_y, push_undo);

  g_list_free (list);
}

void
gimp_item_linked_flip (GimpItem            *item,
                       GimpContext         *context,
                       GimpOrientationType  flip_type,
                       gdouble              axis,
                       gboolean             clip_result)
{
  GList *list;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));

  list = gimp_image_item_list_get_list (gimp_item_get_image (item), item,
                                        GIMP_ITEM_TYPE_ALL,
                                        GIMP_ITEM_SET_LINKED);

  list = gimp_image_item_list_filter (item, list, TRUE, FALSE);

  gimp_image_item_list_flip (gimp_item_get_image (item), list, context,
                             flip_type, axis, clip_result);

  g_list_free (list);
}

void
gimp_item_linked_rotate (GimpItem         *item,
                         GimpContext      *context,
                         GimpRotationType  rotate_type,
                         gdouble           center_x,
                         gdouble           center_y,
                         gboolean          clip_result)
{
  GList *list;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));

  list = gimp_image_item_list_get_list (gimp_item_get_image (item), item,
                                        GIMP_ITEM_TYPE_LAYERS |
                                        GIMP_ITEM_TYPE_VECTORS,
                                        GIMP_ITEM_SET_LINKED);

  list = gimp_image_item_list_filter (item, list, TRUE, FALSE);

  gimp_image_item_list_rotate (gimp_item_get_image (item), list, context,
                               rotate_type, center_x, center_y, clip_result);

  g_list_free (list);

  list = gimp_image_item_list_get_list (gimp_item_get_image (item), item,
                                        GIMP_ITEM_TYPE_CHANNELS,
                                        GIMP_ITEM_SET_LINKED);

  list = gimp_image_item_list_filter (item, list, TRUE, FALSE);

  gimp_image_item_list_rotate (gimp_item_get_image (item), list, context,
                               rotate_type, center_x, center_y, TRUE);

  g_list_free (list);
}

void
gimp_item_linked_transform (GimpItem               *item,
                            GimpContext            *context,
                            const GimpMatrix3      *matrix,
                            GimpTransformDirection  direction,
                            GimpInterpolationType   interpolation_type,
                            GimpTransformResize     clip_result,
                            GimpProgress           *progress)
{
  GList *list;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  list = gimp_image_item_list_get_list (gimp_item_get_image (item), item,
                                        GIMP_ITEM_TYPE_ALL,
                                        GIMP_ITEM_SET_LINKED);

  list = gimp_image_item_list_filter (item, list, TRUE, FALSE);

  gimp_image_item_list_transform (gimp_item_get_image (item), list, context,
                                  matrix, direction,
                                  interpolation_type,
                                  clip_result, progress);

  g_list_free (list);
}
