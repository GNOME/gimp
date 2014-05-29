/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-apply-operation.h
 * Copyright (C) 2012 Øyvind Kolås <pippin@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
 *                    Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_GEGL_APPLY_OPERATION_H__
#define __GIMP_GEGL_APPLY_OPERATION_H__


/*  generic function, also used by the specific ones below  */

void   gimp_gegl_apply_operation       (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglNode              *operation,
                                        GeglBuffer            *dest_buffer,
                                        const GeglRectangle   *dest_rect);


/*  apply specific operations  */

void   gimp_gegl_apply_color_reduction (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer,
                                        gint                   bits,
                                        gint                   dither_type);

void   gimp_gegl_apply_flatten         (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer,
                                        const GimpRGB         *background);

void   gimp_gegl_apply_feather         (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer,
                                        const GeglRectangle   *dest_rect,
                                        gdouble                radius_x,
                                        gdouble                radius_y);

void   gimp_gegl_apply_border          (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer,
                                        const GeglRectangle   *dest_rect,
                                        gint                   radius_x,
                                        gint                   radius_y,
                                        gboolean               feather,
                                        gboolean               edge_lock);

void   gimp_gegl_apply_grow            (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer,
                                        const GeglRectangle   *dest_rect,
                                        gint                   radius_x,
                                        gint                   radius_y);

void   gimp_gegl_apply_shrink          (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer,
                                        const GeglRectangle   *dest_rect,
                                        gint                   radius_x,
                                        gint                   radius_y,
                                        gboolean               edge_lock);

void   gimp_gegl_apply_gaussian_blur   (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer,
                                        const GeglRectangle   *dest_rect,
                                        gdouble                std_dev_x,
                                        gdouble                std_dev_y);

void   gimp_gegl_apply_invert_gamma    (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer);

void   gimp_gegl_apply_invert_linear   (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer);

void   gimp_gegl_apply_opacity         (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer,
                                        GeglBuffer            *mask,
                                        gint                   mask_offset_x,
                                        gint                   mask_offset_y,
                                        gdouble                opacity);

void   gimp_gegl_apply_scale           (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer,
                                        GimpInterpolationType  interpolation_type,
                                        gdouble                x,
                                        gdouble                y);

void   gimp_gegl_apply_set_alpha       (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer,
                                        gdouble                value);

void   gimp_gegl_apply_threshold       (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer,
                                        gdouble                value);

void   gimp_gegl_apply_transform       (GeglBuffer            *src_buffer,
                                        GimpProgress          *progress,
                                        const gchar           *undo_desc,
                                        GeglBuffer            *dest_buffer,
                                        GimpInterpolationType  interpolation_type,
                                        GimpMatrix3           *transform);


#endif /* __GIMP_GEGL_APPLY_OPERATION_H__ */
