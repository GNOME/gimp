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

#ifndef __VIEW_COMMANDS_H__
#define __VIEW_COMMANDS_H__


void   view_zoomin_cmd_callback           (GtkWidget *widget,
					   gpointer   data);
void   view_zoomout_cmd_callback          (GtkWidget *widget,
					   gpointer   data);
void   view_zoom_cmd_callback             (GtkWidget *widget,
					   gpointer   data,
					   guint      action);
void   view_dot_for_dot_cmd_callback      (GtkWidget *widget,
					   gpointer   data);
void   view_info_window_cmd_callback      (GtkWidget *widget,
					   gpointer   data);
void   view_nav_window_cmd_callback       (GtkWidget *widget,
					   gpointer   data);
void   view_toggle_selection_cmd_callback (GtkWidget *widget,
					   gpointer   data);
void   view_toggle_rulers_cmd_callback    (GtkWidget *widget,
					   gpointer   data);
void   view_toggle_statusbar_cmd_callback (GtkWidget *widget,
					   gpointer   data);
void   view_toggle_guides_cmd_callback    (GtkWidget *widget,
					   gpointer   data);
void   view_snap_to_guides_cmd_callback   (GtkWidget *widget,
					   gpointer   data);
void   view_new_view_cmd_callback         (GtkWidget *widget,
					   gpointer   data);
void   view_shrink_wrap_cmd_callback      (GtkWidget *widget,
					   gpointer   data);


#endif /* __VIEW_COMMANDS_H__ */
