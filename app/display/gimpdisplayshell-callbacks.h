/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_DISPLAY_SHELL_CALLBACKS_H__
#define __GIMP_DISPLAY_SHELL_CALLBACKS_H__


#define GIMP_DISPLAY_SHELL_CANVAS_EVENT_MASK (GDK_EXPOSURE_MASK            | \
                                              GDK_POINTER_MOTION_MASK      | \
                                              GDK_POINTER_MOTION_HINT_MASK | \
                                              GDK_BUTTON_PRESS_MASK        | \
                                              GDK_BUTTON_RELEASE_MASK      | \
                                              GDK_STRUCTURE_MASK           | \
                                              GDK_ENTER_NOTIFY_MASK        | \
                                              GDK_LEAVE_NOTIFY_MASK        | \
                                              GDK_FOCUS_CHANGE_MASK        | \
                                              GDK_KEY_PRESS_MASK           | \
                                              GDK_KEY_RELEASE_MASK         | \
                                              GDK_PROXIMITY_OUT_MASK)


gboolean   gimp_display_shell_events                  (GtkWidget        *widget,
                                                       GdkEvent         *event,
                                                       GimpDisplayShell *shell);

void       gimp_display_shell_canvas_realize          (GtkWidget        *widget,
                                                       GimpDisplayShell *shell);
void       gimp_display_shell_canvas_size_allocate    (GtkWidget        *widget,
                                                       GtkAllocation    *alloc,
                                                       GimpDisplayShell *shell);
gboolean   gimp_display_shell_canvas_expose           (GtkWidget        *widget,
                                                       GdkEventExpose   *eevent,
                                                       GimpDisplayShell *shell);
gboolean   gimp_display_shell_canvas_tool_events      (GtkWidget        *widget,
                                                       GdkEvent         *event,
                                                       GimpDisplayShell *shell);

gboolean   gimp_display_shell_hruler_button_press     (GtkWidget        *widget,
                                                       GdkEventButton   *bevent,
                                                       GimpDisplayShell *shell);
gboolean   gimp_display_shell_vruler_button_press     (GtkWidget        *widget,
                                                       GdkEventButton   *bevent,
                                                       GimpDisplayShell *shell);

gboolean   gimp_display_shell_origin_button_press     (GtkWidget        *widget,
                                                       GdkEventButton   *bevent,
                                                       GimpDisplayShell *shell);

gboolean   gimp_display_shell_quick_mask_button_press (GtkWidget        *widget,
                                                       GdkEventButton   *bevent,
                                                       GimpDisplayShell *shell);
void       gimp_display_shell_quick_mask_toggled      (GtkWidget        *widget,
                                                       GimpDisplayShell *shell);

gboolean   gimp_display_shell_nav_button_press        (GtkWidget        *widget,
                                                       GdkEventButton   *bevent,
                                                       GimpDisplayShell *shell);


#endif /* __GIMP_DISPLAY_SHELL_CALLBACKS_H__ */
