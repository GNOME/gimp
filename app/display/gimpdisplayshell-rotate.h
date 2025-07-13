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


void   gimp_display_shell_flip                    (GimpDisplayShell *shell,
                                                   gboolean          flip_horizontally,
                                                   gboolean          flip_vertically);

void   gimp_display_shell_rotate                  (GimpDisplayShell *shell,
                                                   gdouble           delta);
void   gimp_display_shell_rotate_to               (GimpDisplayShell *shell,
                                                   gdouble           value);
void   gimp_display_shell_rotate_drag             (GimpDisplayShell *shell,
                                                   gdouble           last_x,
                                                   gdouble           last_y,
                                                   gdouble           cur_x,
                                                   gdouble           cur_y,
                                                   gboolean          constrain);

void   gimp_display_shell_rotate_update_transform (GimpDisplayShell *shell);
