/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
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

#ifndef __GIMP_TOOL_CONTROL_H__
#define __GIMP_TOOL_CONTROL_H__

#include "app/core/gimpobject.h"

#define GIMP_TYPE_TOOL_CONTROL            (gimp_tool_control_get_type ())
#define GIMP_TOOL_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_CONTROL, GimpToolControl))
#define GIMP_TOOL_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_CONTROL, GimpToolControlClass))
#define GIMP_IS_TOOL_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_CONTROL))
#define GIMP_IS_TOOL_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_CONTROL))
#define GIMP_TOOL_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_CONTROL, GimpToolControlClass))



GimpToolControl *gimp_tool_control_new  (gboolean           scroll_lock,
			                 gboolean           auto_snap_to,
			                 gboolean           preserve,
			                 gboolean           handle_empty_image,
			                 gboolean           perfectmouse,
			                 /* are all these necessary? */
  				         GdkCursorType      cursor,
  				         GimpToolCursorType tool_cursor,
  				         GimpCursorModifier cursor_modifier,
				         GdkCursorType      toggle_cursor,
  				         GimpToolCursorType toggle_tool_cursor,
  			  	         GimpCursorModifier toggle_cursor_modifier);


void               gimp_tool_control_pause                      (GimpToolControl   *control);
void               gimp_tool_control_resume                     (GimpToolControl   *control);
gboolean           gimp_tool_control_is_paused                  (GimpToolControl   *control);

void               gimp_tool_control_activate                   (GimpToolControl   *control);
void               gimp_tool_control_halt                       (GimpToolControl   *control);
gboolean           gimp_tool_control_is_active                  (GimpToolControl   *control);

void               gimp_tool_control_set_toggle                 (GimpToolControl   *control,
                                                                 gboolean           toggled);
gboolean           gimp_tool_control_is_toggled                 (GimpToolControl   *control);

void               gimp_tool_control_set_preserve               (GimpToolControl   *control,
                                                                 gboolean           preserve);
gboolean           gimp_tool_control_preserve                   (GimpToolControl   *control);

void               gimp_tool_control_set_scroll_lock            (GimpToolControl   *control,
                                                                 gboolean           scroll_lock);
gboolean           gimp_tool_control_scroll_lock                (GimpToolControl   *control);
GimpMotionMode     gimp_tool_control_motion_mode                (GimpToolControl   *control);
gboolean           gimp_tool_control_handles_empty_image        (GimpToolControl   *control);
gboolean           gimp_tool_control_auto_snap_to               (GimpToolControl   *control);

GdkCursorType      gimp_tool_control_get_cursor                 (GimpToolControl    *control);

void               gimp_tool_control_set_cursor                 (GimpToolControl    *control,
                                                                 GdkCursorType       cursor);
GimpToolCursorType gimp_tool_control_get_tool_cursor            (GimpToolControl   *control);

void               gimp_tool_control_set_tool_cursor            (GimpToolControl    *control,
                                                                 GimpToolCursorType  cursor);
GimpCursorModifier gimp_tool_control_get_cursor_modifier        (GimpToolControl    *control);

void               gimp_tool_control_set_cursor_modifier        (GimpToolControl    *control,
                                                                 GimpCursorModifier  cmodifier);
GdkCursorType      gimp_tool_control_get_toggle_cursor          (GimpToolControl    *control);

void               gimp_tool_control_set_toggle_cursor          (GimpToolControl    *control,
                                                                 GdkCursorType       cursor);
GimpToolCursorType gimp_tool_control_get_toggle_tool_cursor     (GimpToolControl    *control);

void               gimp_tool_control_set_toggle_tool_cursor     (GimpToolControl    *control,
                                                                 GimpToolCursorType  cursor);
GimpCursorModifier gimp_tool_control_get_toggle_cursor_modifier (GimpToolControl    *control);

void               gimp_tool_control_set_toggle_cursor_modifier (GimpToolControl    *control,
                                                                 GimpCursorModifier  cmodifier);

                                              

#endif  /*  __GIMP_TOOL_CONTROL_H__  */
