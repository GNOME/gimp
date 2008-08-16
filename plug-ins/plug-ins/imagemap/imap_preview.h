/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2003 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 *
 */

#ifndef _IMAP_PREVIEW_H
#define _IMAP_PREVIEW_H

#include <libgimp/gimp.h>

typedef struct {
   GimpDrawable *drawable;
   GtkWidget	*window;
   GtkWidget	*preview;
   GtkWidget	*hruler;
   GtkWidget	*vruler;
   gint		width;
   gint		height;
   gint         widget_width;
   gint         widget_height;
   GimpPixelRgn src_rgn;

   GdkCursorType cursor;
} Preview_t;

Preview_t *make_preview(GimpDrawable *drawable);
void preview_redraw(Preview_t *preview);

void add_preview_motion_event(Preview_t *preview, GCallback func);
void add_enter_notify_event(Preview_t *preview, GCallback func);
void add_leave_notify_event(Preview_t *preview, GCallback func);
void add_preview_button_press_event(Preview_t *preview, GCallback func);

gint preview_get_width(GtkWidget *preview);
gint preview_get_height(GtkWidget *preview);

void preview_zoom(Preview_t *preview, gint zoom_factor);
GdkCursorType preview_set_cursor(Preview_t *preview,
				 GdkCursorType cursor_type);

#endif /* _IMAP_PREVIEW_H */
