/* The GIMP -- an image manipulation program
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


TileManager * gimp_drawable_transform_tiles_affine (GimpDrawable *drawable,
                                                    TileManager  *float_tiles,
                                                    gboolean      interpolation,
                                                    gboolean      clip_result,
                                                    GimpMatrix3   matrix,
                                                    GimpTransformDirection direction,
                                                    GimpProgressFunc progress_callback,
                                                    gpointer      progress_data);
TileManager * gimp_drawable_transform_tiles_flip   (GimpDrawable *drawable,
                                                    TileManager  *orig,
                                                    InternalOrientationType flip_type);

gboolean      gimp_drawable_transform_affine       (GimpDrawable *drawable,
                                                    gboolean      interpolation,
                                                    gboolean      clip_result,
                                                    GimpMatrix3   matrix,
                                                    GimpTransformDirection direction);
gboolean      gimp_drawable_transform_flip         (GimpDrawable *drawable,
                                                    InternalOrientationType flip_type);


TileManager * gimp_drawable_transform_cut          (GimpDrawable *drawable,
                                                    gboolean     *new_layer);
gboolean      gimp_drawable_transform_paste        (GimpDrawable *drawable,
                                                    TileManager  *tiles,
                                                    gboolean      new_layer);


#endif  /*  __GIMP_DRAWABLE_TRANSFORM_H__  */
