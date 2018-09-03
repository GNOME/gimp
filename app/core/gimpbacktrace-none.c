/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpbacktrace-none.c
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

#include "gimpbacktrace-backend.h"


#ifdef GIMP_BACKTRACE_BACKEND_NONE


#include "core-types.h"

#include "gimpbacktrace.h"


/*  public functions  */


void
gimp_backtrace_init (void)
{
}

gboolean
gimp_backtrace_start (void)
{
  return FALSE;
}

void
gimp_backtrace_stop (void)
{
}

GimpBacktrace *
gimp_backtrace_new (gboolean include_current_thread)
{
  return NULL;
}

void
gimp_backtrace_free (GimpBacktrace *backtrace)
{
  g_return_if_fail (backtrace == NULL);
}

gint
gimp_backtrace_get_n_threads (GimpBacktrace *backtrace)
{
  g_return_val_if_reached (0);
}

guintptr
gimp_backtrace_get_thread_id (GimpBacktrace *backtrace,
                              gint           thread)
{
  g_return_val_if_reached (0);
}

const gchar *
gimp_backtrace_get_thread_name (GimpBacktrace *backtrace,
                                gint           thread)
{
  g_return_val_if_reached (NULL);
}

gboolean
gimp_backtrace_is_thread_running (GimpBacktrace *backtrace,
                                  gint           thread)
{
  g_return_val_if_reached (FALSE);
}

gint
gimp_backtrace_find_thread_by_id (GimpBacktrace *backtrace,
                                  guintptr       thread_id,
                                  gint           thread_hint)
{
  g_return_val_if_reached (-1);
}

gint
gimp_backtrace_get_n_frames (GimpBacktrace *backtrace,
                             gint           thread)
{
  g_return_val_if_reached (0);
}

guintptr
gimp_backtrace_get_frame_address (GimpBacktrace *backtrace,
                                  gint           thread,
                                  gint           frame)
{
  g_return_val_if_reached (0);
}

gboolean
gimp_backtrace_get_address_info (guintptr                  address,
                                 GimpBacktraceAddressInfo *info)
{
  return FALSE;
}


#endif /* GIMP_BACKTRACE_BACKEND_NONE */
