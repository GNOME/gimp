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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "appenv.h"
#include "airbrush.h"
#include "blend.h"
#include "bucket_fill.h"
#include "brightness_contrast.h"
#include "brushes.h"
#include "by_color_select.h"
#include "channel_cmds.h"
#include "channel_ops.h"
#include "clone.h"
#include "color_balance.h"
#include "color_picker.h"
#include "convert.h"
#include "convolve.h"
#include "crop.h"
#include "curves.h"
#include "desaturate.h"
#include "drawable_cmds.h"
#include "edit_cmds.h"
#include "ellipse_select.h"
#include "equalize.h"
#include "eraser.h"
#include "flip_tool.h"
#include "floating_sel_cmds.h"
#include "free_select.h"
#include "fuzzy_select.h"
#include "gdisplay_cmds.h"
#include "gimage_cmds.h"
#include "gimage_mask_cmds.h"
#include "gimprc.h"
#include "gradient.h"
#include "histogram_tool.h"
#include "hue_saturation.h"
#include "invert.h"
#include "layer_cmds.h"
#include "levels.h"
#include "internal_procs.h"
#include "paintbrush.h"
#include "palette.h"
#include "patterns.h"
#include "pencil.h"
#include "perspective_tool.h"
#include "posterize.h"
#include "rect_select.h"
#include "rotate_tool.h"
#include "scale_tool.h"
#include "shear_tool.h"
#include "text_tool.h"
#include "threshold.h"
#include "undo_cmds.h"
#include "procedural_db.h"


void
internal_procs_init ()
{
  /*  Tool procedures  */
  procedural_db_register (&airbrush_proc);
  procedural_db_register (&blend_proc);
  procedural_db_register (&bucket_fill_proc);
  procedural_db_register (&by_color_select_proc);
  procedural_db_register (&clone_proc);
  procedural_db_register (&color_picker_proc);
  procedural_db_register (&convolve_proc);
  procedural_db_register (&crop_proc);
  procedural_db_register (&ellipse_select_proc);
  procedural_db_register (&eraser_proc);
  procedural_db_register (&flip_proc);
  procedural_db_register (&free_select_proc);
  procedural_db_register (&fuzzy_select_proc);
  procedural_db_register (&paintbrush_proc);
  procedural_db_register (&pencil_proc);
  procedural_db_register (&perspective_proc);
  procedural_db_register (&rect_select_proc);
  procedural_db_register (&rotate_proc);
  procedural_db_register (&scale_proc);
  procedural_db_register (&shear_proc);
  procedural_db_register (&text_tool_proc);
  procedural_db_register (&text_tool_get_extents_proc);

  /*  GDisplay procedures  */
  procedural_db_register (&gdisplay_new_proc);
  procedural_db_register (&gdisplay_delete_proc);
  procedural_db_register (&gdisplays_flush_proc);

  /*  Edit procedures  */
  procedural_db_register (&edit_cut_proc);
  procedural_db_register (&edit_copy_proc);
  procedural_db_register (&edit_paste_proc);
  procedural_db_register (&edit_clear_proc);
  procedural_db_register (&edit_fill_proc);
  procedural_db_register (&edit_stroke_proc);

  /*  GImage procedures  */
  procedural_db_register (&gimage_list_images_proc);
  procedural_db_register (&gimage_new_proc);
  procedural_db_register (&gimage_resize_proc);
  procedural_db_register (&gimage_scale_proc);
  procedural_db_register (&gimage_delete_proc);
  procedural_db_register (&gimage_free_shadow_proc);
  procedural_db_register (&gimage_get_layers_proc);
  procedural_db_register (&gimage_get_channels_proc);
  procedural_db_register (&gimage_get_active_layer_proc);
  procedural_db_register (&gimage_get_active_channel_proc);
  procedural_db_register (&gimage_get_selection_proc);
  procedural_db_register (&gimage_get_component_active_proc);
  procedural_db_register (&gimage_get_component_visible_proc);
  procedural_db_register (&gimage_set_active_layer_proc);
  procedural_db_register (&gimage_set_active_channel_proc);
  procedural_db_register (&gimage_unset_active_channel_proc);
  procedural_db_register (&gimage_set_component_active_proc);
  procedural_db_register (&gimage_set_component_visible_proc);
  procedural_db_register (&gimage_pick_correlate_layer_proc);
  procedural_db_register (&gimage_raise_layer_proc);
  procedural_db_register (&gimage_lower_layer_proc);
  procedural_db_register (&gimage_merge_visible_layers_proc);
  procedural_db_register (&gimage_flatten_proc);
  procedural_db_register (&gimage_add_layer_proc);
  procedural_db_register (&gimage_remove_layer_proc);
  procedural_db_register (&gimage_add_layer_mask_proc);
  procedural_db_register (&gimage_remove_layer_mask_proc);
  procedural_db_register (&gimage_raise_channel_proc);
  procedural_db_register (&gimage_lower_channel_proc);
  procedural_db_register (&gimage_add_channel_proc);
  procedural_db_register (&gimage_remove_channel_proc);
  procedural_db_register (&gimage_active_drawable_proc);
  procedural_db_register (&gimage_base_type_proc);
  procedural_db_register (&gimage_get_filename_proc);
  procedural_db_register (&gimage_set_filename_proc);
  procedural_db_register (&gimage_width_proc);
  procedural_db_register (&gimage_height_proc);
  procedural_db_register (&gimage_get_cmap_proc);
  procedural_db_register (&gimage_set_cmap_proc);
  procedural_db_register (&gimage_enable_undo_proc);
  procedural_db_register (&gimage_disable_undo_proc);
  procedural_db_register (&gimage_clean_all_proc);
  procedural_db_register (&gimage_floating_sel_proc);

  /*  GImage mask procedures  */
  procedural_db_register (&gimage_mask_bounds_proc);
  procedural_db_register (&gimage_mask_value_proc);
  procedural_db_register (&gimage_mask_is_empty_proc);
  procedural_db_register (&gimage_mask_translate_proc);
  procedural_db_register (&gimage_mask_float_proc);
  procedural_db_register (&gimage_mask_clear_proc);
  procedural_db_register (&gimage_mask_invert_proc);
  procedural_db_register (&gimage_mask_sharpen_proc);
  procedural_db_register (&gimage_mask_all_proc);
  procedural_db_register (&gimage_mask_none_proc);
  procedural_db_register (&gimage_mask_feather_proc);
  procedural_db_register (&gimage_mask_border_proc);
  procedural_db_register (&gimage_mask_grow_proc);
  procedural_db_register (&gimage_mask_shrink_proc);
  procedural_db_register (&gimage_mask_layer_alpha_proc);
  procedural_db_register (&gimage_mask_load_proc);
  procedural_db_register (&gimage_mask_save_proc);

  /*  Layer procedures  */
  procedural_db_register (&layer_new_proc);
  procedural_db_register (&layer_copy_proc);
  procedural_db_register (&layer_create_mask_proc);
  procedural_db_register (&layer_scale_proc);
  procedural_db_register (&layer_resize_proc);
  procedural_db_register (&layer_delete_proc);
  procedural_db_register (&layer_translate_proc);
  procedural_db_register (&layer_add_alpha_proc);
  procedural_db_register (&layer_get_name_proc);
  procedural_db_register (&layer_set_name_proc);
  procedural_db_register (&layer_get_visible_proc);
  procedural_db_register (&layer_set_visible_proc);
  procedural_db_register (&layer_get_preserve_trans_proc);
  procedural_db_register (&layer_set_preserve_trans_proc);
  procedural_db_register (&layer_get_apply_mask_proc);
  procedural_db_register (&layer_set_apply_mask_proc);
  procedural_db_register (&layer_get_show_mask_proc);
  procedural_db_register (&layer_set_show_mask_proc);
  procedural_db_register (&layer_get_edit_mask_proc);
  procedural_db_register (&layer_set_edit_mask_proc);
  procedural_db_register (&layer_get_opacity_proc);
  procedural_db_register (&layer_set_opacity_proc);
  procedural_db_register (&layer_get_mode_proc);
  procedural_db_register (&layer_set_mode_proc);
  procedural_db_register (&layer_set_offsets_proc);
  procedural_db_register (&layer_mask_proc);
  procedural_db_register (&layer_is_floating_sel_proc);

  /*  Channel procedures  */
  procedural_db_register (&channel_new_proc);
  procedural_db_register (&channel_copy_proc);
  procedural_db_register (&channel_delete_proc);
  procedural_db_register (&channel_get_name_proc);
  procedural_db_register (&channel_set_name_proc);
  procedural_db_register (&channel_get_visible_proc);
  procedural_db_register (&channel_set_visible_proc);
  procedural_db_register (&channel_get_show_masked_proc);
  procedural_db_register (&channel_set_show_masked_proc);
  procedural_db_register (&channel_get_opacity_proc);
  procedural_db_register (&channel_set_opacity_proc);
  procedural_db_register (&channel_get_color_proc);
  procedural_db_register (&channel_set_color_proc);

  /*  Drawable procedures  */
  procedural_db_register (&drawable_merge_shadow_proc);
  procedural_db_register (&drawable_fill_proc);
  procedural_db_register (&drawable_update_proc);
  procedural_db_register (&drawable_mask_bounds_proc);
  procedural_db_register (&drawable_gimage_proc);
  procedural_db_register (&drawable_type_proc);
  procedural_db_register (&drawable_has_alpha_proc);
  procedural_db_register (&drawable_type_with_alpha_proc);
  procedural_db_register (&drawable_color_proc);
  procedural_db_register (&drawable_gray_proc);
  procedural_db_register (&drawable_indexed_proc);
  procedural_db_register (&drawable_bytes_proc);
  procedural_db_register (&drawable_width_proc);
  procedural_db_register (&drawable_height_proc);
  procedural_db_register (&drawable_offsets_proc);
  procedural_db_register (&drawable_layer_proc);
  procedural_db_register (&drawable_layer_mask_proc);
  procedural_db_register (&drawable_channel_proc);
  procedural_db_register (&drawable_set_pixel_proc);
  procedural_db_register (&drawable_get_pixel_proc);

  /*  Floating Selections  */
  procedural_db_register (&floating_sel_remove_proc);
  procedural_db_register (&floating_sel_anchor_proc);
  procedural_db_register (&floating_sel_to_layer_proc);

  /*  Undo  */
  procedural_db_register (&undo_push_group_start_proc);
  procedural_db_register (&undo_push_group_end_proc);

  /*  Palette  */
  procedural_db_register (&palette_get_foreground_proc);
  procedural_db_register (&palette_get_background_proc);
  procedural_db_register (&palette_set_foreground_proc);
  procedural_db_register (&palette_set_background_proc);

  /*  Interface procs  */
  procedural_db_register (&brushes_get_brush_proc);
  procedural_db_register (&brushes_refresh_brush_proc);
  procedural_db_register (&brushes_set_brush_proc);
  procedural_db_register (&brushes_get_opacity_proc);
  procedural_db_register (&brushes_set_opacity_proc);
  procedural_db_register (&brushes_get_spacing_proc);
  procedural_db_register (&brushes_set_spacing_proc);
  procedural_db_register (&brushes_get_paint_mode_proc);
  procedural_db_register (&brushes_set_paint_mode_proc);
  procedural_db_register (&brushes_list_proc);
  procedural_db_register (&patterns_get_pattern_proc);
  procedural_db_register (&patterns_set_pattern_proc);
  procedural_db_register (&patterns_list_proc);

  procedural_db_register (&gradients_get_list_proc);
  procedural_db_register (&gradients_get_active_proc);
  procedural_db_register (&gradients_set_active_proc);
  procedural_db_register (&gradients_sample_uniform_proc);
  procedural_db_register (&gradients_sample_custom_proc);

  /*  Image procedures  */
  procedural_db_register (&desaturate_proc);
  procedural_db_register (&equalize_proc);
  procedural_db_register (&invert_proc);

  procedural_db_register (&brightness_contrast_proc);
  procedural_db_register (&curves_spline_proc);
  procedural_db_register (&curves_explicit_proc);
  procedural_db_register (&color_balance_proc);
  procedural_db_register (&histogram_proc);
  procedural_db_register (&hue_saturation_proc);
  procedural_db_register (&levels_proc);
  procedural_db_register (&posterize_proc);
  procedural_db_register (&threshold_proc);

  procedural_db_register (&convert_rgb_proc);
  procedural_db_register (&convert_grayscale_proc);
  procedural_db_register (&convert_indexed_proc);

  /*  Channel Ops procedures  */
  procedural_db_register (&channel_ops_duplicate_proc);
  procedural_db_register (&channel_ops_offset_proc);

  /*  Gimprc procedures  */
  procedural_db_register (&gimprc_query_proc);

  /*  Procedural Database  */
  procedural_db_register (&procedural_db_dump_proc);
  procedural_db_register (&procedural_db_query_proc);
  procedural_db_register (&procedural_db_proc_info_proc);
  procedural_db_register (&procedural_db_proc_arg_proc);
  procedural_db_register (&procedural_db_proc_val_proc);
  procedural_db_register (&procedural_db_get_data_proc);
  procedural_db_register (&procedural_db_set_data_proc);
}
