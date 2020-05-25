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

#ifndef __GIMP_DISPLAY_SHELL_UTILS_H__
#define __GIMP_DISPLAY_SHELL_UTILS_H__


void      gimp_display_shell_get_constrained_line_params (GimpDisplayShell *shell,
                                                          gdouble          *offset_angle,
                                                          gdouble          *xres,
                                                          gdouble          *yres);
void      gimp_display_shell_constrain_line              (GimpDisplayShell *shell,
                                                          gdouble           start_x,
                                                          gdouble           start_y,
                                                          gdouble          *end_x,
                                                          gdouble          *end_y,
                                                          gint              n_snap_lines);
gdouble   gimp_display_shell_constrain_angle             (GimpDisplayShell *shell,
                                                          gdouble           angle,
                                                          gint              n_snap_lines);

gchar   * gimp_display_shell_get_line_status             (GimpDisplayShell *shell,
                                                          const gchar      *status,
                                                          const gchar      *separator,
                                                          gdouble           x1,
                                                          gdouble           y1,
                                                          gdouble           x2,
                                                          gdouble           y2);


#endif  /*  __GIMP_DISPLAY_SHELL_UTILS_H__  */
