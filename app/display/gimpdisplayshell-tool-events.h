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

#ifndef __GIMP_DISPLAY_SHELL_TOOL_EVENTS_H__
#define __GIMP_DISPLAY_SHELL_TOOL_EVENTS_H__


gboolean   gimp_display_shell_events                  (GtkWidget        *widget,
                                                       GdkEvent         *event,
                                                       GimpDisplayShell *shell);

gboolean   gimp_display_shell_canvas_tool_events      (GtkWidget        *widget,
                                                       GdkEvent         *event,
                                                       GimpDisplayShell *shell);
void       gimp_display_shell_canvas_grab_notify      (GtkWidget        *widget,
                                                       gboolean          was_grabbed,
                                                       GimpDisplayShell *shell);

void       gimp_display_shell_zoom_gesture_begin      (GtkGestureZoom   *gesture,
                                                       GdkEventSequence *sequence,
                                                       GimpDisplayShell *shell);
void       gimp_display_shell_zoom_gesture_update     (GtkGestureZoom   *gesture,
                                                       GdkEventSequence *sequence,
                                                       GimpDisplayShell *shell);
void       gimp_display_shell_zoom_gesture_end        (GtkGestureZoom   *gesture,
                                                       GdkEventSequence *sequence,
                                                       GimpDisplayShell *shell);

void       gimp_display_shell_rotate_gesture_begin    (GtkGestureRotate *gesture,
                                                       GdkEventSequence *sequence,
                                                       GimpDisplayShell *shell);
void       gimp_display_shell_rotate_gesture_update   (GtkGestureRotate *gesture,
                                                       GdkEventSequence *sequence,
                                                       GimpDisplayShell *shell);
void       gimp_display_shell_rotate_gesture_end      (GtkGestureRotate *gesture,
                                                       GdkEventSequence *sequence,
                                                       GimpDisplayShell *shell);

void       gimp_display_shell_buffer_stroke           (GimpMotionBuffer *buffer,
                                                       const GimpCoords *coords,
                                                       guint32           time,
                                                       GdkModifierType   state,
                                                       GimpDisplayShell *shell);
void       gimp_display_shell_buffer_hover            (GimpMotionBuffer *buffer,
                                                       const GimpCoords *coords,
                                                       GdkModifierType   state,
                                                       gboolean          proximity,
                                                       GimpDisplayShell *shell);

gboolean   gimp_display_shell_hruler_button_press     (GtkWidget        *widget,
                                                       GdkEventButton   *bevent,
                                                       GimpDisplayShell *shell);
gboolean   gimp_display_shell_vruler_button_press     (GtkWidget        *widget,
                                                       GdkEventButton   *bevent,
                                                       GimpDisplayShell *shell);


#endif /* __GIMP_DISPLAY_SHELL_TOOL_EVENT_H__ */
