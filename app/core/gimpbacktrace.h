/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabacktrace.h
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

#ifndef __LIGMA_BACKTRACE_H__
#define __LIGMA_BACKTRACE_H__


typedef struct _LigmaBacktraceAddressInfo LigmaBacktraceAddressInfo;


struct _LigmaBacktraceAddressInfo
{
  gchar    object_name[256];

  gchar    symbol_name[256];
  guintptr symbol_address;

  gchar    source_file[256];
  gint     source_line;
};


void            ligma_backtrace_init              (void);

gboolean        ligma_backtrace_start             (void);
void            ligma_backtrace_stop              (void);

LigmaBacktrace * ligma_backtrace_new               (gboolean                 include_current_thread);
void            ligma_backtrace_free              (LigmaBacktrace           *backtrace);

gint            ligma_backtrace_get_n_threads     (LigmaBacktrace           *backtrace);
guintptr        ligma_backtrace_get_thread_id     (LigmaBacktrace           *backtrace,
                                                  gint                     thread);
const gchar   * ligma_backtrace_get_thread_name   (LigmaBacktrace           *backtrace,
                                                  gint                     thread);
gboolean        ligma_backtrace_is_thread_running (LigmaBacktrace           *backtrace,
                                                  gint                     thread);

gint            ligma_backtrace_find_thread_by_id (LigmaBacktrace           *backtrace,
                                                  guintptr                 thread_id,
                                                  gint                     thread_hint);

gint            ligma_backtrace_get_n_frames      (LigmaBacktrace           *backtrace,
                                                  gint                     thread);
guintptr        ligma_backtrace_get_frame_address (LigmaBacktrace           *backtrace,
                                                  gint                     thread,
                                                  gint                     frame);

gboolean        ligma_backtrace_get_address_info  (guintptr                  address,
                                                  LigmaBacktraceAddressInfo *info);


#endif  /*  __LIGMA_BACKTRACE_H__  */
