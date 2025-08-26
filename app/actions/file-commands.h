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


void   file_open_cmd_callback                 (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   file_open_as_layers_cmd_callback       (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   file_open_as_link_layer_cmd_callback   (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   file_open_location_cmd_callback        (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   file_open_recent_cmd_callback          (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);

void   file_save_cmd_callback                 (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   file_create_template_cmd_callback      (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);

void   file_revert_cmd_callback               (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   file_close_all_cmd_callback            (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   file_copy_location_cmd_callback        (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   file_show_in_file_manager_cmd_callback (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);
void   file_quit_cmd_callback                 (GimpAction *action,
                                               GVariant   *value,
                                               gpointer    data);

void   file_file_open_dialog                  (Gimp       *gimp,
                                               GFile      *file,
                                               GtkWidget  *parent);
