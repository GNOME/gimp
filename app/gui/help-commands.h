/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "gtk/gtk.h"

void file_new_cmd_callback     (GtkWidget *, gpointer, guint);
void file_open_cmd_callback    (GtkWidget *, gpointer);
void file_save_cmd_callback    (GtkWidget *, gpointer);
void file_save_as_cmd_callback (GtkWidget *, gpointer);
void file_revert_cmd_callback  (GtkWidget *, gpointer);
void file_pref_cmd_callback    (GtkWidget *, gpointer);
void file_close_cmd_callback   (GtkWidget *, gpointer);
void file_quit_cmd_callback    (GtkWidget *, gpointer);

void edit_undo_cmd_callback         (GtkWidget *, gpointer);
void edit_redo_cmd_callback         (GtkWidget *, gpointer);
void edit_cut_cmd_callback          (GtkWidget *, gpointer);
void edit_copy_cmd_callback         (GtkWidget *, gpointer);
void edit_paste_cmd_callback        (GtkWidget *, gpointer);
void edit_paste_into_cmd_callback   (GtkWidget *, gpointer);
void edit_paste_as_new_cmd_callback (GtkWidget *, gpointer);
void edit_named_cut_cmd_callback    (GtkWidget *, gpointer);
void edit_named_copy_cmd_callback   (GtkWidget *, gpointer);
void edit_named_paste_cmd_callback  (GtkWidget *, gpointer);
void edit_clear_cmd_callback        (GtkWidget *, gpointer);
void edit_fill_cmd_callback         (GtkWidget *widget,
				     gpointer   callback_data,
				     guint      callback_action);
void edit_stroke_cmd_callback       (GtkWidget *, gpointer);

void select_invert_cmd_callback   (GtkWidget *, gpointer);
void select_all_cmd_callback      (GtkWidget *, gpointer);
void select_none_cmd_callback     (GtkWidget *, gpointer);
void select_float_cmd_callback    (GtkWidget *, gpointer);
void select_feather_cmd_callback  (GtkWidget *, gpointer);
void select_sharpen_cmd_callback  (GtkWidget *, gpointer);
void select_shrink_cmd_callback   (GtkWidget *, gpointer);
void select_border_cmd_callback   (GtkWidget *, gpointer);
void select_grow_cmd_callback     (GtkWidget *, gpointer);
void select_save_cmd_callback     (GtkWidget *, gpointer);

void view_zoomin_cmd_callback            (GtkWidget *, gpointer);
void view_zoomout_cmd_callback           (GtkWidget *, gpointer);
void view_zoom_16_1_cmd_callback         (GtkWidget *, gpointer);
void view_zoom_8_1_cmd_callback          (GtkWidget *, gpointer);
void view_zoom_4_1_cmd_callback          (GtkWidget *, gpointer);
void view_zoom_2_1_cmd_callback          (GtkWidget *, gpointer);
void view_zoom_1_1_cmd_callback          (GtkWidget *, gpointer);
void view_zoom_1_2_cmd_callback          (GtkWidget *, gpointer);
void view_zoom_1_4_cmd_callback          (GtkWidget *, gpointer);
void view_zoom_1_8_cmd_callback          (GtkWidget *, gpointer);
void view_zoom_1_16_cmd_callback         (GtkWidget *, gpointer);
void view_dot_for_dot_cmd_callback       (GtkWidget *, gpointer);
void view_info_window_cmd_callback       (GtkWidget *, gpointer);
void view_nav_window_cmd_callback        (GtkWidget *, gpointer);
void view_toggle_selection_cmd_callback  (GtkWidget *, gpointer);
void view_toggle_rulers_cmd_callback     (GtkWidget *, gpointer);
void view_toggle_statusbar_cmd_callback  (GtkWidget *, gpointer);
void view_toggle_guides_cmd_callback     (GtkWidget *, gpointer);
void view_snap_to_guides_cmd_callback    (GtkWidget *, gpointer);
void view_new_view_cmd_callback          (GtkWidget *, gpointer);
void view_shrink_wrap_cmd_callback       (GtkWidget *, gpointer);

void image_convert_rgb_cmd_callback       (GtkWidget *, gpointer);
void image_convert_grayscale_cmd_callback (GtkWidget *, gpointer);
void image_convert_indexed_cmd_callback   (GtkWidget *, gpointer);
void image_desaturate_cmd_callback        (GtkWidget *, gpointer);
void image_invert_cmd_callback            (GtkWidget *, gpointer);
void image_equalize_cmd_callback          (GtkWidget *, gpointer);
void image_offset_cmd_callback            (GtkWidget *, gpointer);
void image_resize_cmd_callback            (GtkWidget *, gpointer);
void image_scale_cmd_callback             (GtkWidget *, gpointer);
void image_duplicate_cmd_callback         (GtkWidget *, gpointer);

void layers_previous_cmd_callback          (GtkWidget *, gpointer);
void layers_next_cmd_callback              (GtkWidget *, gpointer);
void layers_raise_cmd_callback             (GtkWidget *, gpointer);
void layers_lower_cmd_callback             (GtkWidget *, gpointer);
void layers_raise_to_top_cmd_callback      (GtkWidget *, gpointer);
void layers_lower_to_bottom_cmd_callback   (GtkWidget *, gpointer);
void layers_anchor_cmd_callback            (GtkWidget *, gpointer);
void layers_merge_cmd_callback             (GtkWidget *, gpointer);
void layers_flatten_cmd_callback           (GtkWidget *, gpointer);
void layers_mask_select_cmd_callback       (GtkWidget *, gpointer);
void layers_add_alpha_channel_cmd_callback (GtkWidget *, gpointer);
void layers_alpha_select_cmd_callback      (GtkWidget *, gpointer);
void layers_resize_to_image_cmd_callback   (GtkWidget *, gpointer);

void tools_default_colors_cmd_callback (GtkWidget *, gpointer);
void tools_swap_colors_cmd_callback    (GtkWidget *, gpointer);
void tools_select_cmd_callback         (GtkWidget *widget,
					gpointer   callback_data,
					guint      callback_action);

void filters_repeat_cmd_callback (GtkWidget *widget,
				  gpointer   callback_data,
				  guint      callback_action);

void dialogs_lc_cmd_callback              (GtkWidget *, gpointer);
void dialogs_tool_options_cmd_callback    (GtkWidget *, gpointer);
void dialogs_brushes_cmd_callback         (GtkWidget *, gpointer);
void dialogs_patterns_cmd_callback        (GtkWidget *, gpointer);
void dialogs_gradients_cmd_callback       (GtkWidget *, gpointer);
void dialogs_palette_cmd_callback         (GtkWidget *, gpointer);
void dialogs_indexed_palette_cmd_callback (GtkWidget *, gpointer);
void dialogs_input_devices_cmd_callback   (GtkWidget *, gpointer);
void dialogs_device_status_cmd_callback   (GtkWidget *, gpointer);
void dialogs_error_console_cmd_callback   (GtkWidget *, gpointer);
void dialogs_display_filters_cmd_callback (GtkWidget *, gpointer);
void dialogs_undo_history_cmd_callback    (GtkWidget *, gpointer);

void dialogs_module_browser_cmd_callback  (GtkWidget *, gpointer);

void help_help_cmd_callback         (GtkWidget *, gpointer);
void help_context_help_cmd_callback (GtkWidget *, gpointer);
void help_tips_cmd_callback         (GtkWidget *, gpointer);
void help_about_cmd_callback        (GtkWidget *, gpointer);

#endif /* __COMMANDS_H__ */
