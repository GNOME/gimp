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
#ifndef __GIMAGE_H__
#define __GIMAGE_H__

#include "gimpimage.h"
/* icky.. this is temporary */
#include "gimpimageP.h"

typedef GimpImage GImage;

GImage*
gimage_new(int width, int height, GimpImageBaseType base_type);

GImage*
gimage_get_ID (gint ID);

void
gimage_delete (GImage *gimage);

void
gimage_invalidate_previews (void);

void
gimage_set_layer_mask_apply (GImage *gimage, GimpLayer* layer);

void
gimage_set_layer_mask_edit (GImage *gimage, GimpLayer * layer, int edit);

void
gimage_set_layer_mask_show (GImage *gimage, GimpLayer* layer);

void
gimage_foreach (GFunc func, gpointer user_data);


#define gimage_set_filename gimp_image_set_filename
#define gimage_set_resolution gimp_image_set_resolution
#define gimage_get_resolution gimp_image_get_resolution
#define gimage_set_save_proc gimp_image_set_save_proc
#define gimage_get_save_proc gimp_image_get_save_proc
#define gimage_resize gimp_image_resize
#define gimage_scale gimp_image_scale
#define gimage_shadow gimp_image_shadow
#define gimage_free_shadow gimp_image_free_shadow
#define gimage_apply_image gimp_image_apply_image
#define gimage_replace_image gimp_image_replace_image
#define gimage_get_foreground gimp_image_get_foreground
#define gimage_get_background gimp_image_get_background
#define gimage_get_color gimp_image_get_color
#define gimage_transform_color gimp_image_transform_color
#define gimage_add_hguide gimp_image_add_hguide
#define gimage_add_vguide gimp_image_add_vguide
#define gimage_add_guide gimp_image_add_guide
#define gimage_remove_guide gimp_image_remove_guide
#define gimage_delete_guide gimp_image_delete_guide
#define gimage_get_layer_index gimp_image_get_layer_index
#define gimage_get_channel_index gimp_image_get_channel_index
#define gimage_get_active_layer gimp_image_get_active_layer
#define gimage_get_active_channel gimp_image_get_active_channel
#define gimage_get_mask gimp_image_get_mask
#define gimage_get_component_active gimp_image_get_component_active
#define gimage_get_component_visible gimp_image_get_component_visible
#define gimage_layer_boundary gimp_image_layer_boundary
#define gimage_set_active_layer gimp_image_set_active_layer
#define gimage_set_active_channel gimp_image_set_active_channel
#define gimage_unset_active_channel gimp_image_unset_active_channel
#define gimage_set_component_active gimp_image_set_component_active
#define gimage_set_component_visible gimp_image_set_component_visible
#define gimage_pick_correlate_layer gimp_image_pick_correlate_layer
#define gimage_raise_layer gimp_image_raise_layer
#define gimage_lower_layer gimp_image_lower_layer
#define gimage_merge_visible_layers gimp_image_merge_visible_layers
#define gimage_flatten gimp_image_flatten
#define gimage_merge_layers gimp_image_merge_layers
#define gimage_add_layer gimp_image_add_layer
#define gimage_remove_layer gimp_image_remove_layer
#define gimage_add_layer_mask gimp_image_add_layer_mask
#define gimage_remove_layer_mask gimp_image_remove_layer_mask
#define gimage_raise_channel gimp_image_raise_channel
#define gimage_lower_channel gimp_image_lower_channel
#define gimage_add_channel gimp_image_add_channel
#define gimage_remove_channel gimp_image_remove_channel
#define gimage_construct gimp_image_construct
#define gimage_invalidate gimp_image_invalidate
#define gimage_validate gimp_image_validate
#define gimage_is_empty gimp_image_is_empty
#define gimage_active_drawable gimp_image_active_drawable
#define gimage_base_type gimp_image_base_type
#define gimage_base_type_with_alpha gimp_image_base_type_with_alpha
#define gimage_filename gimp_image_filename
#define gimage_enable_undo gimp_image_enable_undo
#define gimage_disable_undo gimp_image_disable_undo
#define gimage_dirty gimp_image_dirty
#define gimage_clean gimp_image_clean
#define gimage_clean_all gimp_image_clean_all
#define gimage_floating_sel gimp_image_floating_sel
#define gimage_cmap gimp_image_cmap
#define gimage_projection gimp_image_projection
#define gimage_projection_type gimp_image_projection_type
#define gimage_projection_bytes gimp_image_projection_bytes
#define gimage_projection_opacity gimp_image_projection_opacity
#define gimage_projection_realloc gimp_image_projection_realloc
#define gimage_composite gimp_image_composite
#define gimage_composite_type gimp_image_composite_type
#define gimage_composite_bytes gimp_image_composite_bytes
#define gimage_composite_preview gimp_image_composite_preview
#define gimage_preview_valid gimp_image_preview_valid
#define gimage_invalidate_preview gimp_image_invalidate_preview

extern guint32 next_guide_id;

#endif /* __GIMAGE_H__ */
