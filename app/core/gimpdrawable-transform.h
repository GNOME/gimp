/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#ifndef __GIMP_DRAWABLE_TRANSFORM_H__
#define __GIMP_DRAWABLE_TRANSFORM_H__


TileManager  * gimp_drawable_transform_tiles_affine (GimpDrawable           *drawable,
                                                     GimpContext            *context,
                                                     TileManager            *orig_tiles,
                                                     gint                    orig_offset_x,
                                                     gint                    orig_offset_y,
                                                     const GimpMatrix3      *matrix,
                                                     GimpTransformDirection  direction,
                                                     GimpInterpolationType   interpolation_type,
                                                     gint                    recursion_level,
                                                     GimpTransformResize     clip_result,
                                                     gint                   *new_offset_x,
                                                     gint                   *new_offset_y,
                                                     GimpProgress           *progress);
TileManager  * gimp_drawable_transform_tiles_flip   (GimpDrawable           *drawable,
                                                     GimpContext            *context,
                                                     TileManager            *orig_tiles,
                                                     gint                    orig_offset_x,
                                                     gint                    orig_offset_y,
                                                     GimpOrientationType     flip_type,
                                                     gdouble                 axis,
                                                     gboolean                clip_result,
                                                     gint                   *new_offset_x,
                                                     gint                   *new_offset_y);

TileManager  * gimp_drawable_transform_tiles_rotate (GimpDrawable           *drawable,
                                                     GimpContext            *context,
                                                     TileManager            *orig_tiles,
                                                     gint                    orig_offset_x,
                                                     gint                    orig_offset_y,
                                                     GimpRotationType        rotate_type,
                                                     gdouble                 center_x,
                                                     gdouble                 center_y,
                                                     gboolean                clip_result,
                                                     gint                   *new_offset_x,
                                                     gint                   *new_offset_y);

GimpDrawable * gimp_drawable_transform_affine       (GimpDrawable           *drawable,
                                                     GimpContext            *context,
                                                     const GimpMatrix3      *matrix,
                                                     GimpTransformDirection  direction,
                                                     GimpInterpolationType   interpolation_type,
                                                     gint                    recursion_level,
                                                     GimpTransformResize     clip_result,
                                                     GimpProgress           *progress);

GimpDrawable * gimp_drawable_transform_flip         (GimpDrawable           *drawable,
                                                     GimpContext            *context,
                                                     GimpOrientationType     flip_type,
                                                     gdouble                 axis,
                                                     gboolean                clip_result);

GimpDrawable * gimp_drawable_transform_rotate       (GimpDrawable           *drawable,
                                                     GimpContext            *context,
                                                     GimpRotationType        rotate_type,
                                                     gdouble                 center_x,
                                                     gdouble                 center_y,
                                                     gboolean                clip_result);

TileManager  * gimp_drawable_transform_cut          (GimpDrawable           *drawable,
                                                     GimpContext            *context,
                                                     gint                   *offset_x,
                                                     gint                   *offset_y,
                                                     gboolean               *new_layer);
GimpDrawable * gimp_drawable_transform_paste        (GimpDrawable           *drawable,
                                                     TileManager            *tiles,
                                                     gint                    offset_x,
                                                     gint                    offset_y,
                                                     gboolean                new_layer);


#endif  /*  __GIMP_DRAWABLE_TRANSFORM_H__  */
