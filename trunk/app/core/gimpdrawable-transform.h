/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#ifndef __GIMP_DRAWABLE_TRANSFORM_H__
#define __GIMP_DRAWABLE_TRANSFORM_H__


typedef enum
{
  X0,
  Y0,
  X1,
  Y1,
  X2,
  Y2,
  X3,
  Y3
} GimpTransformBoundingBox;


TileManager * gimp_drawable_transform_tiles_affine (GimpDrawable           *drawable,
                                                    GimpContext            *context,
                                                    TileManager            *orig_tiles,
                                                    const GimpMatrix3      *matrix,
                                                    GimpTransformDirection  direction,
                                                    GimpInterpolationType   interpolation_type,
                                                    gint                    recursion_level,
                                                    GimpTransformResize     clip_result,
                                                    GimpProgress           *progress);
TileManager * gimp_drawable_transform_tiles_flip   (GimpDrawable           *drawable,
                                                    GimpContext            *context,
                                                    TileManager            *orig_tiles,
                                                    GimpOrientationType     flip_type,
                                                    gdouble                 axis,
                                                    gboolean                clip_result);

TileManager * gimp_drawable_transform_tiles_rotate (GimpDrawable           *drawable,
                                                    GimpContext            *context,
                                                    TileManager            *orig_tiles,
                                                    GimpRotationType        rotate_type,
                                                    gdouble                 center_x,
                                                    gdouble                 center_y,
                                                    gboolean                clip_result);

gboolean      gimp_drawable_transform_affine       (GimpDrawable           *drawable,
                                                    GimpContext            *context,
                                                    const GimpMatrix3      *matrix,
                                                    GimpTransformDirection  direction,
                                                    GimpInterpolationType   interpolation_type,
                                                    gint                    recursion_level,
                                                    GimpTransformResize     clip_result,
                                                    GimpProgress           *progress);

gboolean      gimp_drawable_transform_flip         (GimpDrawable           *drawable,
                                                    GimpContext            *context,
                                                    GimpOrientationType     flip_type,
                                                    gboolean                auto_center,
                                                    gdouble                 axis,
                                                    gboolean                clip_result);

gboolean      gimp_drawable_transform_rotate       (GimpDrawable           *drawable,
                                                    GimpContext            *context,
                                                    GimpRotationType        rotate_type,
                                                    gboolean                auto_center,
                                                    gdouble                 center_x,
                                                    gdouble                 center_y,
                                                    gboolean                clip_result);

TileManager * gimp_drawable_transform_cut          (GimpDrawable           *drawable,
                                                    GimpContext            *context,
                                                    gboolean               *new_layer);
gboolean      gimp_drawable_transform_paste        (GimpDrawable           *drawable,
                                                    TileManager            *tiles,
                                                    gboolean                new_layer);


#endif  /*  __GIMP_DRAWABLE_TRANSFORM_H__  */
