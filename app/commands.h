#ifndef __COMMANDS_H__
#define __COMMANDS_H__


#include "gtk/gtk.h"

void file_new_cmd_callback (GtkWidget           *widget,
			    gpointer             callback_data,
			    guint                callback_action);
void file_open_cmd_callback (GtkWidget *, gpointer);
void file_save_cmd_callback (GtkWidget *, gpointer);
void file_save_as_cmd_callback (GtkWidget *, gpointer);
void file_pref_cmd_callback (GtkWidget *, gpointer);
void file_close_cmd_callback (GtkWidget *, gpointer);
void file_quit_cmd_callback (GtkWidget *, gpointer);
void edit_cut_cmd_callback (GtkWidget *, gpointer);
void edit_copy_cmd_callback (GtkWidget *, gpointer);
void edit_paste_cmd_callback (GtkWidget *, gpointer);
void edit_paste_into_cmd_callback (GtkWidget *, gpointer);
void edit_clear_cmd_callback (GtkWidget *, gpointer);
void edit_fill_cmd_callback (GtkWidget *, gpointer);
void edit_stroke_cmd_callback (GtkWidget *, gpointer);
void edit_undo_cmd_callback (GtkWidget *, gpointer);
void edit_redo_cmd_callback (GtkWidget *, gpointer);
void edit_named_cut_cmd_callback (GtkWidget *, gpointer);
void edit_named_copy_cmd_callback (GtkWidget *, gpointer);
void edit_named_paste_cmd_callback (GtkWidget *, gpointer);
void select_toggle_cmd_callback (GtkWidget *, gpointer);
void select_invert_cmd_callback (GtkWidget *, gpointer);
void select_all_cmd_callback (GtkWidget *, gpointer);
void select_none_cmd_callback (GtkWidget *, gpointer);
void select_float_cmd_callback (GtkWidget *, gpointer);
void select_sharpen_cmd_callback (GtkWidget *, gpointer);
void select_border_cmd_callback (GtkWidget *, gpointer);
void select_feather_cmd_callback (GtkWidget *, gpointer);
void select_grow_cmd_callback (GtkWidget *, gpointer);
void select_shrink_cmd_callback (GtkWidget *, gpointer);
void select_by_color_cmd_callback (GtkWidget *, gpointer);
void select_save_cmd_callback (GtkWidget *, gpointer);
void view_zoomin_cmd_callback (GtkWidget *, gpointer);
void view_zoomout_cmd_callback (GtkWidget *, gpointer);
void view_zoom_16_1_callback (GtkWidget *, gpointer);
void view_zoom_8_1_callback (GtkWidget *, gpointer);
void view_zoom_4_1_callback (GtkWidget *, gpointer);
void view_zoom_2_1_callback (GtkWidget *, gpointer);
void view_zoom_1_1_callback (GtkWidget *, gpointer);
void view_zoom_1_2_callback (GtkWidget *, gpointer);
void view_zoom_1_4_callback (GtkWidget *, gpointer);
void view_zoom_1_8_callback (GtkWidget *, gpointer);
void view_zoom_1_16_callback (GtkWidget *, gpointer);
void view_window_info_cmd_callback (GtkWidget *, gpointer);
void view_toggle_rulers_cmd_callback (GtkWidget *, gpointer);
void view_toggle_guides_cmd_callback (GtkWidget *, gpointer);
void view_snap_to_guides_cmd_callback (GtkWidget *, gpointer);
void view_new_view_cmd_callback (GtkWidget *, gpointer);
void view_shrink_wrap_cmd_callback (GtkWidget *, gpointer);
void image_equalize_cmd_callback (GtkWidget *, gpointer);
void image_invert_cmd_callback (GtkWidget *, gpointer);
void image_desaturate_cmd_callback (GtkWidget *, gpointer);
void image_convert_rgb_cmd_callback (GtkWidget *, gpointer);
void image_convert_grayscale_cmd_callback (GtkWidget *, gpointer);
void image_convert_indexed_cmd_callback (GtkWidget *, gpointer);
void image_resize_cmd_callback (GtkWidget *, gpointer);
void image_scale_cmd_callback (GtkWidget *, gpointer);
void channel_ops_duplicate_cmd_callback (GtkWidget *, gpointer);
void channel_ops_offset_cmd_callback (GtkWidget *, gpointer);
void layers_raise_cmd_callback (GtkWidget *, gpointer);
void layers_lower_cmd_callback (GtkWidget *, gpointer);
void layers_anchor_cmd_callback (GtkWidget *, gpointer);
void layers_merge_cmd_callback (GtkWidget *, gpointer);
void layers_flatten_cmd_callback (GtkWidget *, gpointer);
void layers_alpha_select_cmd_callback (GtkWidget *, gpointer);
void layers_mask_select_cmd_callback (GtkWidget *, gpointer);
void layers_add_alpha_channel_cmd_callback (GtkWidget *, gpointer);
void tools_default_colors_cmd_callback (GtkWidget *, gpointer);
void tools_swap_colors_cmd_callback (GtkWidget *, gpointer);
void tools_select_cmd_callback (GtkWidget           *widget,
				gpointer             callback_data,
				guint                callback_action);
void filters_repeat_cmd_callback (GtkWidget           *widget,
				  gpointer             callback_data,
				  guint                callback_action);
void dialogs_brushes_cmd_callback (GtkWidget *, gpointer);
void dialogs_patterns_cmd_callback (GtkWidget *, gpointer);
void dialogs_palette_cmd_callback (GtkWidget *, gpointer);
void dialogs_gradient_editor_cmd_callback (GtkWidget *, gpointer);
void dialogs_lc_cmd_callback (GtkWidget *, gpointer);
void dialogs_indexed_palette_cmd_callback (GtkWidget *, gpointer);
void dialogs_tools_options_cmd_callback (GtkWidget *, gpointer);
void dialogs_input_devices_cmd_callback (GtkWidget *, gpointer);
void dialogs_device_status_cmd_callback (GtkWidget *, gpointer);
void dialogs_error_console_cmd_callback (GtkWidget *, gpointer);
void about_dialog_cmd_callback (GtkWidget *, gpointer);
void tips_dialog_cmd_callback (GtkWidget *, gpointer);

#endif /* __COMMANDS_H__ */
