/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_DISPLAY_SHELL_SCALE_H__
#define __LIGMA_DISPLAY_SHELL_SCALE_H__


gboolean ligma_display_shell_scale_revert             (LigmaDisplayShell *shell);
gboolean ligma_display_shell_scale_can_revert         (LigmaDisplayShell *shell);
void     ligma_display_shell_scale_save_revert_values (LigmaDisplayShell *shell);

void     ligma_display_shell_scale_set_dot_for_dot    (LigmaDisplayShell *shell,
                                                      gboolean          dot_for_dot);

void     ligma_display_shell_scale_get_image_size     (LigmaDisplayShell *shell,
                                                      gint             *w,
                                                      gint             *h);
void     ligma_display_shell_scale_get_image_bounds   (LigmaDisplayShell *shell,
                                                      gint             *x,
                                                      gint             *y,
                                                      gint             *w,
                                                      gint             *h);
void     ligma_display_shell_scale_get_image_unrotated_bounds
                                                     (LigmaDisplayShell *shell,
                                                      gint             *x,
                                                      gint             *y,
                                                      gint             *w,
                                                      gint             *h);
void     ligma_display_shell_scale_get_image_bounding_box
                                                     (LigmaDisplayShell *shell,
                                                      gint             *x,
                                                      gint             *y,
                                                      gint             *w,
                                                      gint             *h);
void     ligma_display_shell_scale_get_image_unrotated_bounding_box
                                                     (LigmaDisplayShell *shell,
                                                      gint             *x,
                                                      gint             *y,
                                                      gint             *w,
                                                      gint             *h);
gboolean ligma_display_shell_scale_image_is_within_viewport
                                                     (LigmaDisplayShell *shell,
                                                      gboolean         *horizontally,
                                                      gboolean         *vertically);

void     ligma_display_shell_scale_update             (LigmaDisplayShell *shell);

void     ligma_display_shell_scale                    (LigmaDisplayShell *shell,
                                                      LigmaZoomType      zoom_type,
                                                      gdouble           scale,
                                                      LigmaZoomFocus     zoom_focus);
void     ligma_display_shell_scale_to_rectangle       (LigmaDisplayShell *shell,
                                                      LigmaZoomType      zoom_type,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gdouble           width,
                                                      gdouble           height,
                                                      gboolean          resize_window);
void     ligma_display_shell_scale_fit_in             (LigmaDisplayShell *shell);
void     ligma_display_shell_scale_fill               (LigmaDisplayShell *shell);
void     ligma_display_shell_scale_by_values          (LigmaDisplayShell *shell,
                                                      gdouble           scale,
                                                      gint              offset_x,
                                                      gint              offset_y,
                                                      gboolean          resize_window);

void     ligma_display_shell_scale_drag               (LigmaDisplayShell *shell,
                                                      gdouble           start_x,
                                                      gdouble           start_y,
                                                      gdouble           delta_x,
                                                      gdouble           delta_y);

void     ligma_display_shell_scale_shrink_wrap        (LigmaDisplayShell *shell,
                                                      gboolean          grow_only);
void     ligma_display_shell_scale_resize             (LigmaDisplayShell *shell,
                                                      gboolean          resize_window,
                                                      gboolean          grow_only);
void     ligma_display_shell_set_initial_scale        (LigmaDisplayShell *shell,
                                                      gdouble           scale,
                                                      gint             *display_width,
                                                      gint             *display_height);

void     ligma_display_shell_get_rotated_scale        (LigmaDisplayShell *shell,
                                                      gdouble          *scale_x,
                                                      gdouble          *scale_y);

/*  debug API for testing  */

void  ligma_display_shell_push_zoom_focus_pointer_pos (LigmaDisplayShell *shell,
                                                      gint              x,
                                                      gint              y);


#endif  /*  __LIGMA_DISPLAY_SHELL_SCALE_H__  */
