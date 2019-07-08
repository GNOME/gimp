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

#ifndef __PALETTE_EDITOR_COMMANDS_H__
#define __PALETTE_EDITOR_COMMANDS_H__


void   palette_editor_edit_color_cmd_callback   (GimpAction *action,
                                                 GVariant   *value,
                                                 gpointer    data);
void   palette_editor_new_color_cmd_callback    (GimpAction *action,
                                                 GVariant   *value,
                                                 gpointer    data);
void   palette_editor_delete_color_cmd_callback (GimpAction *action,
                                                 GVariant   *value,
                                                 gpointer    data);

void   palette_editor_zoom_cmd_callback         (GimpAction *action,
                                                 GVariant   *value,
                                                 gpointer    data);


#endif /* __PALETTE_EDITOR_COMMANDS_H__ */
