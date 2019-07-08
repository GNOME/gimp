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

#ifndef __GRADIENT_EDITOR_COMMANDS_H__
#define __GRADIENT_EDITOR_COMMANDS_H__


enum
{
  GRADIENT_EDITOR_COLOR_NEIGHBOR_ENDPOINT,
  GRADIENT_EDITOR_COLOR_OTHER_ENDPOINT,
  GRADIENT_EDITOR_COLOR_FOREGROUND,
  GRADIENT_EDITOR_COLOR_BACKGROUND,
  GRADIENT_EDITOR_COLOR_FIRST_CUSTOM
};


void   gradient_editor_left_color_cmd_callback       (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_left_color_type_cmd_callback  (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_load_left_cmd_callback        (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_save_left_cmd_callback        (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);

void   gradient_editor_right_color_cmd_callback      (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_right_color_type_cmd_callback (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_load_right_cmd_callback       (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_save_right_cmd_callback       (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);

void   gradient_editor_blending_func_cmd_callback    (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_coloring_type_cmd_callback    (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);

void   gradient_editor_flip_cmd_callback             (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_replicate_cmd_callback        (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_split_midpoint_cmd_callback   (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_split_uniformly_cmd_callback  (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_delete_cmd_callback           (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_recenter_cmd_callback         (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_redistribute_cmd_callback     (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);

void   gradient_editor_blend_color_cmd_callback      (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   gradient_editor_blend_opacity_cmd_callback    (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);

void   gradient_editor_zoom_cmd_callback             (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);


#endif /* __GRADIENT_EDITOR_COMMANDS_H__ */
