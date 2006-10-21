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

#ifndef __EDIT_COMMANDS_H__
#define __EDIT_COMMANDS_H__


void   edit_undo_cmd_callback               (GtkAction *action,
                                             gpointer   data);
void   edit_redo_cmd_callback               (GtkAction *action,
                                             gpointer   data);
void   edit_strong_undo_cmd_callback        (GtkAction *action,
                                             gpointer   data);
void   edit_strong_redo_cmd_callback        (GtkAction *action,
                                             gpointer   data);
void   edit_undo_clear_cmd_callback         (GtkAction *action,
                                             gpointer   data);
void   edit_fade_cmd_callback               (GtkAction *action,
                                             gpointer   data);
void   edit_cut_cmd_callback                (GtkAction *action,
                                             gpointer   data);
void   edit_copy_cmd_callback               (GtkAction *action,
                                             gpointer   data);
void   edit_copy_visible_cmd_callback       (GtkAction *action,
                                             gpointer   data);
void   edit_paste_cmd_callback              (GtkAction *action,
                                             gpointer   data);
void   edit_paste_into_cmd_callback         (GtkAction *action,
                                             gpointer   data);
void   edit_paste_as_new_cmd_callback       (GtkAction *action,
                                             gpointer   data);
void   edit_named_cut_cmd_callback          (GtkAction *action,
                                             gpointer   data);
void   edit_named_copy_cmd_callback         (GtkAction *action,
                                             gpointer   data);
void   edit_named_copy_visible_cmd_callback (GtkAction *action,
                                             gpointer   data);
void   edit_named_paste_cmd_callback        (GtkAction *action,
                                             gpointer   data);
void   edit_clear_cmd_callback              (GtkAction *action,
                                             gpointer   data);
void   edit_fill_cmd_callback               (GtkAction *action,
                                             gint       value,
                                             gpointer   data);


#endif /* __EDIT_COMMANDS_H__ */
