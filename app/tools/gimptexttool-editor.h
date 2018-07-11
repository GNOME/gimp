/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextTool
 * Copyright (C) 2002-2010  Sven Neumann <sven@gimp.org>
 *                          Daniel Eddeland <danedde@svn.gnome.org>
 *                          Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_TEXT_TOOL_EDITOR_H__
#define __GIMP_TEXT_TOOL_EDITOR_H__


void       gimp_text_tool_editor_init             (GimpTextTool        *text_tool);
void       gimp_text_tool_editor_finalize         (GimpTextTool        *text_tool);

void       gimp_text_tool_editor_start            (GimpTextTool        *text_tool);
void       gimp_text_tool_editor_position         (GimpTextTool        *text_tool);
void       gimp_text_tool_editor_halt             (GimpTextTool        *text_tool);

void       gimp_text_tool_editor_button_press     (GimpTextTool        *text_tool,
                                                   gdouble              x,
                                                   gdouble              y,
                                                   GimpButtonPressType  press_type);
void       gimp_text_tool_editor_button_release   (GimpTextTool        *text_tool);
void       gimp_text_tool_editor_motion           (GimpTextTool        *text_tool,
                                                   gdouble              x,
                                                   gdouble              y);
gboolean   gimp_text_tool_editor_key_press        (GimpTextTool        *text_tool,
                                                   GdkEventKey         *kevent);
gboolean   gimp_text_tool_editor_key_release      (GimpTextTool        *text_tool,
                                                   GdkEventKey         *kevent);

void       gimp_text_tool_reset_im_context        (GimpTextTool        *text_tool);
void       gimp_text_tool_abort_im_context        (GimpTextTool        *text_tool);

void       gimp_text_tool_editor_get_cursor_rect  (GimpTextTool        *text_tool,
                                                   gboolean             overwrite,
                                                   PangoRectangle      *cursor_rect);
void       gimp_text_tool_editor_update_im_cursor (GimpTextTool        *text_tool);


#endif /* __GIMP_TEXT_TOOL_EDITOR_H__ */
