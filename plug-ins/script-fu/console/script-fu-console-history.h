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

#ifndef __SCRIPT_FU_CONSOLE_HISTORY_H__
#define __SCRIPT_FU_CONSOLE_HISTORY_H__


typedef struct
{
  GList         *model;
  gint           model_len;
  gint           model_cursor;
  gint           model_max;
} CommandHistory;


void         console_history_init              (CommandHistory *self);

void         console_history_new_tail          (CommandHistory *self);
void         console_history_set_tail          (CommandHistory *self,
                                                const gchar    *command);

void         console_history_move_cursor       (CommandHistory *self,
                                                gint            direction);
void         console_history_cursor_to_tail    (CommandHistory *self);
gboolean     console_history_is_cursor_at_tail (CommandHistory *self);
const gchar *console_history_get_at_cursor     (CommandHistory *self);

GStrv        console_history_from_settings     (CommandHistory       *self,
                                                GimpProcedureConfig  *config);
void         console_history_to_settings       (CommandHistory       *self,
                                                GimpProcedureConfig  *config);


#endif /*  __SCRIPT_FU_CONSOLE_HISTORY_H__  */
