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
#ifndef  __GIMAGE_CMDS_H__
#define  __GIMAGE_CMDS_H__

#include "procedural_db.h"

void  channel_ops_duplicate (void *);

extern ProcRecord gimage_list_images_proc;
extern ProcRecord gimage_new_proc;
extern ProcRecord gimage_resize_proc;
extern ProcRecord gimage_scale_proc;
extern ProcRecord gimage_delete_proc;
extern ProcRecord gimage_free_shadow_proc;
extern ProcRecord gimage_get_layers_proc;
extern ProcRecord gimage_get_channels_proc;
extern ProcRecord gimage_get_active_layer_proc;
extern ProcRecord gimage_get_active_channel_proc;
extern ProcRecord gimage_get_selection_proc;
extern ProcRecord gimage_get_component_active_proc;
extern ProcRecord gimage_get_component_visible_proc;
extern ProcRecord gimage_set_active_layer_proc;
extern ProcRecord gimage_set_active_channel_proc;
extern ProcRecord gimage_unset_active_channel_proc;
extern ProcRecord gimage_set_component_active_proc;
extern ProcRecord gimage_set_component_visible_proc;
extern ProcRecord gimage_pick_correlate_layer_proc;
extern ProcRecord gimage_raise_layer_proc;
extern ProcRecord gimage_lower_layer_proc;
extern ProcRecord gimage_raise_layer_to_top_proc;
extern ProcRecord gimage_lower_layer_to_bottom_proc;
extern ProcRecord gimage_merge_visible_layers_proc;
extern ProcRecord gimage_merge_down_proc;
extern ProcRecord gimage_flatten_proc;
extern ProcRecord gimage_add_layer_proc;
extern ProcRecord gimage_remove_layer_proc;
extern ProcRecord gimage_add_layer_mask_proc;
extern ProcRecord gimage_remove_layer_mask_proc;
extern ProcRecord gimage_raise_channel_proc;
extern ProcRecord gimage_lower_channel_proc;
extern ProcRecord gimage_add_channel_proc;
extern ProcRecord gimage_remove_channel_proc;
extern ProcRecord gimage_active_drawable_proc;
extern ProcRecord gimage_base_type_proc;
extern ProcRecord gimage_get_filename_proc;
extern ProcRecord gimage_set_filename_proc;
extern ProcRecord gimage_get_resolution_proc;
extern ProcRecord gimage_set_resolution_proc;
extern ProcRecord gimage_get_unit_proc;
extern ProcRecord gimage_set_unit_proc;
extern ProcRecord gimage_width_proc;
extern ProcRecord gimage_height_proc;
extern ProcRecord gimage_get_cmap_proc;
extern ProcRecord gimage_set_cmap_proc;
extern ProcRecord gimage_enable_undo_proc;
extern ProcRecord gimage_disable_undo_proc;
extern ProcRecord gimage_clean_all_proc;
extern ProcRecord gimage_floating_sel_proc;
extern ProcRecord gimage_floating_sel_attached_to_proc;
extern ProcRecord channel_ops_duplicate_proc;

extern ProcRecord gimp_image_add_hguide_proc;
extern ProcRecord gimp_image_add_vguide_proc;
extern ProcRecord gimp_image_delete_guide_proc;
extern ProcRecord gimp_image_findnext_guide_proc;
extern ProcRecord gimp_image_get_guide_orientation_proc;
extern ProcRecord gimp_image_get_guide_position_proc;

extern ProcRecord gimp_image_find_parasite_proc;
extern ProcRecord gimp_image_parasite_list_proc;
extern ProcRecord gimp_image_attach_parasite_proc;
extern ProcRecord gimp_image_detach_parasite_proc;

extern ProcRecord gimp_image_get_layer_by_tattoo_proc;
extern ProcRecord gimp_image_get_channel_by_tattoo_proc;

#endif  /*  __GIMAGE_CMDS_H__  */
