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

  if (flags & G_LOG_LEVEL_CRITICAL)
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
#ifndef G_OS_WIN32
  g_printerr ("%s: %s: %s\n", gimp_filename_to_utf8 (full_prog_name),
              reason, message);

  if (use_handler)
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
#if defined(G_OS_UNIX)
  GString *gtrace = NULL;
  gchar    buffer[256];
  ssize_t  read_n;
  pid_t    pid;
  int      status;
  int      out_fd[2];
#endif

  /* Though we should theoretically ask with GIMP_STACK_TRACE_QUERY, we
   * just assume yes right now. TODO: improve this!
   */
  if (stack_trace_mode == GIMP_STACK_TRACE_NEVER)
    return NULL;

  /* This works only on UNIX systems. On Windows, we'll have to find
   * another method, probably with DrMingW.
   */
#if defined(G_OS_UNIX)
  if (pipe (out_fd) == -1)
    {
      return NULL;
    }

  /* This is a trick to get the stack trace inside a string.
   * GLib's g_on_error_stack_trace() unfortunately writes directly to
   * the standard output, which is a very unfortunate implementation.
   */
  pid = fork ();
  if (pid == 0)
    {
      /* Child process. */

      /* XXX I just don't understand why, but somehow the parent process
       * doesn't get the output if I don't print something first. I just
       * leave this very dirty hack until I figure out what's going on.
       */
      printf(" ");

      /* Redirect the debugger output. */
      dup2 (out_fd[1], STDOUT_FILENO);
      close (out_fd[0]);
      close (out_fd[1]);
      g_on_error_stack_trace (full_prog_name);
      _exit (0);
    }
  else if (pid > 0)
    {
      /* Main process. */
      waitpid (pid, &status, 0);
    }
  else if (pid == (pid_t) -1)
    {
      /* No trace can be done. */
      return NULL;
    }

  gtrace = g_string_new ("");

  /* It is important to close the writing side of the pipe, otherwise
   * the read() will wait forever without getting the information that
   * writing is finished.
   */
  close (out_fd[1]);

  while ((read_n = read (out_fd[0], buffer, 256)) > 0)
    {
      g_string_append_len (gtrace, buffer, read_n);
    }
  close (out_fd[0]);

  if (gtrace)
    trace = g_string_free (gtrace, FALSE);
  if (trace && strlen (g_strstrip (trace)) == 0)
    {
      /* Empty strings are the same as no strings. */
      g_free (trace);
      trace = NULL;
    }
#endif

  return trace;
}
