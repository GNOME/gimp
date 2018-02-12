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

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gio/gio.h>
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "core/gimp.h"

#include "errors.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

/*  private variables  */

static Gimp                *the_errors_gimp   = NULL;
static gboolean             use_debug_handler = FALSE;
static GimpStackTraceMode   stack_trace_mode  = GIMP_STACK_TRACE_QUERY;
static gchar               *full_prog_name    = NULL;
static gchar               *backtrace_file    = NULL;


/*  local function prototypes  */

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


/*  public functions  */

void
errors_init (Gimp               *gimp,
             const gchar        *_full_prog_name,
             gboolean            _use_debug_handler,
             GimpStackTraceMode  _stack_trace_mode,
             const gchar        *_backtrace_file)
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
  backtrace_file    = g_strdup (_backtrace_file);

  for (i = 0; i < G_N_ELEMENTS (log_domains); i++)
    g_log_set_handler (log_domains[i],
                       G_LOG_LEVEL_WARNING |
                       G_LOG_LEVEL_MESSAGE |
                       G_LOG_LEVEL_CRITICAL,
                       gimp_message_log_func, gimp);

  g_log_set_handler ("GEGL",
                     G_LOG_LEVEL_WARNING |
                     G_LOG_LEVEL_MESSAGE |
                     G_LOG_LEVEL_CRITICAL,
                     gimp_message_log_func, gimp);
  g_log_set_handler (NULL,
                     G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL,
                     gimp_error_log_func, gimp);
}

void
errors_exit (void)
{
  the_errors_gimp = NULL;

  if (backtrace_file)
    g_free (backtrace_file);
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
gimp_message_log_func (const gchar    *log_domain,
                       GLogLevelFlags  flags,
                       const gchar    *message,
                       gpointer        data)
{
  Gimp                *gimp       = data;
  const gchar         *msg_domain = NULL;
  GimpMessageSeverity  severity   = GIMP_MESSAGE_WARNING;

  /* All GIMP messages are processed under the same domain, but
   * we need to keep the log domain information for third party
   * messages.
   */
  if (! g_str_has_prefix (log_domain, "Gimp") &&
      ! g_str_has_prefix (log_domain, "LibGimp"))
    msg_domain = log_domain;

  if (flags & (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING))
    {
      GimpCoreConfig  *config = gimp->config;
      GimpDebugPolicy  debug_policy;

      g_object_get (G_OBJECT (config),
                    "debug-policy", &debug_policy,
                    NULL);

      if ((debug_policy == GIMP_DEBUG_POLICY_CRITICAL &&
           (flags & G_LOG_LEVEL_CRITICAL)) ||
          debug_policy == GIMP_DEBUG_POLICY_WARNING)
        {
          severity = GIMP_MESSAGE_ERROR;
        }
    }

  if (gimp)
    {
      gimp_show_message (gimp, NULL, severity, msg_domain, message);
    }
  else
    {
      const gchar *reason;

      switch (flags & G_LOG_LEVEL_MASK)
        {
        case G_LOG_LEVEL_WARNING:
          reason = "WARNING";
          break;
        case G_LOG_LEVEL_CRITICAL:
          reason = "CRITICAL";
          break;
        default:
          reason = "MESSAGE";
          break;
        }

      g_printerr ("%s: %s-%s: %s\n",
                  gimp_filename_to_utf8 (full_prog_name),
                  log_domain, reason, message);
    }
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
  GimpCoreConfig *config             = the_errors_gimp->config;
  GimpDebugPolicy debug_policy;
  gboolean        eek_handled        = FALSE;

  /* GIMP has 2 ways to handle termination signals and fatal errors: one
   * is the stack trace mode which is set at start as command line
   * option --stack-trace-mode, this won't change for the length of the
   * session and outputs a trace in terminal; the other is set in
   * preferences, outputs a trace in a GUI and can change anytime during
   * the session.
   * The GUI backtrace has priority if it is set.
   */
  g_object_get (G_OBJECT (config),
                "debug-policy", &debug_policy,
                NULL);

  /* Let's just always output on stdout at least so that there is a
   * trace if the rest fails. */
  g_printerr ("%s: %s: %s\n", gimp_filename_to_utf8 (full_prog_name),
              reason, message);

#if ! defined (G_OS_WIN32) || defined (HAVE_EXCHNDL)

  if (use_handler)
    {
#ifndef GIMP_CONSOLE_COMPILATION
      if (debug_policy != GIMP_DEBUG_POLICY_NEVER &&
          ! the_errors_gimp->no_interface)
        {
          FILE     *fd;
          gboolean  has_backtrace = TRUE;

          /* If GUI backtrace enabled (it is disabled by default), it
           * takes precedence over the command line argument.
           */
#ifdef G_OS_WIN32
          const gchar *gimpdebug = "gimp-debug-tool-" GIMP_TOOL_VERSION ".exe";
#elif defined (PLATFORM_OSX)
          const gchar *gimpdebug = "gimp-debug-tool-" GIMP_TOOL_VERSION;
#else
          const gchar *gimpdebug = LIBEXECDIR "/gimp-debug-tool-" GIMP_TOOL_VERSION;
#endif
          gchar *args[7] = { (gchar *) gimpdebug, full_prog_name, NULL,
                             (gchar *) reason, (gchar *) message,
                             backtrace_file, NULL };
          gchar  pid[16];

          g_snprintf (pid, 16, "%u", (guint) getpid ());
          args[2] = pid;

#ifndef G_OS_WIN32
          /* On Win32, the trace has already been processed by ExcHnl
           * and is waiting for us in a text file.
           */
          fd = g_fopen (backtrace_file, "w");
          has_backtrace = gimp_print_stack_trace ((const gchar *) full_prog_name,
                                                  fd, NULL);
          fclose (fd);
#endif

          /* We don't care about any return value. If it fails, too
           * bad, we just won't have any stack trace.
           * We still need to use the sync() variant because we have
           * to keep GIMP up long enough for the debugger to get its
           * trace.
           */
          if (has_backtrace &&
              g_file_test (backtrace_file, G_FILE_TEST_IS_REGULAR) &&
              g_spawn_async (NULL, args, NULL,
                             G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_STDOUT_TO_DEV_NULL,
                             NULL, NULL, NULL, NULL))
            eek_handled = TRUE;
        }
#endif /* !GIMP_CONSOLE_COMPILATION */

#ifndef G_OS_WIN32
      if (! eek_handled)
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

                  gimp_on_error_query ((const gchar *) full_prog_name);
                }
              break;

            case GIMP_STACK_TRACE_ALWAYS:
                {
                  sigset_t sigset;

                  sigemptyset (&sigset);
                  sigprocmask (SIG_SETMASK, &sigset, NULL);

                  gimp_print_stack_trace ((const gchar *) full_prog_name,
                                          stdout, NULL);
                }
              break;

            default:
              break;
            }
        }
#endif /* ! G_OS_WIN32 */
    }
#endif /* ! G_OS_WIN32 || HAVE_EXCHNDL */

#if defined (G_OS_WIN32) && ! defined (GIMP_CONSOLE_COMPILATION)
  /* g_on_error_* don't do anything reasonable on Win32. */
  if (! eek_handled && ! the_errors_gimp->no_interface)
    MessageBox (NULL, g_strdup_printf ("%s: %s", reason, message),
                full_prog_name, MB_OK|MB_ICONERROR);
#endif

  exit (EXIT_FAILURE);
}
