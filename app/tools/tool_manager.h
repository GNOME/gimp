/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __TOOL_MANAGER_H__
#define __TOOL_MANAGER_H__


void       tool_manager_init                       (Ligma             *ligma);
void       tool_manager_exit                       (Ligma             *ligma);

LigmaTool * tool_manager_get_active                 (Ligma             *ligma);

void       tool_manager_push_tool                  (Ligma             *ligma,
                                                    LigmaTool         *tool);
void       tool_manager_pop_tool                   (Ligma             *ligma);


gboolean   tool_manager_initialize_active          (Ligma             *ligma,
                                                    LigmaDisplay      *display);
void       tool_manager_control_active             (Ligma             *ligma,
                                                    LigmaToolAction    action,
                                                    LigmaDisplay      *display);
void       tool_manager_button_press_active        (Ligma             *ligma,
                                                    const LigmaCoords *coords,
                                                    guint32           time,
                                                    GdkModifierType   state,
                                                    LigmaButtonPressType press_type,
                                                    LigmaDisplay      *display);
void       tool_manager_button_release_active      (Ligma             *ligma,
                                                    const LigmaCoords *coords,
                                                    guint32           time,
                                                    GdkModifierType   state,
                                                    LigmaDisplay      *display);
void       tool_manager_motion_active              (Ligma             *ligma,
                                                    const LigmaCoords *coords,
                                                    guint32           time,
                                                    GdkModifierType   state,
                                                    LigmaDisplay      *display);
gboolean   tool_manager_key_press_active           (Ligma             *ligma,
                                                    GdkEventKey      *kevent,
                                                    LigmaDisplay      *display);
gboolean   tool_manager_key_release_active         (Ligma             *ligma,
                                                    GdkEventKey      *kevent,
                                                    LigmaDisplay      *display);

void       tool_manager_focus_display_active       (Ligma             *ligma,
                                                    LigmaDisplay      *display);
void       tool_manager_modifier_state_active      (Ligma             *ligma,
                                                    GdkModifierType   state,
                                                    LigmaDisplay      *display);

void     tool_manager_active_modifier_state_active (Ligma             *ligma,
                                                    GdkModifierType   state,
                                                    LigmaDisplay      *display);

void       tool_manager_oper_update_active         (Ligma             *ligma,
                                                    const LigmaCoords *coords,
                                                    GdkModifierType   state,
                                                    gboolean          proximity,
                                                    LigmaDisplay      *display);
void       tool_manager_cursor_update_active       (Ligma             *ligma,
                                                    const LigmaCoords *coords,
                                                    GdkModifierType   state,
                                                    LigmaDisplay      *display);

const gchar   * tool_manager_can_undo_active       (Ligma             *ligma,
                                                    LigmaDisplay      *display);
const gchar   * tool_manager_can_redo_active       (Ligma             *ligma,
                                                    LigmaDisplay      *display);
gboolean        tool_manager_undo_active           (Ligma             *ligma,
                                                    LigmaDisplay      *display);
gboolean        tool_manager_redo_active           (Ligma             *ligma,
                                                    LigmaDisplay      *display);

LigmaUIManager * tool_manager_get_popup_active      (Ligma             *ligma,
                                                    const LigmaCoords *coords,
                                                    GdkModifierType   state,
                                                    LigmaDisplay      *display,
                                                    const gchar     **ui_path);


#endif  /*  __TOOL_MANAGER_H__  */
