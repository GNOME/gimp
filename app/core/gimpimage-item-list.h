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

#ifndef __LIGMA_IMAGE_ITEM_LIST_H__
#define __LIGMA_IMAGE_ITEM_LIST_H__


gboolean   ligma_image_item_list_bounds    (LigmaImage              *image,
                                           GList                  *list,
                                           gint                   *x,
                                           gint                   *y,
                                           gint                   *width,
                                           gint                   *height);

void       ligma_image_item_list_translate (LigmaImage              *image,
                                           GList                  *list,
                                           gint                    offset_x,
                                           gint                    offset_y,
                                           gboolean                push_undo);
void       ligma_image_item_list_flip      (LigmaImage              *image,
                                           GList                  *list,
                                           LigmaContext            *context,
                                           LigmaOrientationType     flip_type,
                                           gdouble                 axis,
                                           LigmaTransformResize     expected_clip_result);
void       ligma_image_item_list_rotate    (LigmaImage              *image,
                                           GList                  *list,
                                           LigmaContext            *context,
                                           LigmaRotationType        rotate_type,
                                           gdouble                 center_x,
                                           gdouble                 center_y,
                                           gboolean                clip_result);
void       ligma_image_item_list_transform (LigmaImage              *image,
                                           GList                  *list,
                                           LigmaContext            *context,
                                           const LigmaMatrix3      *matrix,
                                           LigmaTransformDirection  direction,
                                           LigmaInterpolationType   interpolation_type,
                                           LigmaTransformResize     clip_result,
                                           LigmaProgress           *progress);

GList    * ligma_image_item_list_get_list  (LigmaImage              *image,
                                           LigmaItemTypeMask        type,
                                           LigmaItemSet             set);

GList    * ligma_image_item_list_filter    (GList                  *list);


#endif /* __LIGMA_IMAGE_ITEM_LIST_H__ */
