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

#pragma once


void   image_new_cmd_callback                      (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_duplicate_cmd_callback                (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);

void   image_convert_base_type_cmd_callback        (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_convert_precision_cmd_callback        (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_convert_trc_cmd_callback              (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);

void   image_color_profile_use_srgb_cmd_callback   (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_color_profile_assign_cmd_callback     (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_color_profile_convert_cmd_callback    (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_color_profile_discard_cmd_callback    (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_color_profile_save_cmd_callback       (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);

void   image_resize_cmd_callback                   (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_resize_to_layers_cmd_callback         (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_resize_to_selection_cmd_callback      (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_print_size_cmd_callback               (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_scale_cmd_callback                    (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_flip_cmd_callback                     (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_rotate_cmd_callback                   (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_crop_to_selection_cmd_callback        (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_crop_to_content_cmd_callback          (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);

void   image_merge_layers_cmd_callback             (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_merge_layers_last_vals_cmd_callback   (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_flatten_image_cmd_callback            (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);

void   image_configure_grid_cmd_callback           (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);
void   image_properties_cmd_callback               (GimpAction *action,
                                                    GVariant   *value,
                                                    gpointer    data);

void   image_softproof_profile_cmd_callback          (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   image_softproof_intent_cmd_callback           (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   image_softproof_bpc_cmd_callback              (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
