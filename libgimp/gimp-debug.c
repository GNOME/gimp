/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimp-debug.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <sys/types.h>

#include <glib.h>

#ifndef G_OS_WIN32
#include "libgimpbase/gimpsignal.h"

#else

#ifdef HAVE_EXCHNDL
#include <time.h>
#include <exchndl.h>
#endif

#include <signal.h>
#endif

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
#  ifdef STRICT
#  undef STRICT
#  endif
#  define STRICT

#  ifdef _WIN32_WINNT
#  undef _WIN32_WINNT
#  endif
#  define _WIN32_WINNT 0x0601

#  include <windows.h>
#  include <tlhelp32.h>
#  include <processthreadsapi.h>
#  undef RGB
#endif

#include "gimp.h"
#include "gimp-debug.h"


static const GDebugKey gimp_debug_keys[] =
{
  { "pid",            GIMP_DEBUG_PID            },
  { "fatal-warnings", GIMP_DEBUG_FATAL_WARNINGS },
  { "fw",             GIMP_DEBUG_FATAL_WARNINGS },
  { "query",          GIMP_DEBUG_QUERY          },
  { "init",           GIMP_DEBUG_INIT           },
  { "run",            GIMP_DEBUG_RUN            },
  { "quit",           GIMP_DEBUG_QUIT           },
  { "on",             GIMP_DEBUG_DEFAULT        }
};

static guint          gimp_debug_flags   = 0;


void
_gimp_debug_init (const gchar *basename)
{
  const gchar *env_string = g_getenv ("GIMP_PLUGIN_DEBUG");

  if (env_string)
    {
      gchar       *debug_string;
      const gchar *debug_messages;

      debug_string = strchr (env_string, ',');

      if (debug_string)
        {
          gint len = debug_string - env_string;

          if ((strlen (basename) == len) &&
              (strncmp (basename, env_string, len) == 0))
            {
              gimp_debug_flags =
                g_parse_debug_string (debug_string + 1,
                                      gimp_debug_keys,
                                      G_N_ELEMENTS (gimp_debug_keys));
            }
        }
      else if (strcmp (env_string, basename) == 0)
        {
          gimp_debug_flags = GIMP_DEBUG_DEFAULT;
        }

      /*  make debug output visible by setting G_MESSAGES_DEBUG  */
      debug_messages = g_getenv ("G_MESSAGES_DEBUG");

      if (debug_messages)
        {
          gchar *tmp = g_strconcat (debug_messages, ",LibGimp", NULL);
          g_setenv ("G_MESSAGES_DEBUG", tmp, TRUE);
          g_free (tmp);
        }
      else
        {
          g_setenv ("G_MESSAGES_DEBUG", "LibGimp", TRUE);
        }
    }
}

guint
_gimp_debug_flags (void)
{
  return gimp_debug_flags;
}

void
_gimp_debug_stop (void)
{
#ifndef G_OS_WIN32

  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Waiting for debugger...");
  raise (SIGSTOP);

#else

  HANDLE        hThreadSnap = NULL;
  THREADENTRY32 te32        = { 0 };
  DWORD         opid        = GetCurrentProcessId();

  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
         "Debugging (restart externally): %ld",
         (long int) opid);

  hThreadSnap = CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0);
  if (hThreadSnap == INVALID_HANDLE_VALUE)
    {
      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
             "error getting threadsnap - debugging impossible");
      return;
    }

  te32.dwSize = sizeof (THREADENTRY32);

  if (Thread32First (hThreadSnap, &te32))
    {
      do
        {
          if (te32.th32OwnerProcessID == opid)
            {
              HANDLE hThread = OpenThread (THREAD_SUSPEND_RESUME, FALSE,
                                           te32.th32ThreadID);
              SuspendThread (hThread);
              CloseHandle (hThread);
            }
        }
      while (Thread32Next (hThreadSnap, &te32));
    }
  else
    {
      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "error getting threads");
    }

  CloseHandle (hThreadSnap);

#endif
}
