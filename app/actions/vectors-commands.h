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

#ifndef __VECTORS_COMMANDS_H__
#define __VECTORS_COMMANDS_H__


void   vectors_vectors_tool_cmd_callback         (GtkAction   *action,
                                                  gpointer     data);
void   vectors_edit_attributes_cmd_callback      (GtkAction   *action,
                                                  gpointer     data);
void   vectors_new_cmd_callback                  (GtkAction   *action,
                                                  gpointer     data);
void   vectors_new_last_vals_cmd_callback        (GtkAction   *action,
                                                  gpointer     data);

void   vectors_raise_cmd_callback                (GtkAction   *action,
                                                  gpointer     data);
void   vectors_raise_to_top_cmd_callback         (GtkAction   *action,
                                                  gpointer     data);
void   vectors_lower_cmd_callback                (GtkAction   *action,
                                                  gpointer     data);
void   vectors_lower_to_bottom_cmd_callback      (GtkAction   *action,
                                                  gpointer     data);

void   vectors_duplicate_cmd_callback            (GtkAction   *action,
                                                  gpointer     data);
void   vectors_delete_cmd_callback               (GtkAction   *action,
                                                  gpointer     data);
void   vectors_merge_visible_cmd_callback        (GtkAction   *action,
                                                  gpointer     data);
void   vectors_to_selection_cmd_callback         (GtkAction   *action,
                                                  gint         value,
                                                  gpointer     data);
void   vectors_selection_to_vectors_cmd_callback (GtkAction   *action,
                                                  gint         value,
                                                  gpointer     data);
void   vectors_stroke_cmd_callback               (GtkAction   *action,
                                                  gpointer     data);
void   vectors_stroke_last_vals_cmd_callback     (GtkAction   *action,
                                                  gpointer     data);

void   vectors_copy_cmd_callback                 (GtkAction   *action,
                                                  gpointer     data);
void   vectors_paste_cmd_callback                (GtkAction   *action,
                                                  gpointer     data);
void   vectors_export_cmd_callback               (GtkAction   *action,
                                                  gpointer     data);
void   vectors_import_cmd_callback               (GtkAction   *action,
                                                  gpointer     data);

void   vectors_visible_cmd_callback              (GtkAction   *action,
                                                  gpointer     data);
void   vectors_linked_cmd_callback               (GtkAction   *action,
                                                  gpointer     data);


#endif /* __VECTORS_COMMANDS_H__ */
