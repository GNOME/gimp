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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __DISP_CALLBACKS_H__
#define __DISP_CALLBACKS_H__

#define CANVAS_EVENT_MASK  GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | \
                           GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_PRESS_MASK | \
			   GDK_BUTTON_RELEASE_MASK | GDK_STRUCTURE_MASK | \
			   GDK_ENTER_NOTIFY_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK

gint gdisplay_canvas_events (GtkWidget *, GdkEvent *);
gint gdisplay_hruler_button_press (GtkWidget *, GdkEventButton *, gpointer);
gint gdisplay_vruler_button_press (GtkWidget *, GdkEventButton *, gpointer);


#endif /*  __DISP_CALLBACKS_H__  */
