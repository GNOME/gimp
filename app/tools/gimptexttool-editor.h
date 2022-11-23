/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextTool
 * Copyright (C) 2002-2010  Sven Neumann <sven@ligma.org>
 *                          Daniel Eddeland <danedde@svn.gnome.org>
 *                          Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TEXT_TOOL_EDITOR_H__
#define __LIGMA_TEXT_TOOL_EDITOR_H__


void       ligma_text_tool_editor_init             (LigmaTextTool        *text_tool);
void       ligma_text_tool_editor_finalize         (LigmaTextTool        *text_tool);

void       ligma_text_tool_editor_start            (LigmaTextTool        *text_tool);
void       ligma_text_tool_editor_position         (LigmaTextTool        *text_tool);
void       ligma_text_tool_editor_halt             (LigmaTextTool        *text_tool);

void       ligma_text_tool_editor_button_press     (LigmaTextTool        *text_tool,
                                                   gdouble              x,
                                                   gdouble              y,
                                                   LigmaButtonPressType  press_type);
void       ligma_text_tool_editor_button_release   (LigmaTextTool        *text_tool);
void       ligma_text_tool_editor_motion           (LigmaTextTool        *text_tool,
                                                   gdouble              x,
                                                   gdouble              y);
gboolean   ligma_text_tool_editor_key_press        (LigmaTextTool        *text_tool,
                                                   GdkEventKey         *kevent);
gboolean   ligma_text_tool_editor_key_release      (LigmaTextTool        *text_tool,
                                                   GdkEventKey         *kevent);

void       ligma_text_tool_reset_im_context        (LigmaTextTool        *text_tool);
void       ligma_text_tool_abort_im_context        (LigmaTextTool        *text_tool);

void       ligma_text_tool_editor_get_cursor_rect  (LigmaTextTool        *text_tool,
                                                   gboolean             overwrite,
                                                   PangoRectangle      *cursor_rect);
void       ligma_text_tool_editor_update_im_cursor (LigmaTextTool        *text_tool);


#endif /* __LIGMA_TEXT_TOOL_EDITOR_H__ */
