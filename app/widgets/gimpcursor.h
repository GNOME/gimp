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

#ifndef __GIMP_CURSOR_H__
#define __GIMP_CURSOR_H__


GdkCursor      * gimp_cursor_new    (GdkWindow          *window,
                                     GimpHandedness      cursor_handedness,
                                     GimpCursorType      cursor_type,
                                     GimpToolCursorType  tool_cursor,
                                     GimpCursorModifier  modifier);
void             gimp_cursor_set    (GtkWidget          *widget,
                                     GimpHandedness      cursor_handedness,
                                     GimpCursorType      cursor_type,
                                     GimpToolCursorType  tool_cursor,
                                     GimpCursorModifier  modifier);

GimpCursorType   gimp_cursor_rotate (GimpCursorType      cursor,
                                     gdouble             angle);


#endif /*  __GIMP_CURSOR_H__  */
