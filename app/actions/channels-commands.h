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


void   channels_edit_attributes_cmd_callback (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);
void   channels_new_cmd_callback             (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);
void   channels_new_last_vals_cmd_callback   (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);

void   channels_raise_cmd_callback           (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);
void   channels_raise_to_top_cmd_callback    (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);
void   channels_lower_cmd_callback           (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);
void   channels_lower_to_bottom_cmd_callback (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);

void   channels_duplicate_cmd_callback       (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);
void   channels_delete_cmd_callback          (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);
void   channels_to_selection_cmd_callback    (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);

void   channels_visible_cmd_callback         (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);
void   channels_lock_content_cmd_callback    (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);
void   channels_lock_position_cmd_callback   (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);

void   channels_color_tag_cmd_callback       (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);

void   channels_select_cmd_callback          (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);
