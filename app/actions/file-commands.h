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

#ifndef __FILE_COMMANDS_H__
#define __FILE_COMMANDS_H__


void   file_open_cmd_callback            (GtkAction   *action,
                                          gpointer     data);
void   file_open_as_layers_cmd_callback  (GtkAction   *action,
                                          gpointer     data);
void   file_open_location_cmd_callback   (GtkAction   *action,
                                          gpointer     data);
void   file_last_opened_cmd_callback     (GtkAction   *action,
                                          gint         value,
                                          gpointer     data);

void   file_save_cmd_callback            (GtkAction   *action,
                                          gpointer     data);
void   file_save_as_cmd_callback         (GtkAction   *action,
                                          gpointer     data);
void   file_save_a_copy_cmd_callback     (GtkAction   *action,
                                          gpointer     data);
void   file_save_and_close_cmd_callback  (GtkAction   *action,
                                          gpointer     data);
void   file_save_template_cmd_callback   (GtkAction   *action,
                                          gpointer     data);

void   file_revert_cmd_callback          (GtkAction   *action,
                                          gpointer     data);
void   file_close_all_cmd_callback       (GtkAction   *action,
                                          gpointer     data);
void   file_quit_cmd_callback            (GtkAction   *action,
                                          gpointer     data);

void   file_file_open_dialog             (Gimp        *gimp,
                                          const gchar *uri,
                                          GtkWidget   *parent);


#endif /* __FILE_COMMANDS_H__ */
