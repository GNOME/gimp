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

#ifndef __GIMP_DISPLAY_SHELL_APPEARANCE_H__
#define __GIMP_DISPLAY_SHELL_APPEARANCE_H__


void       gimp_display_shell_set_fullscreen         (GimpDisplayShell *shell,
                                                      gboolean          fullscreen);
gboolean   gimp_display_shell_get_fullscreen         (GimpDisplayShell *shell);

void       gimp_display_shell_set_show_menubar       (GimpDisplayShell *shell,
                                                      gboolean          show);
gboolean   gimp_display_shell_get_show_menubar       (GimpDisplayShell *shell);

void       gimp_display_shell_set_show_rulers        (GimpDisplayShell *shell,
                                                      gboolean          show);
gboolean   gimp_display_shell_get_show_rulers        (GimpDisplayShell *shell);

void       gimp_display_shell_set_show_scrollbars    (GimpDisplayShell *shell,
                                                      gboolean          show);
gboolean   gimp_display_shell_get_show_scrollbars    (GimpDisplayShell *shell);

void       gimp_display_shell_set_show_statusbar     (GimpDisplayShell *shell,
                                                      gboolean          show);
gboolean   gimp_display_shell_get_show_statusbar     (GimpDisplayShell *shell);

void       gimp_display_shell_set_show_selection     (GimpDisplayShell *shell,
                                                      gboolean          show);
gboolean   gimp_display_shell_get_show_selection     (GimpDisplayShell *shell);

void       gimp_display_shell_set_show_layer         (GimpDisplayShell *shell,
                                                      gboolean          show);
gboolean   gimp_display_shell_get_show_layer         (GimpDisplayShell *shell);

void       gimp_display_shell_set_show_transform     (GimpDisplayShell *shell,
                                                      gboolean          show);
gboolean   gimp_display_shell_get_show_transform     (GimpDisplayShell *shell);

void       gimp_display_shell_set_show_grid          (GimpDisplayShell *shell,
                                                      gboolean          show);
gboolean   gimp_display_shell_get_show_grid          (GimpDisplayShell *shell);

void       gimp_display_shell_set_show_guides        (GimpDisplayShell *shell,
                                                      gboolean          show);
gboolean   gimp_display_shell_get_show_guides        (GimpDisplayShell *shell);

void       gimp_display_shell_set_snap_to_grid       (GimpDisplayShell *shell,
                                                      gboolean          snap);
gboolean   gimp_display_shell_get_snap_to_grid       (GimpDisplayShell *shell);

void       gimp_display_shell_set_show_sample_points (GimpDisplayShell *shell,
                                                      gboolean          show);
gboolean   gimp_display_shell_get_show_sample_points (GimpDisplayShell *shell);

void       gimp_display_shell_set_snap_to_guides     (GimpDisplayShell *shell,
                                                      gboolean          snap);
gboolean   gimp_display_shell_get_snap_to_guides     (GimpDisplayShell *shell);

void       gimp_display_shell_set_snap_to_canvas     (GimpDisplayShell *shell,
                                                      gboolean          snap);
gboolean   gimp_display_shell_get_snap_to_canvas     (GimpDisplayShell *shell);

void       gimp_display_shell_set_snap_to_vectors    (GimpDisplayShell *shell,
                                                      gboolean          snap);
gboolean   gimp_display_shell_get_snap_to_vectors    (GimpDisplayShell *shell);

void       gimp_display_shell_set_padding            (GimpDisplayShell *shell,
                                                      GimpCanvasPaddingMode  mode,
                                                      const GimpRGB    *color);
void       gimp_display_shell_get_padding            (GimpDisplayShell *shell,
                                                      GimpCanvasPaddingMode *mode,
                                                      GimpRGB          *color);


#endif /* __GIMP_DISPLAY_SHELL_APPEARANCE_H__ */
