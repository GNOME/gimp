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

#ifndef __ERRORS_H__
#define __ERRORS_H__

#ifndef GIMP_APP_GLUE_COMPILATION
#error You must not #include "errors.h" from an app/ subdir
#endif


void   gimp_errors_init      (Gimp               *gimp,
                              const gchar        *full_prog_name,
                              gboolean            use_debug_handler,
                              GimpStackTraceMode  stack_trace_mode);

void   gimp_message_log_func (const gchar        *log_domain,
                              GLogLevelFlags      flags,
                              const gchar        *message,
                              gpointer            data);
void   gimp_error_log_func   (const gchar        *domain,
                              GLogLevelFlags      flags,
                              const gchar        *message,
                              gpointer            data) G_GNUC_NORETURN;

void   gimp_fatal_error      (const gchar        *format,
                              ...) G_GNUC_PRINTF (1, 2) G_GNUC_NORETURN;
void   gimp_terminate        (const gchar        *format,
                              ...) G_GNUC_PRINTF (1, 2) G_GNUC_NORETURN;


#endif /* __ERRORS_H__ */
