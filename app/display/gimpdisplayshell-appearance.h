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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


void       gimp_display_shell_appearance_update        (GimpDisplayShell       *shell);

void       gimp_display_shell_set_show_menubar         (GimpDisplayShell       *shell,
                                                        gboolean                show);
gboolean   gimp_display_shell_get_show_menubar         (GimpDisplayShell       *shell);

void       gimp_display_shell_set_show_statusbar       (GimpDisplayShell       *shell,
                                                        gboolean                show);
gboolean   gimp_display_shell_get_show_statusbar       (GimpDisplayShell       *shell);

void       gimp_display_shell_set_show_rulers          (GimpDisplayShell       *shell,
                                                        gboolean                show);
gboolean   gimp_display_shell_get_show_rulers          (GimpDisplayShell       *shell);

void       gimp_display_shell_set_show_scrollbars      (GimpDisplayShell       *shell,
                                                        gboolean                show);
gboolean   gimp_display_shell_get_show_scrollbars      (GimpDisplayShell       *shell);

void       gimp_display_shell_set_show_selection       (GimpDisplayShell       *shell,
                                                        gboolean                show);
gboolean   gimp_display_shell_get_show_selection       (GimpDisplayShell       *shell);

void       gimp_display_shell_set_show_layer           (GimpDisplayShell       *shell,
                                                        gboolean                show);
gboolean   gimp_display_shell_get_show_layer           (GimpDisplayShell       *shell);

void       gimp_display_shell_set_show_canvas          (GimpDisplayShell       *shell,
                                                        gboolean                show);
gboolean   gimp_display_shell_get_show_canvas          (GimpDisplayShell       *shell);
void       gimp_display_shell_update_show_canvas       (GimpDisplayShell       *shell);

void       gimp_display_shell_set_show_grid            (GimpDisplayShell       *shell,
                                                        gboolean                show);
gboolean   gimp_display_shell_get_show_grid            (GimpDisplayShell       *shell);

void       gimp_display_shell_set_show_guides          (GimpDisplayShell       *shell,
                                                        gboolean                show);
gboolean   gimp_display_shell_get_show_guides          (GimpDisplayShell       *shell);

void       gimp_display_shell_set_snap_to_grid         (GimpDisplayShell       *shell,
                                                        gboolean                snap);
gboolean   gimp_display_shell_get_snap_to_grid         (GimpDisplayShell       *shell);

void       gimp_display_shell_set_show_sample_points   (GimpDisplayShell       *shell,
                                                        gboolean                show);
gboolean   gimp_display_shell_get_show_sample_points   (GimpDisplayShell       *shell);

void       gimp_display_shell_set_snap_to_guides       (GimpDisplayShell       *shell,
                                                        gboolean                snap);
gboolean   gimp_display_shell_get_snap_to_guides       (GimpDisplayShell       *shell);

void       gimp_display_shell_set_snap_to_canvas       (GimpDisplayShell       *shell,
                                                        gboolean                snap);
gboolean   gimp_display_shell_get_snap_to_canvas       (GimpDisplayShell       *shell);

void       gimp_display_shell_set_snap_to_path         (GimpDisplayShell       *shell,
                                                        gboolean                snap);
gboolean   gimp_display_shell_get_snap_to_path         (GimpDisplayShell       *shell);

void       gimp_display_shell_set_snap_to_bbox         (GimpDisplayShell       *shell,
                                                        gboolean                snap);
gboolean   gimp_display_shell_get_snap_to_bbox         (GimpDisplayShell       *shell);

void       gimp_display_shell_set_snap_to_equidistance (GimpDisplayShell       *shell,
                                                        gboolean                snap);
gboolean   gimp_display_shell_get_snap_to_equidistance (GimpDisplayShell       *shell);

void       gimp_display_shell_set_padding              (GimpDisplayShell       *shell,
                                                        GimpCanvasPaddingMode   mode,
                                                        GeglColor              *color);
void       gimp_display_shell_get_padding              (GimpDisplayShell       *shell,
                                                        GimpCanvasPaddingMode  *mode,
                                                        GeglColor             **color);
void       gimp_display_shell_set_padding_in_show_all  (GimpDisplayShell       *shell,
                                                        gboolean                keep);
gboolean   gimp_display_shell_get_padding_in_show_all  (GimpDisplayShell       *shell);
