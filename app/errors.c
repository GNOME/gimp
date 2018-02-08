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

#include <sys/wait.h>

#ifdef HAVE_EXECINFO_H
/* Allowing backtrace() API. */
#include <execinfo.h>
#endif

#include <gio/gio.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

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
static gchar               *backtrace_file    = NULL;


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

static gboolean  gimp_print_stack_trace          (FILE               *file,
                                                  gchar            **trace);
static void      gimp_on_error_query             (void);


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
#ifdef GIMP_UNSTABLE
                       G_LOG_LEVEL_WARNING |
#endif
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

  if (generate_backtrace &&
      (flags & (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING)))
    {
      severity = (flags & G_LOG_LEVEL_CRITICAL) ?
                 GIMP_MESSAGE_ERROR : GIMP_MESSAGE_WARNING;

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
          gimp_print_stack_trace (NULL, &trace);
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
  GimpCoreConfig *config             = the_errors_gimp->config;
  gboolean        generate_backtrace = FALSE;
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
                "generate-backtrace", &generate_backtrace,
                NULL);

  /* Let's just always output on stdout at least so that there is a
   * trace if the rest fails. */
  g_printerr ("%s: %s: %s\n", gimp_filename_to_utf8 (full_prog_name),
              reason, message);

#if ! defined (G_OS_WIN32) || defined (HAVE_EXCHNDL)

  if (use_handler)
    {
#ifndef GIMP_CONSOLE_COMPILATION
      if (generate_backtrace && ! the_errors_gimp->no_interface)
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
          has_backtrace = gimp_print_stack_trace (fd, NULL);
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

                  gimp_on_error_query ();
                }
              break;

            case GIMP_STACK_TRACE_ALWAYS:
                {
                  sigset_t sigset;

                  sigemptyset (&sigset);
                  sigprocmask (SIG_SETMASK, &sigset, NULL);

                  gimp_print_stack_trace (stdout, NULL);
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

/* In some error cases (e.g. segmentation fault), trying to allocate
 * more memory will trigger more segmentation faults and therefore loop
 * our error handling (which is just wrong). Therefore printing to a
 * file description is an implementation without any memory allocation.
 * On the other hand, if trace is not #NULL, a newly-allocated string
 * will be returned.
 */
static gboolean
gimp_print_stack_trace (FILE   *file,
                        gchar **trace)
{
  gboolean stack_printed = FALSE;

  /* This works only on UNIX systems. */
#ifndef G_OS_WIN32
  GString *gtrace = NULL;
  gchar    gimp_pid[16];
  pid_t    pid;
  gchar    buffer[256];
  ssize_t  read_n;
  int      out_fd[2];

  g_snprintf (gimp_pid, 16, "%u", (guint) getpid ());

  if (pipe (out_fd) == -1)
    {
      return FALSE;
    }

  pid = fork ();
  if (pid == 0)
    {
      /* Child process. */
      gchar *args[7] = { "gdb", "-batch", "-ex", "backtrace full",
                         full_prog_name, NULL, NULL };

      args[5] = gimp_pid;

      /* Redirect the debugger output. */
      dup2 (out_fd[1], STDOUT_FILENO);
      close (out_fd[0]);
      close (out_fd[1]);

      /* Run GDB. */
      if (execvp (args[0], args) == -1)
        {
          /* LLDB as alternative. */
          gchar *args_lldb[11] = { "lldb", "--attach-pid", NULL, "--batch",
                                   "--one-line", "bt",
                                   "--one-line-on-crash", "bt",
                                   "--one-line-on-crash", "quit", NULL };

          args_lldb[2] = gimp_pid;

          execvp (args_lldb[0], args_lldb);
        }

      _exit (0);
    }
  else if (pid > 0)
    {
      /* Main process */
      int status;

      waitpid (pid, &status, 0);

      /* It is important to close the writing side of the pipe, otherwise
       * the read() will wait forever without getting the information that
       * writing is finished.
       */
      close (out_fd[1]);

      while ((read_n = read (out_fd[0], buffer, 256)) > 0)
        {
          /* It's hard to know if the debugger was found since it
           * happened in the child. Let's just assume that any output
           * means it succeeded.
           */
          stack_printed = TRUE;

          buffer[read_n] = '\0';
          if (file)
            g_fprintf (file, "%s", buffer);
          if (trace)
            {
              if (! gtrace)
                gtrace = g_string_new (NULL);
              g_string_append (gtrace, (const gchar *) buffer);
            }
        }
      close (out_fd[0]);
    }
  else if (pid == (pid_t) -1)
    {
      /* Fork failed. */
      return FALSE;
    }

#ifdef HAVE_EXECINFO_H
  if (! stack_printed)
    {
      /* As a last resort, try using the backtrace() Linux API. It is a bit
       * less fancy than gdb or lldb, which is why it is not given priority.
       */
      void  *bt_buf[100];
      char **symbols;
      int    n_symbols;
      int    i;

      n_symbols = backtrace (bt_buf, 100);
      symbols = backtrace_symbols (bt_buf, n_symbols);
      if (symbols)
        {
          stack_printed = TRUE;

          for (i = 0; i < n_symbols; i++)
            {
              if (file)
                g_fprintf (file, "%s\n", (const gchar *) symbols[i]);
              if (trace)
                {
                  if (! gtrace)
                    gtrace = g_string_new (NULL);
                  g_string_append (gtrace,
                                   (const gchar *) symbols[i]);
                  g_string_append_c (gtrace, '\n');
                }
            }
          free (symbols);
        }
    }
#endif /* HAVE_EXECINFO_H */

  if (trace)
    {
      if (gtrace)
        *trace = g_string_free (gtrace, FALSE);
      else
        *trace = NULL;
    }
#endif /* G_OS_WIN32 */

  return stack_printed;
}

/* This is mostly the same as g_on_error_query() except that we use our
 * own backtrace function, much more complete.
 */
static void
gimp_on_error_query (void)
{
#ifndef G_OS_WIN32
  gchar buf[16];

 retry:

  g_fprintf (stdout,
             "%s (pid:%u): %s: ",
             full_prog_name,
             (guint) getpid (),
             "[E]xit, show [S]tack trace or [P]roceed");
  fflush (stdout);

  if (isatty(0) && isatty(1))
    fgets (buf, 8, stdin);
  else
    strcpy (buf, "E\n");

  if ((buf[0] == 'E' || buf[0] == 'e')
      && buf[1] == '\n')
    _exit (0);
  else if ((buf[0] == 'P' || buf[0] == 'p')
           && buf[1] == '\n')
    return;
  else if ((buf[0] == 'S' || buf[0] == 's')
           && buf[1] == '\n')
    {
      if (! gimp_print_stack_trace (stdout, NULL))
        g_fprintf (stderr, "%s\n", "Stack trace not available on your system.");
      goto retry;
    }
  else
    goto retry;
#endif
}
