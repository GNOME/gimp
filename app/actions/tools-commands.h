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

#ifndef __TOOLS_COMMANDS_H__
#define __TOOLS_COMMANDS_H__


void   tools_select_cmd_callback                    (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   tools_color_average_radius_cmd_callback      (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   tools_paintbrush_pixel_size_cmd_callback     (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_paintbrush_size_cmd_callback           (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_paintbrush_angle_cmd_callback          (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_paintbrush_aspect_ratio_cmd_callback   (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_paintbrush_spacing_cmd_callback        (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_paintbrush_hardness_cmd_callback       (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_paintbrush_force_cmd_callback          (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   tools_ink_blob_pixel_size_cmd_callback       (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_ink_blob_size_cmd_callback             (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_ink_blob_aspect_cmd_callback           (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_ink_blob_angle_cmd_callback            (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   tools_airbrush_rate_cmd_callback             (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_airbrush_flow_cmd_callback             (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   tools_mybrush_radius_cmd_callback            (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_mybrush_pixel_size_cmd_callback        (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_mybrush_hardness_cmd_callback          (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   tools_fg_select_brush_size_cmd_callback      (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   tools_transform_preview_opacity_cmd_callback (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   tools_warp_effect_pixel_size_cmd_callback    (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_warp_effect_size_cmd_callback          (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_warp_effect_hardness_cmd_callback      (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   tools_opacity_cmd_callback                   (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_size_cmd_callback                      (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_aspect_cmd_callback                    (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_angle_cmd_callback                     (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_spacing_cmd_callback                   (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_hardness_cmd_callback                  (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_force_cmd_callback                     (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   tools_paint_select_pixel_size_cmd_callback   (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   tools_object_1_cmd_callback                  (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   tools_object_2_cmd_callback                  (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);


#endif /* __TOOLS_COMMANDS_H__ */
