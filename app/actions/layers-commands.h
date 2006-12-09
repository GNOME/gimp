/* GIMP - The GNU Image Manipulation Program
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

#ifndef __LAYERS_COMMANDS_H__
#define __LAYERS_COMMANDS_H__


void   layers_text_tool_cmd_callback          (GtkAction   *action,
                                               gpointer     data);
void   layers_edit_attributes_cmd_callback    (GtkAction   *action,
                                               gpointer     data);
void   layers_new_cmd_callback                (GtkAction   *action,
                                               gpointer     data);
void   layers_new_last_vals_cmd_callback      (GtkAction   *action,
                                               gpointer     data);

void   layers_select_cmd_callback             (GtkAction   *action,
                                               gint         value,
                                               gpointer     data);

void   layers_raise_cmd_callback              (GtkAction   *action,
                                               gpointer     data);
void   layers_raise_to_top_cmd_callback       (GtkAction   *action,
                                               gpointer     data);
void   layers_lower_cmd_callback              (GtkAction   *action,
                                               gpointer     data);
void   layers_lower_to_bottom_cmd_callback    (GtkAction   *action,
                                               gpointer     data);

void   layers_duplicate_cmd_callback          (GtkAction   *action,
                                               gpointer     data);
void   layers_anchor_cmd_callback             (GtkAction   *action,
                                               gpointer     data);
void   layers_merge_down_cmd_callback         (GtkAction   *action,
                                               gpointer     data);
void   layers_delete_cmd_callback             (GtkAction   *action,
                                               gpointer     data);
void   layers_text_discard_cmd_callback       (GtkAction   *action,
                                               gpointer     data);
void   layers_text_to_vectors_cmd_callback    (GtkAction   *action,
                                               gpointer     data);
void   layers_text_along_vectors_cmd_callback (GtkAction   *action,
                                               gpointer     data);

void   layers_resize_cmd_callback             (GtkAction   *action,
                                               gpointer     data);
void   layers_resize_to_image_cmd_callback    (GtkAction   *action,
                                               gpointer     data);
void   layers_scale_cmd_callback              (GtkAction   *action,
                                               gpointer     data);
void   layers_crop_cmd_callback               (GtkAction   *action,
                                               gpointer     data);

void   layers_mask_add_cmd_callback           (GtkAction   *action,
                                               gpointer     data);
void   layers_mask_apply_cmd_callback         (GtkAction   *action,
                                               gint         value,
                                               gpointer     data);
void   layers_mask_edit_cmd_callback          (GtkAction   *action,
                                               gpointer     data);
void   layers_mask_show_cmd_callback          (GtkAction   *action,
                                               gpointer     data);
void   layers_mask_disable_cmd_callback       (GtkAction   *action,
                                               gpointer     data);
void   layers_mask_to_selection_cmd_callback  (GtkAction   *action,
                                               gint         value,
                                               gpointer     data);

void   layers_alpha_add_cmd_callback          (GtkAction   *action,
                                               gpointer     data);
void   layers_alpha_remove_cmd_callback       (GtkAction   *action,
                                               gpointer     data);
void   layers_alpha_to_selection_cmd_callback (GtkAction   *action,
                                               gint         value,
                                               gpointer     data);

void   layers_opacity_cmd_callback            (GtkAction   *action,
                                               gint         value,
                                               gpointer     data);
void   layers_mode_cmd_callback               (GtkAction   *action,
                                               gint         value,
                                               gpointer     data);

void   layers_lock_alpha_cmd_callback         (GtkAction   *action,
                                               gpointer     data);


#endif /* __LAYERS_COMMANDS_H__ */
