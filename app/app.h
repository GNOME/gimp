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

#pragma once

#ifndef GIMP_APP_GLUE_COMPILATION
#error You must not #include "app.h" from a subdir
#endif


void  app_libs_init (GOptionContext      *context,
                     gboolean             no_interface);
void  app_abort     (gboolean             no_interface,
                     const gchar         *abort_message) G_GNUC_NORETURN;
void  app_exit      (gint                 status) G_GNUC_NORETURN;

gint  app_run       (const gchar         *full_prog_name,
                     const gchar        **filenames,
                     GFile               *alternate_system_gimprc,
                     GFile               *alternate_gimprc,
                     const gchar         *session_name,
                     const gchar         *batch_interpreter,
                     const gchar        **batch_commands,
                     gboolean             quit,
                     gboolean             as_new,
                     gboolean             no_interface,
                     gboolean             no_data,
                     gboolean             no_fonts,
                     gboolean             no_splash,
                     gboolean             be_verbose,
                     gboolean             use_shm,
                     gboolean             use_cpu_accel,
                     gboolean             console_messages,
                     gboolean             use_debug_handler,
                     gboolean             show_playground,
                     gboolean             show_debug_menu,
                     GimpStackTraceMode   stack_trace_mode,
                     GimpPDBCompatMode    pdb_compat_mode,
                     const gchar         *backtrace_file);
