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

#include "config.h"

#include <signal.h>

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "core/gimp.h"

#include "errors.h"
#include "signals.h"

#ifdef G_OS_WIN32
#ifdef HAVE_EXCHNDL
#include <windows.h>
#include <time.h>
#include <exchndl.h>

static LPTOP_LEVEL_EXCEPTION_FILTER g_prevExceptionFilter = NULL;

static LONG WINAPI  gimp_sigfatal_handler (PEXCEPTION_POINTERS pExceptionInfo);
#endif

#else

static void         gimp_sigfatal_handler (gint sig_num) G_GNUC_NORETURN;

#endif


void
gimp_init_signal_handlers (gchar **backtrace_file)
{
  time_t   t;
  gchar   *filename;
  gchar   *dir;
#if defined (G_OS_WIN32) && defined (HAVE_EXCHNDL)
  wchar_t *backtrace_file_utf16;
#endif

#ifdef G_OS_WIN32
  /* This has to be the non-roaming directory (i.e., the local
     directory) as backtraces correspond to the binaries on this
     system. */
  dir = g_build_filename (g_get_user_data_dir (),
                          GIMPDIR, GIMP_USER_VERSION, "CrashLog",
                          NULL);
#else
  dir = g_build_filename (gimp_directory (), "CrashLog", NULL);
#endif

  time (&t);
  filename = g_strdup_printf ("%s-crash-%" G_GUINT64_FORMAT ".txt",
                              PACKAGE_NAME, (guint64) t);
  *backtrace_file = g_build_filename (dir, filename, NULL);
  g_free (filename);
  g_free (dir);

#ifdef G_OS_WIN32
  /* Use Dr. Mingw (dumps backtrace on crash) if it is available. Do
   * nothing otherwise on Win32.
   * The user won't get any stack trace from glib anyhow.
   * Without Dr. MinGW, It's better to let Windows inform about the
   * program error, and offer debugging (if the user has installed MSVC
   * or some other compiler that knows how to install itself as a
   * handler for program errors).
   */

#ifdef HAVE_EXCHNDL
  /* Order is very important here. We need to add our signal handler
   * first, then run ExcHndlInit() which will add its own handler, so
   * that ExcHnl's handler runs first since that's in FILO order.
   */
  if (! g_prevExceptionFilter)
    g_prevExceptionFilter = SetUnhandledExceptionFilter (gimp_sigfatal_handler);

  ExcHndlInit ();

  if ((backtrace_file_utf16 = g_utf8_to_utf16 (*backtrace_file, -1, NULL, NULL, NULL)))
    {
      ExcHndlSetLogFileNameW (backtrace_file_utf16);
      g_free (backtrace_file_utf16);
    }

#endif /* HAVE_EXCHNDL */

#else

  /* Handle fatal signals */

  /* these are handled by gimp_terminate() */
  gimp_signal_private (SIGHUP,  gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGINT,  gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGQUIT, gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGTERM, gimp_sigfatal_handler, 0);

  /* these are handled by gimp_fatal_error() */
  /*
   * MacOS has it's own crash handlers which end up fighting the
   * these Gimp supplied handlers and leading to very hard to
   * deal with hangs (just get a spin dump)
   */
#ifndef PLATFORM_OSX
  gimp_signal_private (SIGABRT, gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGBUS,  gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGSEGV, gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGFPE,  gimp_sigfatal_handler, 0);
#endif

  /* Ignore SIGPIPE because plug_in.c handles broken pipes */
  gimp_signal_private (SIGPIPE, SIG_IGN, 0);

  /* Restart syscalls on SIGCHLD */
  gimp_signal_private (SIGCHLD, SIG_DFL, SA_RESTART);

#endif /* G_OS_WIN32 */
}


#ifdef G_OS_WIN32

#ifdef HAVE_EXCHNDL
static LONG WINAPI
gimp_sigfatal_handler (PEXCEPTION_POINTERS pExceptionInfo)
{
  EXCEPTION_RECORD *er;
  int               fatal;

  if (pExceptionInfo == NULL ||
      pExceptionInfo->ExceptionRecord == NULL)
    return EXCEPTION_CONTINUE_SEARCH;

  er = pExceptionInfo->ExceptionRecord;
  fatal = I_RpcExceptionFilter (er->ExceptionCode);

  /* IREF() returns EXCEPTION_CONTINUE_SEARCH for fatal exceptions */
  if (fatal == EXCEPTION_CONTINUE_SEARCH)
    {
      /* Just in case, so that we don't loop or anything similar, just
       * re-establish previous handler.
       */
      SetUnhandledExceptionFilter (g_prevExceptionFilter);

      /* Now process the exception. */
      gimp_fatal_error ("unhandled exception");
    }

  if (g_prevExceptionFilter && g_prevExceptionFilter != gimp_sigfatal_handler)
    return g_prevExceptionFilter (pExceptionInfo);
  else
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

#else

/* gimp core signal handler for fatal signals */

static void
gimp_sigfatal_handler (gint sig_num)
{
  switch (sig_num)
    {
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
      gimp_terminate (g_strsignal (sig_num));
      break;

    case SIGABRT:
    case SIGBUS:
    case SIGSEGV:
    case SIGFPE:
    default:
      gimp_fatal_error (g_strsignal (sig_num));
      break;
    }
}

#endif /* G_OS_WIN32 */
