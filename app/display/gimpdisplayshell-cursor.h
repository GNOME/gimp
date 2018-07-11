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

#ifndef __GIMP_DISPLAY_SHELL_CURSOR_H__
#define __GIMP_DISPLAY_SHELL_CURSOR_H__


/*  functions dealing with the normal windowing system cursor  */

void   gimp_display_shell_set_cursor             (GimpDisplayShell    *shell,
                                                  GimpCursorType       cursor_type,
                                                  GimpToolCursorType   tool_cursor,
                                                  GimpCursorModifier   modifier);
void   gimp_display_shell_unset_cursor           (GimpDisplayShell    *shell);
void   gimp_display_shell_set_override_cursor    (GimpDisplayShell    *shell,
                                                  GimpCursorType       cursor_type);
void   gimp_display_shell_unset_override_cursor  (GimpDisplayShell    *shell);


/*  functions dealing with the software cursor that is drawn to the
 *  canvas by GIMP
 */

void   gimp_display_shell_update_software_cursor (GimpDisplayShell    *shell,
                                                  GimpCursorPrecision  precision,
                                                  gint                 display_x,
                                                  gint                 display_y,
                                                  gdouble              image_x,
                                                  gdouble              image_y);
void   gimp_display_shell_clear_software_cursor  (GimpDisplayShell    *shell);


#endif /* __GIMP_DISPLAY_SHELL_CURSOR_H__ */
