/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef SIOD_WRAPPER_H
#define SIOD_WRAPPER_H


void          siod_init              (gboolean register_scripts);

FILE        * siod_get_output_file   (void);
void          siod_set_output_file   (FILE *file);

void          siod_set_console_mode  (int flag);

gint          siod_get_verbose_level (void);
void          siod_set_verbose_level (gint verbose_level);

void          siod_print_welcome     (void);

const gchar * siod_get_error_msg     (void);
const gchar * siod_get_success_msg   (void);

void          siod_output_string     (FILE        *fp,
                                      const gchar *format,
                                      ...) G_GNUC_PRINTF (2, 3);

/* if the return value is 0, success. error otherwise. */
gint          siod_interpret_string  (const gchar *expr);


#endif /* SIOD_WRAPPER_H */
