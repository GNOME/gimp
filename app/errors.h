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

#ifndef __ERRORS_H__
#define __ERRORS_H__

#ifndef GIMP_APP_GLUE_COMPILATION
#error You must not #include "errors.h" from an app/ subdir
#endif


void    errors_init      (Gimp               *gimp,
                          const gchar        *full_prog_name,
                          gboolean            use_debug_handler,
                          GimpStackTraceMode  stack_trace_mode,
                          const gchar        *backtrace_file);
void    errors_exit      (void);

GList * errors_recovered (void);

void    gimp_fatal_error (const gchar        *message) G_GNUC_NORETURN;
void    gimp_terminate   (const gchar        *message) G_GNUC_NORETURN;


#endif /* __ERRORS_H__ */
