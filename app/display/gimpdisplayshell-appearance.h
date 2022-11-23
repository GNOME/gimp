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

#ifndef __LIGMA_DISPLAY_SHELL_APPEARANCE_H__
#define __LIGMA_DISPLAY_SHELL_APPEARANCE_H__


void       ligma_display_shell_appearance_update       (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_show_menubar        (LigmaDisplayShell       *shell,
                                                       gboolean                show);
gboolean   ligma_display_shell_get_show_menubar        (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_show_statusbar      (LigmaDisplayShell       *shell,
                                                       gboolean                show);
gboolean   ligma_display_shell_get_show_statusbar      (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_show_rulers         (LigmaDisplayShell       *shell,
                                                       gboolean                show);
gboolean   ligma_display_shell_get_show_rulers         (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_show_scrollbars     (LigmaDisplayShell       *shell,
                                                       gboolean                show);
gboolean   ligma_display_shell_get_show_scrollbars     (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_show_selection      (LigmaDisplayShell       *shell,
                                                       gboolean                show);
gboolean   ligma_display_shell_get_show_selection      (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_show_layer          (LigmaDisplayShell       *shell,
                                                       gboolean                show);
gboolean   ligma_display_shell_get_show_layer          (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_show_canvas         (LigmaDisplayShell       *shell,
                                                       gboolean                show);
gboolean   ligma_display_shell_get_show_canvas         (LigmaDisplayShell       *shell);
void       ligma_display_shell_update_show_canvas      (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_show_grid           (LigmaDisplayShell       *shell,
                                                       gboolean                show);
gboolean   ligma_display_shell_get_show_grid           (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_show_guides         (LigmaDisplayShell       *shell,
                                                       gboolean                show);
gboolean   ligma_display_shell_get_show_guides         (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_snap_to_grid        (LigmaDisplayShell       *shell,
                                                       gboolean                snap);
gboolean   ligma_display_shell_get_snap_to_grid        (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_show_sample_points  (LigmaDisplayShell       *shell,
                                                       gboolean                show);
gboolean   ligma_display_shell_get_show_sample_points  (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_snap_to_guides      (LigmaDisplayShell       *shell,
                                                       gboolean                snap);
gboolean   ligma_display_shell_get_snap_to_guides      (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_snap_to_canvas      (LigmaDisplayShell       *shell,
                                                       gboolean                snap);
gboolean   ligma_display_shell_get_snap_to_canvas      (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_snap_to_vectors     (LigmaDisplayShell       *shell,
                                                       gboolean                snap);
gboolean   ligma_display_shell_get_snap_to_vectors     (LigmaDisplayShell       *shell);

void       ligma_display_shell_set_padding             (LigmaDisplayShell       *shell,
                                                       LigmaCanvasPaddingMode   mode,
                                                       const LigmaRGB          *color);
void       ligma_display_shell_get_padding             (LigmaDisplayShell       *shell,
                                                       LigmaCanvasPaddingMode  *mode,
                                                       LigmaRGB                *color);
void       ligma_display_shell_set_padding_in_show_all (LigmaDisplayShell       *shell,
                                                       gboolean                keep);
gboolean   ligma_display_shell_get_padding_in_show_all (LigmaDisplayShell       *shell);


#endif /* __LIGMA_DISPLAY_SHELL_APPEARANCE_H__ */
