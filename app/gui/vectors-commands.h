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

#ifndef __VECTORS_COMMANDS_H__
#define __VECTORS_COMMANDS_H__


void   vectors_new_vectors_cmd_callback                (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_raise_vectors_cmd_callback              (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_lower_vectors_cmd_callback              (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_duplicate_vectors_cmd_callback          (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_delete_vectors_cmd_callback             (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_vectors_to_sel_cmd_callback             (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_add_vectors_to_sel_cmd_callback         (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_sub_vectors_from_sel_cmd_callback       (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_intersect_vectors_with_sel_cmd_callback (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_sel_to_vectors_cmd_callback             (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_stroke_vectors_cmd_callback             (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_copy_vectors_cmd_callback               (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_paste_vectors_cmd_callback              (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_import_vectors_cmd_callback             (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_export_vectors_cmd_callback             (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_vectors_tool_cmd_callback               (GtkWidget   *widget,
                                                        gpointer     data);
void   vectors_edit_vectors_attributes_cmd_callback    (GtkWidget   *widget,
                                                        gpointer     data);

void   vectors_stroke_vectors                          (GimpVectors *vectors);
void   vectors_vectors_tool                            (GimpVectors *vectors);
void   vectors_new_vectors_query                       (GimpImage   *gimage,
                                                        GimpVectors *template);
void   vectors_edit_vectors_query                      (GimpVectors *vectors);

void   vectors_menu_update                             (GtkItemFactory *factory,
                                                        gpointer        data);


#endif /* __VECTORS_COMMANDS_H__ */
