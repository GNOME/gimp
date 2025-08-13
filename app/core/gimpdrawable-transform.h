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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


GeglBuffer          * gimp_drawable_transform_buffer_affine      (GimpDrawable            *drawable,
                                                                  GimpContext             *context,
                                                                  GeglBuffer              *orig_buffer,
                                                                  gint                     orig_offset_x,
                                                                  gint                     orig_offset_y,
                                                                  const GimpMatrix3       *matrix,
                                                                  GimpTransformDirection   direction,
                                                                  GimpInterpolationType    interpolation_type,
                                                                  GimpTransformResize      clip_result,
                                                                  GimpColorProfile       **buffer_profile,
                                                                  gint                    *new_offset_x,
                                                                  gint                    *new_offset_y,
                                                                  GimpProgress            *progress);
GeglBuffer          * gimp_drawable_transform_buffer_flip        (GimpDrawable            *drawable,
                                                                  GimpContext             *context,
                                                                  GeglBuffer              *orig_buffer,
                                                                  gint                     orig_offset_x,
                                                                  gint                     orig_offset_y,
                                                                  GimpOrientationType      flip_type,
                                                                  gdouble                  axis,
                                                                  gboolean                 clip_result,
                                                                  GimpColorProfile       **buffer_profile,
                                                                  gint                    *new_offset_x,
                                                                  gint                    *new_offset_y);

GeglBuffer          * gimp_drawable_transform_buffer_rotate      (GimpDrawable            *drawable,
                                                                  GimpContext             *context,
                                                                  GeglBuffer              *buffer,
                                                                  gint                     orig_offset_x,
                                                                  gint                     orig_offset_y,
                                                                  GimpRotationType         rotate_type,
                                                                  gdouble                  center_x,
                                                                  gdouble                  center_y,
                                                                  gboolean                 clip_result,
                                                                  GimpColorProfile       **buffer_profile,
                                                                  gint                    *new_offset_x,
                                                                  gint                    *new_offset_y);

GimpDrawable         * gimp_drawable_transform_affine            (GimpDrawable            *drawable,
                                                                  GimpContext             *context,
                                                                  const GimpMatrix3       *matrix,
                                                                  GimpTransformDirection   direction,
                                                                  GimpInterpolationType    interpolation_type,
                                                                  GimpTransformResize      clip_result,
                                                                  GimpProgress            *progress);

GimpDrawable         * gimp_drawable_transform_flip              (GimpDrawable            *drawable,
                                                                  GimpContext             *context,
                                                                  GimpOrientationType      flip_type,
                                                                  gdouble                  axis,
                                                                  gboolean                 clip_result);

GimpDrawable         * gimp_drawable_transform_rotate            (GimpDrawable            *drawable,
                                                                  GimpContext             *context,
                                                                  GimpRotationType         rotate_type,
                                                                  gdouble                  center_x,
                                                                  gdouble                  center_y,
                                                                  gboolean                 clip_result);

GeglBuffer           * gimp_drawable_transform_cut               (GList                   *drawables,
                                                                  GimpContext             *context,
                                                                  gint                    *offset_x,
                                                                  gint                    *offset_y,
                                                                  gboolean                *new_layer);
GimpDrawable         * gimp_drawable_transform_paste             (GimpDrawable            *drawable,
                                                                  GeglBuffer              *buffer,
                                                                  GimpColorProfile        *buffer_profile,
                                                                  gint                     offset_x,
                                                                  gint                     offset_y,
                                                                  gboolean                 new_layer,
                                                                  gboolean                 push_undo);
