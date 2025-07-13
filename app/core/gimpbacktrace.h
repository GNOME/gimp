/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbacktrace.h
 * Copyright (C) 2018 Ell
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


typedef struct _GimpBacktraceAddressInfo GimpBacktraceAddressInfo;


struct _GimpBacktraceAddressInfo
{
  gchar    object_name[256];

  gchar    symbol_name[256];
  guintptr symbol_address;

  gchar    source_file[256];
  gint     source_line;
};


void            gimp_backtrace_init              (void);

gboolean        gimp_backtrace_start             (void);
void            gimp_backtrace_stop              (void);

GimpBacktrace * gimp_backtrace_new               (gboolean                 include_current_thread);
void            gimp_backtrace_free              (GimpBacktrace           *backtrace);

gint            gimp_backtrace_get_n_threads     (GimpBacktrace           *backtrace);
guintptr        gimp_backtrace_get_thread_id     (GimpBacktrace           *backtrace,
                                                  gint                     thread);
const gchar   * gimp_backtrace_get_thread_name   (GimpBacktrace           *backtrace,
                                                  gint                     thread);
gboolean        gimp_backtrace_is_thread_running (GimpBacktrace           *backtrace,
                                                  gint                     thread);

gint            gimp_backtrace_find_thread_by_id (GimpBacktrace           *backtrace,
                                                  guintptr                 thread_id,
                                                  gint                     thread_hint);

gint            gimp_backtrace_get_n_frames      (GimpBacktrace           *backtrace,
                                                  gint                     thread);
guintptr        gimp_backtrace_get_frame_address (GimpBacktrace           *backtrace,
                                                  gint                     thread,
                                                  gint                     frame);

gboolean        gimp_backtrace_get_address_info  (guintptr                  address,
                                                  GimpBacktraceAddressInfo *info);
