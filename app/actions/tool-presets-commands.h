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

#ifndef __TOOL_PRESETS_COMMANDS_H__
#define __TOOL_PRESETS_COMMANDS_H__


void   tool_presets_save_cmd_callback    (GimpAction *action,
                                          GVariant   *value,
                                          gpointer    data);
void   tool_presets_restore_cmd_callback (GimpAction *action,
                                          GVariant   *value,
                                          gpointer    data);


#endif /* __TOOL_PRESETS_COMMANDS_H__ */
