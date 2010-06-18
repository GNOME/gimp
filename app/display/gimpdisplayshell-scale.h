/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_DISPLAY_SHELL_SCALE_H__
#define __GIMP_DISPLAY_SHELL_SCALE_H__


void     gimp_display_shell_update_scrollbars_and_rulers   (GimpDisplayShell *shell);
void     gimp_display_shell_scale_update_scrollbars        (GimpDisplayShell *shell);
void     gimp_display_shell_scale_update_rulers            (GimpDisplayShell *shell);

gboolean gimp_display_shell_scale_revert                   (GimpDisplayShell *shell);
gboolean gimp_display_shell_scale_can_revert               (GimpDisplayShell *shell);

void     gimp_display_shell_scale_set_dot_for_dot          (GimpDisplayShell *shell,
                                                            gboolean          dot_for_dot);

void     gimp_display_shell_get_screen_resolution          (GimpDisplayShell *shell,
                                                            gdouble          *xres,
                                                            gdouble          *yres);

void     gimp_display_shell_scale                          (GimpDisplayShell *shell,
                                                            GimpZoomType      zoom_type,
                                                            gdouble           scale,
                                                            GimpZoomFocus     zoom_focus);
void     gimp_display_shell_scale_fit_in                   (GimpDisplayShell *shell);
gboolean gimp_display_shell_scale_image_is_within_viewport (GimpDisplayShell *shell,
                                                            gboolean         *horizontally,
                                                            gboolean         *vertically);
void     gimp_display_shell_scale_fill                     (GimpDisplayShell *shell);
void     gimp_display_shell_scale_handle_zoom_revert       (GimpDisplayShell *shell);
void     gimp_display_shell_scale_by_values                (GimpDisplayShell *shell,
                                                            gdouble           scale,
                                                            gint              offset_x,
                                                            gint              offset_y,
                                                            gboolean          resize_window);
void     gimp_display_shell_scale_shrink_wrap              (GimpDisplayShell *shell,
                                                            gboolean          grow_only);

void     gimp_display_shell_scale_resize                   (GimpDisplayShell *shell,
                                                            gboolean          resize_window,
                                                            gboolean          grow_only);
void     gimp_display_shell_calculate_scale_x_and_y        (GimpDisplayShell *shell,
                                                            gdouble           scale,
                                                            gdouble          *scale_x,
                                                            gdouble          *scale_y);
void     gimp_display_shell_set_initial_scale              (GimpDisplayShell *shell,
                                                            gdouble           scale,
                                                            gint             *display_width,
                                                            gint             *display_height);
void     gimp_display_shell_push_zoom_focus_pointer_pos    (GimpDisplayShell *shell,
                                                            gint              x,
                                                            gint              y);


#endif  /*  __GIMP_DISPLAY_SHELL_SCALE_H__  */
