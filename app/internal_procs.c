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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "appenv.h"
#include "app_procs.h"
#include "gimage_cmds.h"
#include "gimage_mask_cmds.h"
#include "layer_cmds.h"
#include "internal_procs.h"
#include "procedural_db.h"

#include "libgimp/gimpintl.h"

void register_gdisplay_procs        (void);
void register_edit_procs            (void);
void register_floating_sel_procs    (void);
void register_undo_procs            (void);
void register_convert_procs         (void);
void register_paths_procs           (void);
void register_palette_procs         (void);
void register_unit_procs            (void);
void register_text_tool_procs       (void);
void register_color_procs           (void);
void register_misc_procs            (void);
void register_tools_procs           (void);
void register_gimprc_procs          (void);
void register_channel_procs         (void);
void register_channel_ops_procs     (void);
void register_gradient_procs        (void);
void register_gradient_select_procs (void);
void register_brushes_procs         (void);
void register_brush_select_procs    (void);
void register_patterns_procs        (void);
void register_pattern_select_procs  (void);
void register_parasite_procs        (void);
void register_drawable_procs        (void);
void register_procedural_db_procs   (void);

void
internal_procs_init (void)
{
  gfloat pcount = 0;
  /* grep -c procedural_db_register internal_procs.c */
  gfloat total_pcount = 264;

  app_init_update_status(_("Internal Procedures"), _("Tool procedures"),
			 pcount/total_pcount);

  /*  Tool procedures  */
  register_tools_procs ();
  pcount += 21;

  register_text_tool_procs ();
  pcount += 6;

  app_init_update_status(NULL, _("GDisplay procedures"),
			 pcount/total_pcount);

  /*  GDisplay procedures  */
  register_gdisplay_procs ();
  pcount += 3;

  app_init_update_status(NULL, _("Edit procedures"),
			 pcount/total_pcount);

  /*  Edit procedures  */
  register_edit_procs ();
  pcount += 6;

  app_init_update_status(NULL, _("GImage procedures"),
			 pcount/total_pcount);

  /*  GImage procedures  */
  procedural_db_register (&gimage_list_images_proc); pcount++;
  procedural_db_register (&gimage_new_proc); pcount++;
  procedural_db_register (&gimage_resize_proc); pcount++;
  procedural_db_register (&gimage_scale_proc); pcount++;
  procedural_db_register (&gimage_delete_proc); pcount++;
  procedural_db_register (&gimage_free_shadow_proc); pcount++;
  procedural_db_register (&gimage_get_layers_proc); pcount++;
  procedural_db_register (&gimage_get_channels_proc); pcount++;
  procedural_db_register (&gimage_get_active_layer_proc); pcount++;
  procedural_db_register (&gimage_get_active_channel_proc); pcount++;
  procedural_db_register (&gimage_get_selection_proc); pcount++;
  procedural_db_register (&gimage_get_component_active_proc); pcount++;
  procedural_db_register (&gimage_get_component_visible_proc); pcount++;
  procedural_db_register (&gimage_set_active_layer_proc); pcount++;
  procedural_db_register (&gimage_set_active_channel_proc); pcount++;
  procedural_db_register (&gimage_unset_active_channel_proc); pcount++;
  procedural_db_register (&gimage_set_component_active_proc); pcount++;
  procedural_db_register (&gimage_set_component_visible_proc); pcount++;
  procedural_db_register (&gimage_pick_correlate_layer_proc); pcount++;
  procedural_db_register (&gimage_raise_layer_proc); pcount++;
  procedural_db_register (&gimage_lower_layer_proc); pcount++;
  procedural_db_register (&gimage_raise_layer_to_top_proc); pcount++;
  procedural_db_register (&gimage_lower_layer_to_bottom_proc); pcount++;
  procedural_db_register (&gimage_merge_visible_layers_proc); pcount++;
  procedural_db_register (&gimage_merge_down_proc); pcount++;
  procedural_db_register (&gimage_flatten_proc); pcount++;
  procedural_db_register (&gimage_add_layer_proc); pcount++;
  procedural_db_register (&gimage_remove_layer_proc); pcount++;
  procedural_db_register (&gimage_add_layer_mask_proc); pcount++;
  procedural_db_register (&gimage_remove_layer_mask_proc); pcount++;
  procedural_db_register (&gimage_raise_channel_proc); pcount++;
  procedural_db_register (&gimage_lower_channel_proc); pcount++;
  procedural_db_register (&gimage_add_channel_proc); pcount++;
  procedural_db_register (&gimage_remove_channel_proc); pcount++;
  procedural_db_register (&gimage_active_drawable_proc); pcount++;
  procedural_db_register (&gimage_base_type_proc); pcount++;
  procedural_db_register (&gimage_get_filename_proc); pcount++;
  procedural_db_register (&gimage_set_filename_proc); pcount++;
  procedural_db_register (&gimage_get_resolution_proc); pcount++;
  procedural_db_register (&gimage_set_resolution_proc); pcount++;
  procedural_db_register (&gimage_get_unit_proc); pcount++;
  procedural_db_register (&gimage_set_unit_proc); pcount++;
  procedural_db_register (&gimage_width_proc); pcount++;
  procedural_db_register (&gimage_height_proc); pcount++;
  procedural_db_register (&gimage_get_cmap_proc); pcount++;
  procedural_db_register (&gimage_set_cmap_proc); pcount++;
  procedural_db_register (&gimage_enable_undo_proc); pcount++;
  procedural_db_register (&gimage_disable_undo_proc); pcount++;
  procedural_db_register (&gimage_clean_all_proc); pcount++;
  procedural_db_register (&gimage_floating_sel_proc); pcount++;
  procedural_db_register (&gimage_floating_sel_attached_to_proc); pcount++;
  procedural_db_register (&gimp_image_add_hguide_proc); pcount++;
  procedural_db_register (&gimp_image_add_vguide_proc); pcount++;
  procedural_db_register (&gimp_image_delete_guide_proc); pcount++;
  procedural_db_register (&gimp_image_findnext_guide_proc); pcount++;
  procedural_db_register (&gimp_image_get_guide_orientation_proc); pcount++;
  procedural_db_register (&gimp_image_get_guide_position_proc); pcount++;
  procedural_db_register (&gimp_image_find_parasite_proc); pcount++;
  procedural_db_register (&gimp_image_parasite_list_proc); pcount++;
  procedural_db_register (&gimp_image_attach_parasite_proc); pcount++;
  procedural_db_register (&gimp_image_detach_parasite_proc); pcount++;
  procedural_db_register (&gimp_image_get_layer_by_tattoo_proc); pcount++;
  procedural_db_register (&gimp_image_get_channel_by_tattoo_proc); pcount++;

  app_init_update_status(NULL, _("GImage mask procedures"),
			 pcount/total_pcount);

  /*  GImage mask procedures  */
  procedural_db_register (&gimage_mask_bounds_proc); pcount++;
  procedural_db_register (&gimage_mask_value_proc); pcount++;
  procedural_db_register (&gimage_mask_is_empty_proc); pcount++;
  procedural_db_register (&gimage_mask_translate_proc); pcount++;
  procedural_db_register (&gimage_mask_float_proc); pcount++;
  procedural_db_register (&gimage_mask_clear_proc); pcount++;
  procedural_db_register (&gimage_mask_invert_proc); pcount++;
  procedural_db_register (&gimage_mask_sharpen_proc); pcount++;
  procedural_db_register (&gimage_mask_all_proc); pcount++;
  procedural_db_register (&gimage_mask_none_proc); pcount++;
  procedural_db_register (&gimage_mask_feather_proc); pcount++;
  procedural_db_register (&gimage_mask_border_proc); pcount++;
  procedural_db_register (&gimage_mask_grow_proc); pcount++;
  procedural_db_register (&gimage_mask_shrink_proc); pcount++;
  procedural_db_register (&gimage_mask_layer_alpha_proc); pcount++;
  procedural_db_register (&gimage_mask_load_proc); pcount++;
  procedural_db_register (&gimage_mask_save_proc); pcount++;

  app_init_update_status(NULL, _("Layer procedures"),
			 pcount/total_pcount);

  /*  Layer procedures  */
  procedural_db_register (&layer_new_proc); pcount++;
  procedural_db_register (&layer_copy_proc); pcount++;
  procedural_db_register (&layer_create_mask_proc); pcount++;
  procedural_db_register (&layer_scale_proc); pcount++;
  procedural_db_register (&layer_resize_proc); pcount++;
  procedural_db_register (&layer_delete_proc); pcount++;
  procedural_db_register (&layer_translate_proc); pcount++;
  procedural_db_register (&layer_add_alpha_proc); pcount++;
  procedural_db_register (&layer_get_name_proc); pcount++;
  procedural_db_register (&layer_set_name_proc); pcount++;
  procedural_db_register (&layer_get_visible_proc); pcount++;
  procedural_db_register (&layer_set_visible_proc); pcount++;
  procedural_db_register (&layer_get_preserve_trans_proc); pcount++;
  procedural_db_register (&layer_set_preserve_trans_proc); pcount++;
  procedural_db_register (&layer_get_apply_mask_proc); pcount++;
  procedural_db_register (&layer_set_apply_mask_proc); pcount++;
  procedural_db_register (&layer_get_show_mask_proc); pcount++;
  procedural_db_register (&layer_set_show_mask_proc); pcount++;
  procedural_db_register (&layer_get_edit_mask_proc); pcount++;
  procedural_db_register (&layer_set_edit_mask_proc); pcount++;
  procedural_db_register (&layer_get_opacity_proc); pcount++;
  procedural_db_register (&layer_set_opacity_proc); pcount++;
  procedural_db_register (&layer_get_mode_proc); pcount++;
  procedural_db_register (&layer_set_mode_proc); pcount++;
  procedural_db_register (&layer_set_offsets_proc); pcount++;
  procedural_db_register (&layer_mask_proc); pcount++;
  procedural_db_register (&layer_is_floating_sel_proc); pcount++;
  procedural_db_register (&layer_get_tattoo_proc); pcount++;
  procedural_db_register (&layer_get_linked_proc); pcount++;
  procedural_db_register (&layer_set_linked_proc); pcount++;

  app_init_update_status(NULL, _("Channel procedures"),
			 pcount/total_pcount);

  /*  Channel procedures  */
  register_channel_procs ();
  pcount += 14;

  app_init_update_status(NULL, _("Drawable procedures"),
			 pcount/total_pcount);

  /*  Drawable procedures  */
  register_drawable_procs ();
  pcount += 25;

  app_init_update_status(NULL, _("Floating selections"),
			 pcount/total_pcount);

  /*  Floating Selections  */
  register_floating_sel_procs ();
  pcount += 6;

  app_init_update_status(NULL, _("Undo"),
			 pcount/total_pcount);

  /*  Undo  */
  register_undo_procs ();
  pcount += 2;

  app_init_update_status(NULL, _("Palette"),
			 pcount/total_pcount);

  /*  Palette  */
  register_palette_procs ();
  pcount += 7;

  app_init_update_status(NULL, _("Interface procedures"),
			 pcount/total_pcount);

  /*  Interface procs  */
  register_brushes_procs ();
  register_brush_select_procs ();
  register_patterns_procs ();
  register_pattern_select_procs ();
  pcount += 20;


  register_gradient_procs ();
  register_gradient_select_procs ();
  pcount += 9;

  app_init_update_status(NULL, _("Image procedures"),
			 pcount/total_pcount);

  register_color_procs ();
  pcount += 12;

  register_convert_procs ();
  pcount += 4;

  app_init_update_status(NULL, _("Channel ops"),
			 pcount/total_pcount);

  /*  Channel Ops procedures  */
  register_channel_ops_procs ();
  pcount += 2;

  app_init_update_status(NULL, _("gimprc ops"),
			 pcount/total_pcount);
  /*  Gimprc procedures  */
  register_gimprc_procs ();
  pcount += 2;

  app_init_update_status(NULL, _("parasites"),
			 pcount/total_pcount);
  /*  parasite procedures  */
  register_parasite_procs ();
  pcount += 5;

  /* paths procedures */
  register_paths_procs ();
  pcount += 6;

  app_init_update_status(NULL, _("Procedural database"),
			 pcount/total_pcount);

  /*  Unit Procedures  */
  register_unit_procs ();
  pcount += 11;

  /*  Procedural Database  */
  register_procedural_db_procs ();
  pcount += 8;

  register_misc_procs ();
}
