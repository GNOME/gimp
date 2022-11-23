/* LIGMA - The GNU Image Manipulation Program
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

#define _GNU_SOURCE  /* need the POSIX signal API */

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gio/gio.h>
#include <glib/gstdio.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core/core-types.h"

#include "core/ligma.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmaitem.h"
#include "core/ligmaparamspecs.h"

#include "config/ligmacoreconfig.h"

#include "pdb/ligmapdb.h"

#include "errors.h"
#include "ligma-log.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

/*  private variables  */

static Ligma                *the_errors_ligma    = NULL;
static gboolean             use_debug_handler  = FALSE;
static LigmaStackTraceMode   stack_trace_mode   = LIGMA_STACK_TRACE_QUERY;
static gchar               *full_prog_name     = NULL;
static gchar               *backtrace_file     = NULL;
static gchar               *backup_path        = NULL;
static GFile               *backup_file        = NULL;
static LigmaLogHandler       log_domain_handler = 0;
static guint                global_handler_id  = 0;


/*  local function prototypes  */

static void    ligma_message_log_func             (const gchar        *log_domain,
                                                  GLogLevelFlags      flags,
                                                  const gchar        *message,
                                                  gpointer            data);
static void    ligma_error_log_func               (const gchar        *domain,
                                                  GLogLevelFlags      flags,
                                                  const gchar        *message,
                                                  gpointer            data) G_GNUC_NORETURN;

static G_GNUC_NORETURN void  ligma_eek            (const gchar        *reason,
                                                  const gchar        *message,
                                                  gboolean            use_handler);


/*  public functions  */

void
errors_init (Ligma               *ligma,
             const gchar        *_full_prog_name,
             gboolean            _use_debug_handler,
             LigmaStackTraceMode  _stack_trace_mode,
             const gchar        *_backtrace_file)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (_full_prog_name != NULL);
  g_return_if_fail (full_prog_name == NULL);

#ifdef LIGMA_UNSTABLE
  g_printerr ("This is a development version of LIGMA.  "
              "Debug messages may appear here.\n\n");
#endif /* LIGMA_UNSTABLE */

  the_errors_ligma   = ligma;
  use_debug_handler = _use_debug_handler ? TRUE : FALSE;
  stack_trace_mode  = _stack_trace_mode;
  full_prog_name    = g_strdup (_full_prog_name);

  /* Create parent directories for both the crash and backup files. */
  backtrace_file    = g_path_get_dirname (_backtrace_file);
  backup_path       = g_build_filename (ligma_directory (), "backups", NULL);

  g_mkdir_with_parents (backtrace_file, S_IRUSR | S_IWUSR | S_IXUSR);
  g_free (backtrace_file);
  backtrace_file = g_strdup (_backtrace_file);

  g_mkdir_with_parents (backup_path, S_IRUSR | S_IWUSR | S_IXUSR);
  g_free (backup_path);
  backup_path = g_build_filename (ligma_directory (), "backups",
                                  "backup-XXX.xcf", NULL);

  backup_file = g_file_new_for_path (backup_path);

  log_domain_handler = ligma_log_set_handler (FALSE,
                                             G_LOG_LEVEL_WARNING |
                                             G_LOG_LEVEL_MESSAGE |
                                             G_LOG_LEVEL_CRITICAL,
                                             ligma_message_log_func, ligma);

  global_handler_id = g_log_set_handler (NULL,
                                         G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL,
                                         ligma_error_log_func, ligma);
}

void
errors_exit (void)
{
  if (log_domain_handler)
    {
      ligma_log_remove_handler (log_domain_handler);

      log_domain_handler = 0;
    }

  if (global_handler_id)
    {
      g_log_remove_handler (NULL, global_handler_id);

      global_handler_id = 0;
    }

  the_errors_ligma = NULL;

  if (backtrace_file)
    g_free (backtrace_file);
  if (full_prog_name)
    g_free (full_prog_name);
  if (backup_path)
    g_free (backup_path);
  if (backup_file)
    g_object_unref (backup_file);
}

GList *
errors_recovered (void)
{
  GList *recovered   = NULL;
  gchar *backup_path = g_build_filename (ligma_directory (), "backups", NULL);
  GDir  *backup_dir  = NULL;

  if ((backup_dir = g_dir_open (backup_path, 0, NULL)))
    {
      const gchar *file;

      while ((file = g_dir_read_name (backup_dir)))
        {
          if (g_str_has_suffix (file, ".xcf"))
            {
              gchar *path = g_build_filename (backup_path, file, NULL);

              if (g_file_test (path, G_FILE_TEST_IS_REGULAR) &&
                  ! g_file_test (path, G_FILE_TEST_IS_SYMLINK))
                {
                  /* A quick basic security check. It is not foolproof,
                   * but better than nothing to make sure we are not
                   * trying to read, then delete a folder or a symlink
                   * to a file outside the backup directory.
                   */
                  recovered = g_list_append (recovered, path);
                }
              else
                {
                  g_free (path);
                }
            }
        }

      g_dir_close (backup_dir);
    }
  g_free (backup_path);

  return recovered;
}

void
ligma_fatal_error (const gchar *message)
{
  ligma_eek ("fatal error", message, TRUE);
}

void
ligma_terminate (const gchar *message)
{
  ligma_eek ("terminated", message, use_debug_handler);
}


/*  private functions  */

static void
ligma_message_log_func (const gchar    *log_domain,
                       GLogLevelFlags  flags,
                       const gchar    *message,
                       gpointer        data)
{
  Ligma                *ligma        = data;
  LigmaCoreConfig      *config      = ligma->config;
  const gchar         *msg_domain  = NULL;
  LigmaMessageSeverity  severity    = LIGMA_MESSAGE_WARNING;
  gboolean             gui_message = TRUE;
  LigmaDebugPolicy      debug_policy;

  /* All LIGMA messages are processed under the same domain, but
   * we need to keep the log domain information for third party
   * messages.
   */
  if (! log_domain ||
      (! g_str_has_prefix (log_domain, "Ligma") &&
       ! g_str_has_prefix (log_domain, "LibLigma")))
    msg_domain = log_domain;

  /* If debug policy requires it, WARNING and CRITICAL errors must be
   * routed for appropriate debugging.
   */
  g_object_get (G_OBJECT (config),
                "debug-policy", &debug_policy,
                NULL);

  switch (flags & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_WARNING:
      severity = LIGMA_MESSAGE_BUG_WARNING;
      if (debug_policy > LIGMA_DEBUG_POLICY_WARNING)
        gui_message = FALSE;
      break;
    case G_LOG_LEVEL_CRITICAL:
      severity = LIGMA_MESSAGE_BUG_CRITICAL;
      if (debug_policy > LIGMA_DEBUG_POLICY_CRITICAL)
        gui_message = FALSE;
      break;
    }

  if (ligma && gui_message)
    {
      ligma_show_message (ligma, NULL, severity, msg_domain, message);
    }
  else
    {
      const gchar *reason = "Message";

      ligma_enum_get_value (LIGMA_TYPE_MESSAGE_SEVERITY, severity,
                           NULL, NULL, &reason, NULL);

      g_printerr ("%s: %s-%s: %s\n",
                  ligma_filename_to_utf8 (full_prog_name),
                  log_domain, reason, message);
    }
}

static void
ligma_error_log_func (const gchar    *domain,
                     GLogLevelFlags  flags,
                     const gchar    *message,
                     gpointer        data)
{
  ligma_fatal_error (message);
}

static void
ligma_eek (const gchar *reason,
          const gchar *message,
          gboolean     use_handler)
{
  LigmaCoreConfig  *config        = the_errors_ligma->config;
  gboolean         eek_handled   = FALSE;
  LigmaDebugPolicy  debug_policy;
  GList           *iter;
  gint             num_idx;
  gint             i = 0;

  /* LIGMA has 2 ways to handle termination signals and fatal errors: one
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
  g_printerr ("%s: %s: %s\n", full_prog_name, reason, message);

#if ! defined (G_OS_WIN32) || defined (HAVE_EXCHNDL)

  if (use_handler)
    {
#ifndef LIGMA_CONSOLE_COMPILATION
      if (debug_policy != LIGMA_DEBUG_POLICY_NEVER &&
          ! the_errors_ligma->no_interface         &&
          backtrace_file)
        {
          FILE     *fd;
          gboolean  has_backtrace = TRUE;

          /* If GUI backtrace enabled (it is disabled by default), it
           * takes precedence over the command line argument.
           */
#ifdef G_OS_WIN32
          const gchar *ligmadebug = "ligma-debug-tool-" LIGMA_TOOL_VERSION ".exe";
#elif defined (PLATFORM_OSX)
          const gchar *ligmadebug = "ligma-debug-tool-" LIGMA_TOOL_VERSION;
#else
          const gchar *ligmadebug = LIBEXECDIR "/ligma-debug-tool-" LIGMA_TOOL_VERSION;
#endif
          gchar *args[9] = { (gchar *) ligmadebug, full_prog_name, NULL,
                             (gchar *) reason, (gchar *) message,
                             backtrace_file, the_errors_ligma->config->last_known_release,
                             NULL, NULL };
          gchar  pid[16];
          gchar  timestamp[16];

          g_snprintf (pid, 16, "%u", (guint) getpid ());
          args[2] = pid;

          g_snprintf (timestamp, 16, "%"G_GINT64_FORMAT, the_errors_ligma->config->last_release_timestamp);
          args[7] = timestamp;

#ifndef G_OS_WIN32
          /* On Win32, the trace has already been processed by ExcHnl
           * and is waiting for us in a text file.
           */
          fd = g_fopen (backtrace_file, "w");
          has_backtrace = ligma_stack_trace_print ((const gchar *) full_prog_name,
                                                  fd, NULL);
          fclose (fd);
#endif

          /* We don't care about any return value. If it fails, too
           * bad, we just won't have any stack trace.
           * We still need to use the sync() variant because we have
           * to keep LIGMA up long enough for the debugger to get its
           * trace.
           */
          if (has_backtrace &&
              g_file_test (backtrace_file, G_FILE_TEST_IS_REGULAR) &&
              g_spawn_async (NULL, args, NULL,
                             G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_STDOUT_TO_DEV_NULL,
                             NULL, NULL, NULL, NULL))
            eek_handled = TRUE;
        }
#endif /* !LIGMA_CONSOLE_COMPILATION */

#ifndef G_OS_WIN32
      if (! eek_handled)
        {
          switch (stack_trace_mode)
            {
            case LIGMA_STACK_TRACE_NEVER:
              break;

            case LIGMA_STACK_TRACE_QUERY:
                {
                  sigset_t sigset;

                  sigemptyset (&sigset);
                  sigprocmask (SIG_SETMASK, &sigset, NULL);

                  if (the_errors_ligma)
                    ligma_gui_ungrab (the_errors_ligma);

                  ligma_stack_trace_query ((const gchar *) full_prog_name);
                }
              break;

            case LIGMA_STACK_TRACE_ALWAYS:
                {
                  sigset_t sigset;

                  sigemptyset (&sigset);
                  sigprocmask (SIG_SETMASK, &sigset, NULL);

                  ligma_stack_trace_print ((const gchar *) full_prog_name,
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

#if defined (G_OS_WIN32) && ! defined (LIGMA_CONSOLE_COMPILATION)
  /* g_on_error_* don't do anything reasonable on Win32. */
  if (! eek_handled && ! the_errors_ligma->no_interface)
    MessageBox (NULL, g_strdup_printf ("%s: %s", reason, message),
                full_prog_name, MB_OK|MB_ICONERROR);
#endif

  /* Let's try to back-up all unsaved images!
   * It is not 100%: when I tested with various bugs created on purpose,
   * I had cases where saving failed. I am not sure if this is because
   * of some memory management along the way to XCF saving or some other
   * messed up state of LIGMA, but this is normal not to expect too much
   * during a crash.
   * Nevertheless in various test cases, I had successful backups XCF of
   * the work in progress. Yeah!
   */
  if (backup_path)
    {
      /* increase the busy counter, so XCF saving calling
       * ligma_set_busy() and ligma_unset_busy() won't call the GUI
       * layer and do whatever windowing system calls to set cursors.
       */
      the_errors_ligma->busy++;

      /* The index of 'XXX' in backup_path string. */
      num_idx = strlen (backup_path) - 7;

      iter = ligma_get_image_iter (the_errors_ligma);
      for (; iter && i < 1000; iter = iter->next)
        {
          LigmaImage *image = iter->data;

          if (! ligma_image_is_dirty (image))
            continue;

          /* This is a trick because we want to avoid any memory
           * allocation when the process is abnormally terminated.
           * We just assume that you'll never have more than 1000 images
           * open (which is already far fetched).
           */
          backup_path[num_idx + 2] = '0' + (i % 10);
          backup_path[num_idx + 1] = '0' + ((i/10) % 10);
          backup_path[num_idx]     = '0' + ((i/100) % 10);

          /* Saving. */
          ligma_pdb_execute_procedure_by_name (the_errors_ligma->pdb,
                                              ligma_get_user_context (the_errors_ligma),
                                              NULL, NULL,
                                              "ligma-xcf-save",
                                              LIGMA_TYPE_RUN_MODE,     LIGMA_RUN_NONINTERACTIVE,
                                              LIGMA_TYPE_IMAGE,        image,
                                              G_TYPE_INT,             0,
                                              LIGMA_TYPE_OBJECT_ARRAY, NULL,
                                              G_TYPE_FILE,            backup_file,
                                              G_TYPE_NONE);
          g_rename (g_file_peek_path (backup_file), backup_path);
          i++;
        }
    }

  exit (EXIT_FAILURE);
}
