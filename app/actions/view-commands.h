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

#ifndef __VIEW_COMMANDS_H__
#define __VIEW_COMMANDS_H__


void   view_new_cmd_callback                   (GtkAction *action,
                                                gpointer   data);

void   view_zoom_fit_in_cmd_callback           (GtkAction *action,
                                                gpointer   data);
void   view_zoom_fit_to_cmd_callback           (GtkAction *action,
                                                gpointer   data);
void   view_zoom_cmd_callback                  (GtkAction *action,
                                                gint       value,
                                                gpointer   data);
void   view_zoom_explicit_cmd_callback         (GtkAction *action,
                                                GtkAction *current,
                                                gpointer   data);
void   view_zoom_other_cmd_callback            (GtkAction *action,
                                                gpointer   data);
void   view_dot_for_dot_cmd_callback           (GtkAction *action,
                                                gpointer   data);

void   view_scroll_horizontal_cmd_callback     (GtkAction *action,
                                                gint       value,
                                                gpointer   data);
void   view_scroll_vertical_cmd_callback       (GtkAction *action,
                                                gint       value,
                                                gpointer   data);

void   view_navigation_window_cmd_callback     (GtkAction *action,
                                                gpointer   data);
void   view_display_filters_cmd_callback       (GtkAction *action,
                                                gpointer   data);
void   view_toggle_selection_cmd_callback      (GtkAction *action,
                                                gpointer   data);
void   view_toggle_layer_boundary_cmd_callback (GtkAction *action,
                                                gpointer   data);
void   view_toggle_menubar_cmd_callback        (GtkAction *action,
                                                gpointer   data);
void   view_toggle_rulers_cmd_callback         (GtkAction *action,
                                                gpointer   data);
void   view_toggle_scrollbars_cmd_callback     (GtkAction *action,
                                                gpointer   data);
void   view_toggle_statusbar_cmd_callback      (GtkAction *action,
                                                gpointer   data);
void   view_toggle_guides_cmd_callback         (GtkAction *action,
                                                gpointer   data);
void   view_toggle_grid_cmd_callback           (GtkAction *action,
                                                gpointer   data);
void   view_toggle_sample_points_cmd_callback  (GtkAction *action,
                                                gpointer   data);

void   view_snap_to_guides_cmd_callback        (GtkAction *action,
                                                gpointer   data);
void   view_snap_to_grid_cmd_callback          (GtkAction *action,
                                                gpointer   data);
void   view_snap_to_canvas_cmd_callback        (GtkAction *action,
                                                gpointer   data);
void   view_snap_to_vectors_cmd_callback       (GtkAction *action,
                                                gpointer   data);
void   view_padding_color_cmd_callback         (GtkAction *action,
                                                gint       value,
                                                gpointer   data);

void   view_shrink_wrap_cmd_callback           (GtkAction *action,
                                                gpointer   data);
void   view_fullscreen_cmd_callback            (GtkAction *action,
                                                gpointer   data);


#endif /* __VIEW_COMMANDS_H__ */
