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

#ifndef __GIMP_DISPLAY_SHELL_SCROLLBARS_H__
#define __GIMP_DISPLAY_SHELL_SCROLLBARS_H__


void   gimp_display_shell_scrollbars_update           (GimpDisplayShell *shell);

void   gimp_display_shell_scrollbars_setup_horizontal (GimpDisplayShell *shell,
                                                       gdouble           value);
void   gimp_display_shell_scrollbars_setup_vertical   (GimpDisplayShell *shell,
                                                       gdouble           value);

void   gimp_display_shell_scrollbars_update_steppers  (GimpDisplayShell *shell,
                                                       gint              min_offset_x,
                                                       gint              max_offset_x,
                                                       gint              min_offset_y,
                                                       gint              max_offset_y);


#endif  /*  __GIMP_DISPLAY_SHELL_SCROLLBARS_H__  */
