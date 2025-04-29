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


void   text_tool_cut_cmd_callback             (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   text_tool_copy_cmd_callback            (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   text_tool_paste_cmd_callback           (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   text_tool_toggle_bold_cmd_callback     (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   text_tool_toggle_italic_cmd_callback   (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   text_tool_toggle_underline_cmd_callback
                                              (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   text_tool_delete_cmd_callback          (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   text_tool_load_cmd_callback            (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   text_tool_clear_cmd_callback           (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   text_tool_text_to_path_cmd_callback    (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   text_tool_text_along_path_cmd_callback (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   text_tool_direction_cmd_callback       (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
