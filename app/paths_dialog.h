/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Andy Thomas alt@picnic.demon.co.uk
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
#ifndef  __PATHS_DIALOG_H__
#define  __PATHS_DIALOG_H__

void  paths_dialog_new_path_callback    (GtkWidget *, gpointer);
void  paths_dialog_delete_path_callback (GtkWidget *, gpointer);
void  paths_dialog_dup_path_callback    (GtkWidget *, gpointer);
void  paths_dialog_copy_path_callback   (GtkWidget *, gpointer);
void  paths_dialog_paste_path_callback  (GtkWidget *, gpointer);
void  paths_dialog_stroke_path_callback (GtkWidget *, gpointer);
void  paths_dialog_path_to_sel_callback (GtkWidget *, gpointer);
void  paths_dialog_sel_to_path_callback (GtkWidget *, gpointer);
void  paths_dialog_import_path_callback (GtkWidget *, gpointer);
void  paths_dialog_export_path_callback (GtkWidget *, gpointer);

#endif  /*  __PATHS_DIALOG_H__  */
