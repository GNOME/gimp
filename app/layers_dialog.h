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
#ifndef __LAYERS_DIALOG_H__
#define __LAYERS_DIALOG_H__

void  layers_dialog_previous_layer_callback        (GtkWidget *, gpointer);
void  layers_dialog_next_layer_callback            (GtkWidget *, gpointer);
void  layers_dialog_raise_layer_callback           (GtkWidget *, gpointer);
void  layers_dialog_lower_layer_callback           (GtkWidget *, gpointer);
void  layers_dialog_raise_layer_to_top_callback    (GtkWidget *, gpointer);
void  layers_dialog_lower_layer_to_bottom_callback (GtkWidget *, gpointer);
void  layers_dialog_new_layer_callback             (GtkWidget *, gpointer);
void  layers_dialog_duplicate_layer_callback       (GtkWidget *, gpointer);
void  layers_dialog_delete_layer_callback          (GtkWidget *, gpointer);
void  layers_dialog_scale_layer_callback           (GtkWidget *, gpointer);
void  layers_dialog_resize_layer_callback          (GtkWidget *, gpointer);
void  layers_dialog_add_layer_mask_callback        (GtkWidget *, gpointer);
void  layers_dialog_apply_layer_mask_callback      (GtkWidget *, gpointer);
void  layers_dialog_anchor_layer_callback          (GtkWidget *, gpointer);
void  layers_dialog_merge_layers_callback          (GtkWidget *, gpointer);
void  layers_dialog_merge_down_callback            (GtkWidget *, gpointer);
void  layers_dialog_flatten_image_callback         (GtkWidget *, gpointer);
void  layers_dialog_alpha_select_callback          (GtkWidget *, gpointer);
void  layers_dialog_mask_select_callback           (GtkWidget *, gpointer);
void  layers_dialog_add_alpha_channel_callback     (GtkWidget *, gpointer);

#endif /* __LAYERS_DIALOG_H__ */
