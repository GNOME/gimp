/* GIMP - The GNU Image Manipulation Program
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

#ifndef SCHEME_WRAPPER_H
#define SCHEME_WRAPPER_H

typedef enum { TS_OUTPUT_NORMAL, TS_OUTPUT_ERROR } TsOutputType;

typedef void (* ts_output_func)       (TsOutputType    type,
                                       const char     *string,
                                       int             len,
                                       gpointer        data);

void          ts_register_output_func (ts_output_func  func,
                                       gpointer        user_data);

void          ts_stdout_output_func   (TsOutputType    type,
                                       const char     *string,
                                       int             len,
                                       gpointer        user_data);

void          ts_gstring_output_func  (TsOutputType    type,
                                       const char     *string,
                                       int             len,
                                       gpointer        user_data);

void          ts_set_print_flag    (gint);
void          ts_print_welcome     (void);

const gchar * ts_get_success_msg   (void);

void          tinyscheme_init      (const gchar  *path,
                                    gboolean      local_register_scripts);
void          tinyscheme_deinit    (void);

void          ts_output_string     (TsOutputType  type,
                                    const char   *string,
                                    int           len);

void          ts_interpret_stdin   (void);

/* if the return value is 0, success. error otherwise. */
gint          ts_interpret_string  (const gchar *);

#endif /* SCHEME_WRAPPER_H */
