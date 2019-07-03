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

#ifndef __EDIT_COMMANDS_H__
#define __EDIT_COMMANDS_H__


void   edit_undo_cmd_callback               (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   edit_redo_cmd_callback               (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   edit_strong_undo_cmd_callback        (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   edit_strong_redo_cmd_callback        (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   edit_undo_clear_cmd_callback         (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);

void   edit_cut_cmd_callback                (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   edit_copy_cmd_callback               (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   edit_copy_visible_cmd_callback       (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);

void   edit_paste_cmd_callback              (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   edit_paste_as_new_image_cmd_callback (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);

void   edit_named_cut_cmd_callback          (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   edit_named_copy_cmd_callback         (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   edit_named_copy_visible_cmd_callback (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   edit_named_paste_cmd_callback        (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);

void   edit_clear_cmd_callback              (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   edit_fill_cmd_callback               (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);


#endif /* __EDIT_COMMANDS_H__ */
