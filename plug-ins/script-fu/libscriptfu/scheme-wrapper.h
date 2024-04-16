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

#ifndef __SCHEME_WRAPPER_H__
#define __SCHEME_WRAPPER_H__

#include "tinyscheme/scheme.h"

typedef void (*TsCallbackFunc) (void);
typedef pointer (*TsWrapperFunc) (scheme*, pointer);


void          tinyscheme_init         (GList        *path,
                                       gboolean      register_scripts);

void          ts_set_run_mode         (GimpRunMode   run_mode);

void          ts_set_print_flag       (gint          print_flag);
void          ts_print_welcome        (void);

const gchar * ts_get_success_msg      (void);
const gchar * ts_get_error_msg        (void);

void          ts_interpret_stdin      (void);

/* if the return value is 0, success. error otherwise. */
gint          ts_interpret_string     (const gchar  *expr);

void          ts_stdout_output_func   (TsOutputType  type,
                                       const char   *string,
                                       int           len,
                                       gpointer      user_data);
void          ts_gstring_output_func  (TsOutputType  type,
                                       const char   *string,
                                       int           len,
                                       gpointer      user_data);
void          ts_register_quit_callback         (TsCallbackFunc callback);
void          ts_register_post_command_callback (TsCallbackFunc callback);

#endif /* __SCHEME_WRAPPER_H__ */
