/* The GIMP -- an image manipulation program
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
#ifndef __CURSORUTIL_H__
#define __CURSORUTIL_H__

#include <gdk/gdktypes.h>

typedef enum
{
  GIMP_MOUSE1_CURSOR = (GDK_LAST_CURSOR + 2),
  GIMP_MOUSE1P_CURSOR,
  GIMP_MOUSE1M_CURSOR,
  GIMP_BIGCIRC_CURSOR,
  GIMP_COLOR_PICKER_CURSOR,
  GIMP_LAST_CURSOR_ENTRY
} GimpCursorType;

void change_win_cursor (GdkWindow *, GdkCursorType);
void unset_win_cursor  (GdkWindow *);

void gimp_add_busy_cursors_until_idle (void);
void gimp_add_busy_cursors            (void);
int  gimp_remove_busy_cursors         (gpointer);

#endif /*  __CURSORUTIL_H__  */
