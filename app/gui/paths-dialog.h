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

GtkWidget * paths_dialog_create     (void);
void        paths_dialog_free       (void);
void        paths_dialog_update     (GimpImage *gimage);
void        paths_dialog_flush      (void);


void   paths_dialog_new_path_callback             (GtkWidget *widget,
						   gpointer   data);
void   paths_dialog_delete_path_callback          (GtkWidget *widget,
						   gpointer   data);
void   paths_dialog_dup_path_callback             (GtkWidget *widget,
						   gpointer   data);
void   paths_dialog_copy_path_callback            (GtkWidget *widget,
						   gpointer   data);
void   paths_dialog_paste_path_callback           (GtkWidget *widget,
						   gpointer   data);
void   paths_dialog_stroke_path_callback          (GtkWidget *widget,
						   gpointer   data);
void   paths_dialog_path_to_sel_callback          (GtkWidget *widget,
						   gpointer   data);
void   paths_dialog_sel_to_path_callback          (GtkWidget *widget,
						   gpointer   data);
void   paths_dialog_import_path_callback          (GtkWidget *widget,
						   gpointer   data);
void   paths_dialog_export_path_callback          (GtkWidget *widget,
						   gpointer   data);
void   paths_dialog_edit_path_attributes_callback (GtkWidget *widget,
						   gpointer   data);
void   paths_dialog_destroy_cb                    (GtkObject *widget,
						   gpointer   data);


/* EEEK */

void   paths_newpoint_current       (GimpBezierSelectTool *, GDisplay *);
void   paths_first_button_press     (GimpBezierSelectTool *, GDisplay *);
void   paths_new_bezier_select_tool (void);
Path * paths_get_bzpaths            (void); 
void   paths_set_bzpaths            (GimpImage *, Path *);
void   paths_dialog_set_default_op  (void);


#endif  /*  __PATHS_DIALOG_H__  */
