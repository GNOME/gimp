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

#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimage-item-list.h"
#include "gimpimage-undo.h"
#include "gimpitem.h"
#include "gimpitem-linked.h"
#include "gimplist.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


/*  public functions  */

gboolean
gimp_item_linked_is_locked (GimpItem *item)
{
  GList    *list;
  GList    *l;
  gboolean  locked = FALSE;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_get_linked (item) == TRUE, FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);

  list = gimp_image_item_list_get_list (gimp_item_get_image (item),
                                        GIMP_ITEM_TYPE_ALL,
                                        GIMP_ITEM_SET_LINKED);

  list = gimp_image_item_list_filter (list);

  for (l = list; l && ! locked; l = g_list_next (l))
    {
      /* We must not use gimp_item_is_position_locked(), especially
       * since a child implementation may call the current function and
       * end up in infinite loop.
       * We are only interested in the value of `lock_position` flag.
       */
      if (gimp_item_get_lock_position (l->data))
        locked = TRUE;
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
  GimpImage *image;
  GList     *items;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));

  image = gimp_item_get_image (item);

  items = gimp_image_item_list_get_list (image,
                                         GIMP_ITEM_TYPE_ALL,
                                         GIMP_ITEM_SET_LINKED);

  items = gimp_image_item_list_filter (items);

  gimp_image_item_list_translate (gimp_item_get_image (item), items,
                                  offset_x, offset_y, push_undo);

  g_list_free (items);
}

void
gimp_item_linked_flip (GimpItem            *item,
                       GimpContext         *context,
                       GimpOrientationType  flip_type,
                       gdouble              axis,
                       gboolean             clip_result)
{
  GimpImage *image;
  GList     *items;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));

  image = gimp_item_get_image (item);

  items = gimp_image_item_list_get_list (image,
                                         GIMP_ITEM_TYPE_ALL,
                                         GIMP_ITEM_SET_LINKED);
  items = gimp_image_item_list_filter (items);

  gimp_image_item_list_flip (image, items, context,
                             flip_type, axis, clip_result);

  g_list_free (items);
}

void
gimp_item_linked_rotate (GimpItem         *item,
                         GimpContext      *context,
                         GimpRotationType  rotate_type,
                         gdouble           center_x,
                         gdouble           center_y,
                         gboolean          clip_result)
{
  GimpImage *image;
  GList     *items;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));

  image = gimp_item_get_image (item);

  items = gimp_image_item_list_get_list (image,
                                         GIMP_ITEM_TYPE_ALL,
                                         GIMP_ITEM_SET_LINKED);
  items = gimp_image_item_list_filter (items);

  gimp_image_item_list_rotate (image, items, context,
                               rotate_type, center_x, center_y, clip_result);

  g_list_free (items);
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
  GimpImage *image;
  GList     *items;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  image = gimp_item_get_image (item);

  items = gimp_image_item_list_get_list (image,
                                         GIMP_ITEM_TYPE_ALL,
                                         GIMP_ITEM_SET_LINKED);
  items = gimp_image_item_list_filter (items);

  gimp_image_item_list_transform (image, items, context,
                                  matrix, direction,
                                  interpolation_type,
                                  clip_result, progress);

  g_list_free (items);
}
