/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_DRAWABLE_TRANSFORM_H__
#define __LIGMA_DRAWABLE_TRANSFORM_H__


GeglBuffer          * ligma_drawable_transform_buffer_affine      (LigmaDrawable            *drawable,
                                                                  LigmaContext             *context,
                                                                  GeglBuffer              *orig_buffer,
                                                                  gint                     orig_offset_x,
                                                                  gint                     orig_offset_y,
                                                                  const LigmaMatrix3       *matrix,
                                                                  LigmaTransformDirection   direction,
                                                                  LigmaInterpolationType    interpolation_type,
                                                                  LigmaTransformResize      clip_result,
                                                                  LigmaColorProfile       **buffer_profile,
                                                                  gint                    *new_offset_x,
                                                                  gint                    *new_offset_y,
                                                                  LigmaProgress            *progress);
GeglBuffer          * ligma_drawable_transform_buffer_flip        (LigmaDrawable            *drawable,
                                                                  LigmaContext             *context,
                                                                  GeglBuffer              *orig_buffer,
                                                                  gint                     orig_offset_x,
                                                                  gint                     orig_offset_y,
                                                                  LigmaOrientationType      flip_type,
                                                                  gdouble                  axis,
                                                                  gboolean                 clip_result,
                                                                  LigmaColorProfile       **buffer_profile,
                                                                  gint                    *new_offset_x,
                                                                  gint                    *new_offset_y);

GeglBuffer          * ligma_drawable_transform_buffer_rotate      (LigmaDrawable            *drawable,
                                                                  LigmaContext             *context,
                                                                  GeglBuffer              *buffer,
                                                                  gint                     orig_offset_x,
                                                                  gint                     orig_offset_y,
                                                                  LigmaRotationType         rotate_type,
                                                                  gdouble                  center_x,
                                                                  gdouble                  center_y,
                                                                  gboolean                 clip_result,
                                                                  LigmaColorProfile       **buffer_profile,
                                                                  gint                    *new_offset_x,
                                                                  gint                    *new_offset_y);

LigmaDrawable         * ligma_drawable_transform_affine            (LigmaDrawable            *drawable,
                                                                  LigmaContext             *context,
                                                                  const LigmaMatrix3       *matrix,
                                                                  LigmaTransformDirection   direction,
                                                                  LigmaInterpolationType    interpolation_type,
                                                                  LigmaTransformResize      clip_result,
                                                                  LigmaProgress            *progress);

LigmaDrawable         * ligma_drawable_transform_flip              (LigmaDrawable            *drawable,
                                                                  LigmaContext             *context,
                                                                  LigmaOrientationType      flip_type,
                                                                  gdouble                  axis,
                                                                  gboolean                 clip_result);

LigmaDrawable         * ligma_drawable_transform_rotate            (LigmaDrawable            *drawable,
                                                                  LigmaContext             *context,
                                                                  LigmaRotationType         rotate_type,
                                                                  gdouble                  center_x,
                                                                  gdouble                  center_y,
                                                                  gboolean                 clip_result);

GeglBuffer           * ligma_drawable_transform_cut               (GList                   *drawables,
                                                                  LigmaContext             *context,
                                                                  gint                    *offset_x,
                                                                  gint                    *offset_y,
                                                                  gboolean                *new_layer);
LigmaDrawable         * ligma_drawable_transform_paste             (LigmaDrawable            *drawable,
                                                                  GeglBuffer              *buffer,
                                                                  LigmaColorProfile        *buffer_profile,
                                                                  gint                     offset_x,
                                                                  gint                     offset_y,
                                                                  gboolean                 new_layer);


#endif  /*  __LIGMA_DRAWABLE_TRANSFORM_H__  */
