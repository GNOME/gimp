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

#pragma once


void       tool_manager_init                       (Gimp             *gimp);
void       tool_manager_exit                       (Gimp             *gimp);

GimpTool * tool_manager_get_active                 (Gimp             *gimp);

void       tool_manager_push_tool                  (Gimp             *gimp,
                                                    GimpTool         *tool);
void       tool_manager_pop_tool                   (Gimp             *gimp);


gboolean   tool_manager_initialize_active          (Gimp             *gimp,
                                                    GimpDisplay      *display);
void       tool_manager_control_active             (Gimp             *gimp,
                                                    GimpToolAction    action,
                                                    GimpDisplay      *display);
void       tool_manager_button_press_active        (Gimp             *gimp,
                                                    const GimpCoords *coords,
                                                    guint32           time,
                                                    GdkModifierType   state,
                                                    GimpButtonPressType press_type,
                                                    GimpDisplay      *display);
void       tool_manager_button_release_active      (Gimp             *gimp,
                                                    const GimpCoords *coords,
                                                    guint32           time,
                                                    GdkModifierType   state,
                                                    GimpDisplay      *display);
void       tool_manager_motion_active              (Gimp             *gimp,
                                                    const GimpCoords *coords,
                                                    guint32           time,
                                                    GdkModifierType   state,
                                                    GimpDisplay      *display);
gboolean   tool_manager_key_press_active           (Gimp             *gimp,
                                                    GdkEventKey      *kevent,
                                                    GimpDisplay      *display);
gboolean   tool_manager_key_release_active         (Gimp             *gimp,
                                                    GdkEventKey      *kevent,
                                                    GimpDisplay      *display);

void       tool_manager_focus_display_active       (Gimp             *gimp,
                                                    GimpDisplay      *display);
void       tool_manager_modifier_state_active      (Gimp             *gimp,
                                                    GdkModifierType   state,
                                                    GimpDisplay      *display);

void     tool_manager_active_modifier_state_active (Gimp             *gimp,
                                                    GdkModifierType   state,
                                                    GimpDisplay      *display);

void       tool_manager_oper_update_active         (Gimp             *gimp,
                                                    const GimpCoords *coords,
                                                    GdkModifierType   state,
                                                    gboolean          proximity,
                                                    GimpDisplay      *display);
void       tool_manager_cursor_update_active       (Gimp             *gimp,
                                                    const GimpCoords *coords,
                                                    GdkModifierType   state,
                                                    GimpDisplay      *display);

const gchar   * tool_manager_can_undo_active       (Gimp             *gimp,
                                                    GimpDisplay      *display);
const gchar   * tool_manager_can_redo_active       (Gimp             *gimp,
                                                    GimpDisplay      *display);
gboolean        tool_manager_undo_active           (Gimp             *gimp,
                                                    GimpDisplay      *display);
gboolean        tool_manager_redo_active           (Gimp             *gimp,
                                                    GimpDisplay      *display);

GimpUIManager * tool_manager_get_popup_active      (Gimp             *gimp,
                                                    const GimpCoords *coords,
                                                    GdkModifierType   state,
                                                    GimpDisplay      *display,
                                                    const gchar     **ui_path);
