/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmabacktrace-none.c
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


#include "config.h"

#include <gio/gio.h>

#include "ligmabacktrace-backend.h"


#ifdef LIGMA_BACKTRACE_BACKEND_NONE


#include "core-types.h"

#include "ligmabacktrace.h"


/*  public functions  */


void
ligma_backtrace_init (void)
{
}

gboolean
ligma_backtrace_start (void)
{
  return FALSE;
}

void
ligma_backtrace_stop (void)
{
}

LigmaBacktrace *
ligma_backtrace_new (gboolean include_current_thread)
{
  return NULL;
}

void
ligma_backtrace_free (LigmaBacktrace *backtrace)
{
  g_return_if_fail (backtrace == NULL);
}

gint
ligma_backtrace_get_n_threads (LigmaBacktrace *backtrace)
{
  g_return_val_if_reached (0);
}

guintptr
ligma_backtrace_get_thread_id (LigmaBacktrace *backtrace,
                              gint           thread)
{
  g_return_val_if_reached (0);
}

const gchar *
ligma_backtrace_get_thread_name (LigmaBacktrace *backtrace,
                                gint           thread)
{
  g_return_val_if_reached (NULL);
}

gboolean
ligma_backtrace_is_thread_running (LigmaBacktrace *backtrace,
                                  gint           thread)
{
  g_return_val_if_reached (FALSE);
}

gint
ligma_backtrace_find_thread_by_id (LigmaBacktrace *backtrace,
                                  guintptr       thread_id,
                                  gint           thread_hint)
{
  g_return_val_if_reached (-1);
}

gint
ligma_backtrace_get_n_frames (LigmaBacktrace *backtrace,
                             gint           thread)
{
  g_return_val_if_reached (0);
}

guintptr
ligma_backtrace_get_frame_address (LigmaBacktrace *backtrace,
                                  gint           thread,
                                  gint           frame)
{
  g_return_val_if_reached (0);
}

gboolean
ligma_backtrace_get_address_info (guintptr                  address,
                                 LigmaBacktraceAddressInfo *info)
{
  return FALSE;
}


#endif /* LIGMA_BACKTRACE_BACKEND_NONE */
