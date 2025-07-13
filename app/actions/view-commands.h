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


void   view_new_cmd_callback                        (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_close_cmd_callback                      (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_scroll_center_cmd_callback              (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_zoom_fit_in_cmd_callback                (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_zoom_fill_cmd_callback                  (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_zoom_selection_cmd_callback             (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_zoom_revert_cmd_callback                (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_zoom_cmd_callback                       (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_zoom_explicit_cmd_callback              (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_show_all_cmd_callback                   (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_dot_for_dot_cmd_callback                (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_scroll_horizontal_cmd_callback          (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_scroll_vertical_cmd_callback            (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_flip_horizontally_cmd_callback          (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_flip_vertically_cmd_callback            (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_flip_reset_cmd_callback                 (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_rotate_absolute_cmd_callback            (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_rotate_relative_cmd_callback            (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_rotate_other_cmd_callback               (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_reset_cmd_callback                      (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_navigation_window_cmd_callback          (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_display_filters_cmd_callback            (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_color_management_reset_cmd_callback     (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_color_management_enable_cmd_callback    (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_color_management_softproof_cmd_callback (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_display_intent_cmd_callback             (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_display_bpc_cmd_callback                (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_softproof_gamut_check_cmd_callback      (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_toggle_selection_cmd_callback           (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_toggle_layer_boundary_cmd_callback      (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_toggle_canvas_boundary_cmd_callback     (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_toggle_menubar_cmd_callback             (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_toggle_rulers_cmd_callback              (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_toggle_scrollbars_cmd_callback          (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_toggle_statusbar_cmd_callback           (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_toggle_guides_cmd_callback              (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_toggle_grid_cmd_callback                (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_toggle_sample_points_cmd_callback       (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_snap_to_guides_cmd_callback             (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_snap_to_grid_cmd_callback               (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_snap_to_canvas_cmd_callback             (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_snap_to_path_cmd_callback               (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_snap_to_bbox_cmd_callback               (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_snap_to_equidistance_cmd_callback       (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_padding_color_cmd_callback              (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_padding_color_in_show_all_cmd_callback  (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);

void   view_shrink_wrap_cmd_callback                (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
void   view_fullscreen_cmd_callback                 (GimpAction *action,
                                                     GVariant   *value,
                                                     gpointer    data);
