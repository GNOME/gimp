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

#include <glib.h>

#include <gdk/gdktypes.h>
#if defined (GDK_WINDOWING_WIN32) || defined (GDK_WINDOWING_X11)
/* Stopgap measure to detect build with current CVS GTk+ */
#include <gdk/gdkcursor.h>
#endif

#include <gtk/gtk.h>

#include "toolsF.h"


typedef struct
{
  guchar    *bits;
  guchar    *mask_bits;
  gint       width, height;
  gint       x_hot, y_hot;
  GdkBitmap *bitmap;
  GdkBitmap *mask;
  GdkCursor *cursor;
} BitmapCursor;

typedef enum
{
  CURSOR_MODE_TOOL_ICON,
  CURSOR_MODE_TOOL_CROSSHAIR,
  CURSOR_MODE_CROSSHAIR
} CursorMode;

typedef enum
{
  CURSOR_MODIFIER_NONE,
  CURSOR_MODIFIER_PLUS,
  CURSOR_MODIFIER_MINUS,
  CURSOR_MODIFIER_INTERSECT,
  CURSOR_MODIFIER_MOVE,
  CURSOR_MODIFIER_RESIZE,
  CURSOR_MODIFIER_CONTROL,
  CURSOR_MODIFIER_HAND
} CursorModifier;

typedef enum
{
  GIMP_MOUSE_CURSOR = (GDK_LAST_CURSOR + 2),
  GIMP_CROSSHAIR_CURSOR,
  GIMP_CROSSHAIR_SMALL_CURSOR,
  GIMP_BAD_CURSOR,
  GIMP_ZOOM_CURSOR,
  GIMP_COLOR_PICKER_CURSOR,
  GIMP_LAST_CURSOR_ENTRY
} GimpCursorType;

/* FIXME: gimp_busy HACK */
extern gboolean gimp_busy;

void       change_win_cursor                (GdkWindow      *win,
					     GdkCursorType   curtype,
					     ToolType        tool_type,
					     CursorModifier  modifier,
					     gboolean        toggle_cursor);
void       unset_win_cursor                 (GdkWindow      *win);

void       gimp_add_busy_cursors_until_idle (void);
void       gimp_add_busy_cursors            (void);
gint       gimp_remove_busy_cursors         (gpointer        data);

gboolean   gtkutil_compress_motion          (GtkWidget      *widget,
					     gdouble        *lastmotion_x,
					     gdouble        *lastmotion_y);

#endif /*  __CURSORUTIL_H__  */
