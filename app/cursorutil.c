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
#include "appenv.h"
#include "cursorutil.h"
#include "gdisplay.h" /* for gdisplay_*_override_cursor() */

extern GSList* display_list; /* It's in gdisplay.c, FYI */

void
change_win_cursor (win, cursortype)
     GdkWindow *win;
     GdkCursorType cursortype;
{
  GdkCursor *cursor;

  cursor = gdk_cursor_new (cursortype);
  gdk_window_set_cursor (win, cursor);
  gdk_cursor_destroy (cursor);
}

void
unset_win_cursor (win)
     GdkWindow *win;
{
  gdk_window_set_cursor (win, NULL);
}
     
void
gimp_add_busy_cursors (void)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  while (list)
    {
      gdisp = (GDisplay *) list->data;
      gdisplay_install_override_cursor(gdisp, GDK_WATCH);

      list = g_slist_next (list);
    }
  gdk_flush();
}
     
void
gimp_remove_busy_cursors (void)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  while (list)
    {
      gdisp = (GDisplay *) list->data;
      gdisplay_remove_override_cursor(gdisp);

      list = g_slist_next (list);
    }
}
