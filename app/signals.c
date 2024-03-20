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
#include "libgimpbase/gimpbase-private.h"

#include "core/core-types.h"

#include "core/gimp.h"

#include "errors.h"
#include "signals.h"

static void         gimp_sigfatal_handler (gint sig_num) G_GNUC_NORETURN;


void
gimp_init_signal_handlers (gchar **backtrace_file)
{
  time_t   t;
#if defined(G_OS_WIN32) && defined(ENABLE_RELOCATABLE_RESOURCES)
  gchar   *bin_dir;
  size_t   codeview_path_len;
  gchar   *codeview_path;
#endif
  gchar   *filename;
  gchar   *dir;

#ifdef G_OS_WIN32
#ifdef ENABLE_RELOCATABLE_RESOURCES
  /* FIXME: https://github.com/jrfonseca/drmingw/issues/91 */
  bin_dir = g_build_filename (gimp_installation_directory (), "bin", NULL);
  codeview_path_len = strlen (g_getenv ("_NT_SYMBOL_PATH") ? g_getenv ("_NT_SYMBOL_PATH") : "") + strlen (bin_dir) + 2;
  codeview_path = g_try_malloc (codeview_path_len);
  if (codeview_path == NULL)
    {
      g_warning ("Failed to allocate memory");
    }
  if (g_getenv ("_NT_SYMBOL_PATH"))
    g_snprintf (codeview_path, codeview_path_len, "%s;%s", bin_dir, g_getenv ("_NT_SYMBOL_PATH"));
  else
    g_snprintf (codeview_path, codeview_path_len, "%s", bin_dir);
  g_setenv ("_NT_SYMBOL_PATH", codeview_path, TRUE);
  g_free (codeview_path);
  g_free (bin_dir);
#endif

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


#ifndef G_OS_WIN32

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

#else
  signal (SIGTERM, gimp_sigfatal_handler);

  signal (SIGABRT, gimp_sigfatal_handler);
  signal (SIGSEGV, gimp_sigfatal_handler);
  signal (SIGFPE, gimp_sigfatal_handler);
#endif /* ! G_OS_WIN32 */
}



/* gimp core signal handler for fatal signals */

static void
gimp_sigfatal_handler (gint sig_num)
{
  switch (sig_num)
    {
#ifndef G_OS_WIN32
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
#endif
    case SIGTERM:
      gimp_terminate (g_strsignal (sig_num));
      break;

    case SIGABRT:
    case SIGSEGV:
    case SIGFPE:
#ifndef G_OS_WIN32
    case SIGBUS:
#endif
    default:
      gimp_fatal_error (g_strsignal (sig_num));
      break;
    }
}
