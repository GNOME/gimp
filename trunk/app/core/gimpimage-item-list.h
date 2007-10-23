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

#ifndef __GIMP_IMAGE_ITEM_LIST_H__
#define __GIMP_IMAGE_ITEM_LIST_H__


void    gimp_image_item_list_translate (GimpImage              *image,
                                        GList                  *list,
                                        gint                    offset_x,
                                        gint                    offset_y,
                                        gboolean                push_undo);
void    gimp_image_item_list_flip      (GimpImage              *image,
                                        GList                  *list,
                                        GimpContext            *context,
                                        GimpOrientationType     flip_type,
                                        gdouble                 axis,
                                        gboolean                clip_result);
void    gimp_image_item_list_rotate    (GimpImage              *image,
                                        GList                  *list,
                                        GimpContext            *context,
                                        GimpRotationType        rotate_type,
                                        gdouble                 center_x,
                                        gdouble                 center_y,
                                        gboolean                clip_result);
void    gimp_image_item_list_transform (GimpImage              *image,
                                        GList                  *list,
                                        GimpContext            *context,
                                        const GimpMatrix3      *matrix,
                                        GimpTransformDirection  direction,
                                        GimpInterpolationType   interpolation_type,
                                        gint                    recursion_level,
                                        GimpTransformResize     clip_result,
                                        GimpProgress           *progress);

GList * gimp_image_item_list_get_list  (GimpImage              *image,
                                        GimpItem               *exclude,
                                        GimpItemTypeMask        type,
                                        GimpItemSet             set);


#endif /* __GIMP_IMAGE_ITEM_LIST_H__ */
