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

#ifndef __SCRIPT_FU_CONSOLE_TOTAL_H__
#define __SCRIPT_FU_CONSOLE_TOTAL_H__


GtkTextBuffer *console_total_history_new           (void);
void           console_total_history_clear         (GtkTextBuffer *self);
gchar         *console_total_history_get_text      (GtkTextBuffer *self);

void           console_total_append_welcome        (GtkTextBuffer *self);
void           console_total_append_text_normal    (GtkTextBuffer *self,
                                                    const gchar   *text,
                                                    gint           len);
void           console_total_append_text_emphasize (GtkTextBuffer *self,
                                                    const gchar   *text,
                                                    gint           len);
void           console_total_append_command        (GtkTextBuffer *self,
                                                    const gchar   *command);


#endif /*  __SCRIPT_FU_CONSOLE_TOTAL_H__  */
