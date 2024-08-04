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

#ifndef __PATHS_COMMANDS_H__
#define __PATHS_COMMANDS_H__


void   paths_edit_cmd_callback                 (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_edit_attributes_cmd_callback      (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_new_cmd_callback                  (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_new_last_vals_cmd_callback        (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);

void   paths_raise_cmd_callback                (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_raise_to_top_cmd_callback         (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_lower_cmd_callback                (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_lower_to_bottom_cmd_callback      (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);

void   paths_duplicate_cmd_callback            (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_delete_cmd_callback               (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_merge_visible_cmd_callback        (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_to_selection_cmd_callback         (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_selection_to_paths_cmd_callback (GimpAction *action,
                                              GVariant   *value,
                                              gpointer    data);

void   paths_fill_cmd_callback                 (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_fill_last_vals_cmd_callback       (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_stroke_cmd_callback               (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_stroke_last_vals_cmd_callback     (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);

void   paths_copy_cmd_callback                 (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_paste_cmd_callback                (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_export_cmd_callback               (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_import_cmd_callback               (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);

void   paths_visible_cmd_callback              (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_lock_content_cmd_callback         (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);
void   paths_lock_position_cmd_callback        (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);

void   paths_color_tag_cmd_callback            (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);

void   paths_select_cmd_callback               (GimpAction *action,
                                                GVariant   *value,
                                                gpointer    data);


#endif /* __PATHS_COMMANDS_H__ */
