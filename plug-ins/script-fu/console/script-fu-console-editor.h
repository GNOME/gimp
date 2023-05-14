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

#ifndef __SCRIPT_FU_CONSOLE_EDITOR_H__
#define __SCRIPT_FU_CONSOLE_EDITOR_H__


GtkWidget   *console_editor_new                   (void);
void         console_editor_set_text_and_position (GtkWidget   *self,
                                                   const gchar *text,
                                                   gint         position);

const gchar *console_editor_get_text              (GtkWidget   *self);
gboolean     console_editor_is_empty              (GtkWidget   *self);
void         console_editor_clear                 (GtkWidget   *self);


#endif /*  __SCRIPT_FU_CONSOLE_EDITOR_H__  */
