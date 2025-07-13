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


void     gimp_display_shell_render_set_scale       (GimpDisplayShell *shell,
                                                    gint              scale);

void     gimp_display_shell_render_invalidate_full (GimpDisplayShell *shell);
void     gimp_display_shell_render_invalidate_area (GimpDisplayShell *shell,
                                                    gint              x,
                                                    gint              y,
                                                    gint              width,
                                                    gint              height);

void     gimp_display_shell_render_validate_area   (GimpDisplayShell *shell,
                                                    gint              x,
                                                    gint              y,
                                                    gint              width,
                                                    gint              height);

gboolean gimp_display_shell_render_is_valid        (GimpDisplayShell *shell,
                                                    gint              x,
                                                    gint              y,
                                                    gint              width,
                                                    gint              height);

void     gimp_display_shell_render                 (GimpDisplayShell *shell,
                                                    cairo_t          *cr,
                                                    gint              x,
                                                    gint              y,
                                                    gint              width,
                                                    gint              height,
                                                    gdouble           scale);
