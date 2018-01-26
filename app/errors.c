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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#define _GNU_SOURCE  /* need the POSIX signal API */

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "core/gimp.h"

#include "errors.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#define MAX_TRACES 3

/*  private variables  */

static Gimp                *the_errors_gimp   = NULL;
static gboolean             use_debug_handler = FALSE;
static GimpStackTraceMode   stack_trace_mode  = GIMP_STACK_TRACE_QUERY;
static gchar               *full_prog_name    = NULL;


/*  local function prototypes  */

static void    gimp_third_party_message_log_func (const gchar        *log_domain,
                                                  GLogLevelFlags      flags,
                                                  const gchar        *message,
                                                  gpointer            data);
static void    gimp_message_log_func             (const gchar        *log_domain,
                                                  GLogLevelFlags      flags,
                                                  const gchar        *message,
                                                  gpointer            data);
static void    gimp_error_log_func               (const gchar        *domain,
                                                  GLogLevelFlags      flags,
                                                  const gchar        *message,
                                                  gpointer            data) G_GNUC_NORETURN;

static G_GNUC_NORETURN void  gimp_eek            (const gchar        *reason,
                                                  const gchar        *message,
                                                  gboolean            use_handler);

static gchar * gimp_get_stack_trace              (void);


/*  public functions  */

void
errors_init (Gimp               *gimp,
             const gchar        *_full_prog_name,
             gboolean            _use_debug_handler,
             GimpStackTraceMode  _stack_trace_mode)
{
  const gchar * const log_domains[] =
  {
    "Gimp",
    "Gimp-Actions",
    "Gimp-Base",
    "Gimp-Composite",
    "Gimp-Config",
    "Gimp-Core",
    "Gimp-Dialogs",
    "Gimp-Display",
    "Gimp-File",
    "Gimp-GEGL",
    "Gimp-GUI",
    "Gimp-Menus",
    "Gimp-Operations",
    "Gimp-PDB",
    "Gimp-Paint",
    "Gimp-Paint-Funcs",
    "Gimp-Plug-In",
    "Gimp-Text",
    "Gimp-Tools",
    "Gimp-Vectors",
    "Gimp-Widgets",
    "Gimp-XCF",
    "LibGimpBase",
    "LibGimpColor",
    "LibGimpConfig",
    "LibGimpMath",
    "LibGimpModule",
    "LibGimpThumb",
    "LibGimpWidgets"
  };
  gint i;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (_full_prog_name != NULL);
  g_return_if_fail (full_prog_name == NULL);

#ifdef GIMP_UNSTABLE
  g_printerr ("This is a development version of GIMP.  "
              "Debug messages may appear here.\n\n");
#endif /* GIMP_UNSTABLE */

  the_errors_gimp   = gimp;
  use_debug_handler = _use_debug_handler ? TRUE : FALSE;
  stack_trace_mode  = _stack_trace_mode;
  full_prog_name    = g_strdup (_full_prog_name);

  for (i = 0; i < G_N_ELEMENTS (log_domains); i++)
    g_log_set_handler (log_domains[i],
                       G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_CRITICAL,
                       gimp_message_log_func, gimp);

  g_log_set_handler ("GEGL",
                     G_LOG_LEVEL_MESSAGE,
                     gimp_third_party_message_log_func, gimp);
  g_log_set_handler (NULL,
                     G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL,
                     gimp_error_log_func, gimp);
}

void
errors_exit (void)
{
  the_errors_gimp = NULL;
}

void
gimp_fatal_error (const gchar *message)
{
  gimp_eek ("fatal error", message, TRUE);
}

void
gimp_terminate (const gchar *message)
{
  gimp_eek ("terminated", message, use_debug_handler);
}


/*  private functions  */

static void
gimp_third_party_message_log_func (const gchar    *log_domain,
                                   GLogLevelFlags  flags,
                                   const gchar    *message,
                                   gpointer        data)
{
  Gimp *gimp = data;

  if (gimp)
    {
      /* Whereas all GIMP messages are processed under the same domain,
       * we need to keep the log domain information for third party
       * messages.
       */
      gimp_show_message (gimp, NULL, GIMP_MESSAGE_WARNING,
                         log_domain, message, NULL);
    }
  else
    {
      g_printerr ("%s: %s\n\n", log_domain, message);
    }
}

static void
gimp_message_log_func (const gchar    *log_domain,
                       GLogLevelFlags  flags,
                       const gchar    *message,
                       gpointer        data)
{
  static gint          n_traces;
  GimpMessageSeverity  severity = GIMP_MESSAGE_WARNING;
  Gimp                *gimp = data;
  gchar               *trace = NULL;
  GimpCoreConfig      *config = gimp->config;
  gboolean             generate_backtrace = FALSE;

  g_object_get (G_OBJECT (config),
                "generate-backtrace", &generate_backtrace,
                NULL);

  if (generate_backtrace && (flags & G_LOG_LEVEL_CRITICAL))
    {
      severity = GIMP_MESSAGE_ERROR;

      if (n_traces < MAX_TRACES)
        {
          /* Getting debug traces is time-expensive, and worse, some
           * critical errors have the bad habit to create more errors
           * (the first ones are therefore usually the most useful).
           * This is why we keep track of how many times we made traces
           * and stop doing them after a while.
           * Hence when this happens, critical errors are simply processed as
           * lower level errors.
           */
          trace = gimp_get_stack_trace ();
          n_traces++;
        }
    }

  if (gimp)
    {
      gimp_show_message (gimp, NULL, severity,
                         NULL, message, trace);
    }
  else
    {
      g_printerr ("%s: %s\n\n",
                  gimp_filename_to_utf8 (full_prog_name), message);
      if (trace)
        g_printerr ("Back trace:\n%s\n\n", trace);
    }

  if (trace)
    g_free (trace);
}

static void
gimp_error_log_func (const gchar    *domain,
                     GLogLevelFlags  flags,
                     const gchar    *message,
                     gpointer        data)
{
  gimp_fatal_error (message);
}

static void
gimp_eek (const gchar *reason,
          const gchar *message,
          gboolean     use_handler)
{
  GimpCoreConfig *config = the_errors_gimp->config;
  gboolean        generate_backtrace = FALSE;

  /* GIMP has 2 ways to handle termination signals and fatal errors: one
   * is the stack trace mode which is set at start as command line
   * option --stack-trace-mode, this won't change for the length of the
   * session and outputs a trace in terminal; the other is set in
   * preferences, outputs a trace in a GUI and can change anytime during
   * the session.
   * The GUI backtrace has priority if it is set.
   */
  g_object_get (G_OBJECT (config),
                "generate-backtrace", &generate_backtrace,
                NULL);

#ifndef G_OS_WIN32
  g_printerr ("%s: %s: %s\n", gimp_filename_to_utf8 (full_prog_name),
              reason, message);

  if (use_handler)
    {
#ifndef GIMP_CONSOLE_COMPILATION
      if (generate_backtrace && ! the_errors_gimp->no_interface)
        {
          /* If enabled (it is disabled by default), the GUI preference
           * takes precedence over the command line argument.
           */
          gchar *args[6] = { "gimpdebug-2.0", full_prog_name, NULL,
                             (gchar *) reason, (gchar *) message, NULL };
          gchar  pid[16];
          gint   exit_status;

          g_snprintf (pid, 16, "%u", (guint) getpid ());
          args[2] = pid;

          /* We don't care about any return value. If it fails, too
           * bad, we just won't have any stack trace.
           * We still need to use the sync() variant because we have
           * to keep GIMP up long enough for the debugger to get its
           * trace.
           */
          g_spawn_sync (NULL, args, NULL,
                        G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_STDOUT_TO_DEV_NULL,
                        NULL, NULL, NULL, NULL, &exit_status, NULL);
        }
      else
#endif
        {
          switch (stack_trace_mode)
            {
            case GIMP_STACK_TRACE_NEVER:
              break;

            case GIMP_STACK_TRACE_QUERY:
                {
                  sigset_t sigset;

                  sigemptyset (&sigset);
                  sigprocmask (SIG_SETMASK, &sigset, NULL);

                  if (the_errors_gimp)
                    gimp_gui_ungrab (the_errors_gimp);

                  g_on_error_query (full_prog_name);
                }
              break;

            case GIMP_STACK_TRACE_ALWAYS:
                {
                  sigset_t sigset;

                  sigemptyset (&sigset);
                  sigprocmask (SIG_SETMASK, &sigset, NULL);

                  g_on_error_stack_trace (full_prog_name);
                }
              break;

            default:
              break;
            }
        }
    }
#else

  /* g_on_error_* don't do anything reasonable on Win32. */

  MessageBox (NULL, g_strdup_printf ("%s: %s", reason, message),
              full_prog_name, MB_OK|MB_ICONERROR);

#endif /* ! G_OS_WIN32 */

  exit (EXIT_FAILURE);
}

static gchar *
gimp_get_stack_trace (void)
{
  gchar   *trace  = NULL;
#ifndef G_OS_WIN32
  gchar   *args[7] = { "gdb", "-batch", "-ex", "backtrace full",
                       full_prog_name, NULL, NULL };
  gchar   *gdb_stdout;
  gchar    pid[16];
#endif

  /* Though we should theoretically ask with GIMP_STACK_TRACE_QUERY, we
   * just assume yes right now. TODO: improve this!
   */
  if (stack_trace_mode == GIMP_STACK_TRACE_NEVER)
    return NULL;

  /* This works only on UNIX systems. On Windows, we'll have to find
   * another method, probably with DrMingW.
   */
#ifndef G_OS_WIN32
  g_snprintf (pid, 16, "%u", (guint) getpid ());
  args[5] = pid;

  if (g_spawn_sync (NULL, args, NULL,
                    G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL,
                    NULL, NULL, &gdb_stdout, NULL, NULL, NULL))
    {
      trace = g_strdup (gdb_stdout);
    }
  else if (gdb_stdout)
    {
      g_free (gdb_stdout);
    }

#endif

  return trace;
}
