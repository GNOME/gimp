/* GIMP - The GNU Image Manipulation Program
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
#include "gimpimage-item-list.h"
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
  GList *list;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_get_linked (item) == TRUE);
  g_return_if_fail (gimp_item_is_attached (item));

  list = gimp_image_item_list_get_list (gimp_item_get_image (item), item,
                                        GIMP_ITEM_TYPE_ALL,
                                        GIMP_ITEM_SET_LINKED);

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

  gimp_image_item_list_rotate (gimp_item_get_image (item), list, context,
                               rotate_type, center_x, center_y, clip_result);

  g_list_free (list);

  list = gimp_image_item_list_get_list (gimp_item_get_image (item), item,
                                        GIMP_ITEM_TYPE_CHANNELS,
                                        GIMP_ITEM_SET_LINKED);

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
                            gint                    recursion_level,
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

  gimp_image_item_list_transform (gimp_item_get_image (item), list, context,
                                  matrix, direction,
                                  interpolation_type, recursion_level,
                                  clip_result, progress);

  g_list_free (list);
}
