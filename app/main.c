/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef __GLIBC__
#include <malloc.h>
#endif

#ifndef  WAIT_ANY
#define  WAIT_ANY -1
#endif

#include <locale.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "config/gimpconfig-dump.h"

#include "core/core-types.h"
#include "core/gimp.h"

#include "app_procs.h"
#include "errors.h"
#include "sanity.h"
#include "units.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include "gimp-intl.h"


#ifdef GIMP_UNSTABLE
static GimpStackTraceMode  stack_trace_mode   = GIMP_STACK_TRACE_QUERY;
static GimpPDBCompatMode   pdb_compat_mode    = GIMP_PDB_COMPAT_WARN;
#else
static GimpStackTraceMode  stack_trace_mode   = GIMP_STACK_TRACE_NEVER;
static GimpPDBCompatMode   pdb_compat_mode    = GIMP_PDB_COMPAT_ON;
#endif


#ifndef G_OS_WIN32
static void     gimp_sigfatal_handler        (gint sig_num) G_GNUC_NORETURN;
static void     gimp_sigchld_handler         (gint sig_num);
#endif

static gboolean gimp_option_stack_trace_mode (const gchar  *option_name,
                                              const gchar  *value,
                                              gpointer      data,
                                              GError      **error);
static gboolean gimp_option_pdb_compat_mode (const gchar   *option_name,
                                             const gchar   *value,
                                             gpointer       data,
                                             GError       **error);

static void     gimp_show_version            (void);
static void     gimp_show_help               (const gchar  *progname);


/*
 *  argv processing:
 *      Arguments are either switches, their associated
 *      values, or image files.  As switches and their
 *      associated values are processed, those slots in
 *      the argv[] array are NULLed. We do this because
 *      unparsed args are treated as images to load on
 *      startup.
 *
 *      The GTK switches are processed first (X switches are
 *      processed here, not by any X routines).  Then the
 *      general GIMP switches are processed.  Any args
 *      left are assumed to be image files the GIMP should
 *      display.
 *
 *      The exception is the batch switch.  When this is
 *      encountered, all remaining args are treated as batch
 *      commands.
 */

int
main (int    argc,
      char **argv)
{
  GOptionContext  *context;
  const gchar     *abort_message      = NULL;
  const gchar     *full_prog_name     = NULL;
  const gchar     *system_gimprc      = NULL;
  const gchar     *user_gimprc        = NULL;
  const gchar     *session_name       = NULL;
  const gchar     *batch_interpreter  = NULL;
  const gchar    **batch_commands     = NULL;
  gboolean         fatal_warnings     = FALSE;
  gboolean         no_interface       = FALSE;
  gboolean         no_data            = FALSE;
  gboolean         no_fonts           = FALSE;
  gboolean         no_splash          = FALSE;
  gboolean         be_verbose         = FALSE;
  gboolean         use_shm            = FALSE;
  gboolean         use_cpu_accel      = TRUE;
  gboolean         console_messages   = FALSE;
  gboolean         use_debug_handler  = FALSE;
  GError          *error              = NULL;
  gint             i;

  const GOptionEntry entries[] =
    {
      {
        "verbose", 0, 0,
        G_OPTION_ARG_NONE, &be_verbose,
        "Be more verbose.", NULL
      },
      {
        "no-interface", 'i', 0,
        G_OPTION_ARG_NONE, &no_interface,
        "Run without a user interface.", NULL
      },
      {
        "no-data", 'd', 0,
        G_OPTION_ARG_NONE, &no_data,
        "Do not load brushes, gradients, palettes, patterns.", NULL
      },
      {
        "no-fonts", 'f', 0,
        G_OPTION_ARG_NONE, &no_fonts,
        "Do not load any fonts.", NULL
      },
      {
        "no-splash", 's', 0,
        G_OPTION_ARG_NONE, &no_splash,
        "Do not show a startup window.", NULL
      },
      {
        "no-shm", 0, G_OPTION_FLAG_REVERSE,
        G_OPTION_ARG_NONE, &use_shm,
        "Do not use shared memory between GIMP and plugins.", NULL
      },
      {
        "no-cpu-accel", 0, G_OPTION_FLAG_REVERSE,
        G_OPTION_ARG_NONE, &use_cpu_accel,
        "Do not use special CPU acceleration functions.", NULL
      },
      {
        "session", 0, 0,
        G_OPTION_ARG_FILENAME, &session_name,
        "Use an alternate sessionrc file.", "name"
      },
      {
        "gimprc", 0, 0,
        G_OPTION_ARG_FILENAME, &user_gimprc,
        "Use an alternate user gimprc file.", "filename"
      },
      {
        "system-gimprc", 0, 0,
        G_OPTION_ARG_FILENAME, &system_gimprc,
        "Use an alternate system gimprc file.", "filename"
      },
      {
        "batch", 'b', 0,
        G_OPTION_ARG_STRING_ARRAY, &batch_commands,
        "Batch command to run (can be used multiple times).", "command"
      },
      {
        "batch-interpreter", 0, 0,
        G_OPTION_ARG_STRING, &batch_interpreter,
        "The procedure to process batch commands with.", "procedure"
      },
      {
        "console-messages", 0, 0,
        G_OPTION_ARG_NONE, &console_messages,
        "Send messages to console instead of using a dialog box.", NULL
      },
      {
        "pdb-compat-mode", 0, 0,
        G_OPTION_ARG_CALLBACK, gimp_option_pdb_compat_mode,
        "Procedural Database compatibility mode.", "never | query | always"
      },
      {
        "stack-trace-mode", 0, 0,
        G_OPTION_ARG_CALLBACK, gimp_option_stack_trace_mode,
        "Debugging mode for fatal signals.", NULL
      },
      {
        "debug-handlers", 0, 0,
        G_OPTION_ARG_NONE, &use_debug_handler,
        "Enable non-fatal debugging signal handlers.", NULL
      },
      /*  GTK+ also looks for --g-fatal-warnings, but we want it for
       *  non-interactive use also.
       */
      { "g-fatal-warnings", 0, G_OPTION_FLAG_HIDDEN,
        G_OPTION_ARG_NONE, &fatal_warnings,
        NULL, NULL
      },
      { NULL }
    };


#if 0
  g_mem_set_vtable (glib_mem_profiler_table);
  g_atexit (g_mem_profile);
#endif

  /* Initialize variables */

  full_prog_name = argv[0];

  /* Initialize i18n support */

  setlocale (LC_ALL, "");

  bindtextdomain (GETTEXT_PACKAGE"-libgimp", gimp_locale_directory ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE"-libgimp", "UTF-8");
#endif

  bindtextdomain (GETTEXT_PACKAGE, gimp_locale_directory ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  textdomain (GETTEXT_PACKAGE);

  /* Check argv[] for "--no-interface" before trying to initialize gtk+.
   * We also check for "--help" or "--version" here since those shouldn't
   * require gui libs either.
   */
  for (i = 1; i < argc; i++)
    {
      const gchar *arg = argv[i];

      if (arg[0] != '-')
        continue;

      if ((strcmp (arg, "--no-interface") == 0) ||
	  (strcmp (arg, "-i") == 0))
	{
	  no_interface = TRUE;
	}
      else if ((strcmp (arg, "--version") == 0) ||
               (strcmp (arg, "-v") == 0))
	{
	  gimp_show_version ();
	  app_exit (EXIT_SUCCESS);
	}
      else if ((strcmp (arg, "--help") == 0) ||
	       (strcmp (arg, "-h") == 0))
	{
	  gimp_show_help (full_prog_name);
	  app_exit (EXIT_SUCCESS);
	}
      else if (strncmp (arg, "--dump-gimprc", 13) == 0)
        {
          GimpConfigDumpFormat format = GIMP_CONFIG_DUMP_NONE;

          if (strcmp (arg, "--dump-gimprc") == 0)
            format = GIMP_CONFIG_DUMP_GIMPRC;
          if (strcmp (arg, "--dump-gimprc-system") == 0)
            format = GIMP_CONFIG_DUMP_GIMPRC_SYSTEM;
          else if (strcmp (arg, "--dump-gimprc-manpage") == 0)
            format = GIMP_CONFIG_DUMP_GIMPRC_MANPAGE;

          if (format)
            {
              Gimp     *gimp;
              gboolean  success;

              g_type_init ();

              gimp = g_object_new (GIMP_TYPE_GIMP, NULL);

              units_init (gimp);

              success = gimp_config_dump (format);

              g_object_unref (gimp);

              app_exit (success ? EXIT_SUCCESS : EXIT_FAILURE);
            }
        }
    }

  if (! app_libs_init (&no_interface, &argc, &argv))
    {
      const gchar *msg;

      msg = _("GIMP could not initialize the graphical user interface.\n"
              "Make sure a proper setup for your display environment exists.");
      g_print ("%s\n\n", msg);

      app_exit (EXIT_FAILURE);
    }

  abort_message = sanity_check ();
  if (abort_message)
    app_abort (no_interface, abort_message);

  g_set_application_name (_("The GIMP"));

#if defined (USE_SYSV_SHM) || defined (USE_POSIX_SHM) || defined (G_OS_WIN32)
  use_shm = TRUE;
#endif

#ifdef __GLIBC__
  /* Tweak memory allocation so that memory allocated in chunks >= 4k
   * (64x64 pixel 1bpp tile) gets returned to the system when free'd ().
   */
  mallopt (M_MMAP_THRESHOLD, 64 * 64 - 1);
#endif

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

#ifdef __GNUC__
#warning FIXME: add this code as soon as we depend on gtk+
#endif
  /* g_option_context_add_group (context, gtk_get_option_group (TRUE));
   */

  if (! g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);

      app_exit (EXIT_FAILURE);
    }

  if (fatal_warnings)
    {
      GLogLevelFlags fatal_mask;

      fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
      fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
      g_log_set_always_fatal (fatal_mask);
    }

#ifndef G_OS_WIN32

  /* No use catching these on Win32, the user won't get any
   * stack trace from glib anyhow. It's better to let Windows inform
   * about the program error, and offer debugging (if the user
   * has installed MSVC or some other compiler that knows how to
   * install itself as a handler for program errors).
   */

  /* Handle fatal signals */

  /* these are handled by gimp_terminate() */
  gimp_signal_private (SIGHUP,  gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGINT,  gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGQUIT, gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGABRT, gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGTERM, gimp_sigfatal_handler, 0);

  if (stack_trace_mode != GIMP_STACK_TRACE_NEVER)
    {
      /* these are handled by gimp_fatal_error() */
      gimp_signal_private (SIGBUS,  gimp_sigfatal_handler, 0);
      gimp_signal_private (SIGSEGV, gimp_sigfatal_handler, 0);
      gimp_signal_private (SIGFPE,  gimp_sigfatal_handler, 0);
    }

  /* Ignore SIGPIPE because plug_in.c handles broken pipes */

  gimp_signal_private (SIGPIPE, SIG_IGN, 0);

  /* Collect dead children */

  gimp_signal_private (SIGCHLD, gimp_sigchld_handler, SA_RESTART);

#endif /* G_OS_WIN32 */

  gimp_errors_init (full_prog_name,
                    use_debug_handler,
                    stack_trace_mode);

  app_run (full_prog_name,
           argc - 1,
           argv + 1,
           system_gimprc,
           user_gimprc,
           session_name,
           batch_interpreter,
           batch_commands,
           no_interface,
           no_data,
           no_fonts,
           no_splash,
           be_verbose,
           use_shm,
           use_cpu_accel,
           console_messages,
           stack_trace_mode,
           pdb_compat_mode);

  return EXIT_SUCCESS;
}

static gboolean
gimp_option_stack_trace_mode (const gchar  *option_name,
                              const gchar  *value,
                              gpointer      data,
                              GError      **error)
{
  if (strcmp (value, "never") == 0)
    stack_trace_mode = GIMP_STACK_TRACE_NEVER;
  else if (strcmp (value, "query") == 0)
    stack_trace_mode = GIMP_STACK_TRACE_QUERY;
  else if (strcmp (value, "always") == 0)
    stack_trace_mode = GIMP_STACK_TRACE_ALWAYS;
  else
    return FALSE;

  return TRUE;
}

static gboolean
gimp_option_pdb_compat_mode (const gchar  *option_name,
                             const gchar  *value,
                             gpointer      data,
                             GError      **error)
{
  if (! strcmp (value, "off"))
    pdb_compat_mode = GIMP_PDB_COMPAT_OFF;
  else if (! strcmp (value, "on"))
    pdb_compat_mode = GIMP_PDB_COMPAT_ON;
  else if (! strcmp (value, "warn"))
    pdb_compat_mode = GIMP_PDB_COMPAT_WARN;
  else
    return FALSE;

  return TRUE;
}

static void
gimp_show_version (void)
{
  g_print ("%s %s\n", _("GIMP version"), GIMP_VERSION);
}

static void
gimp_show_help (const gchar *progname)
{
  gimp_show_version ();

  g_print (_("\nUsage: %s [option ... ] [file ... ]\n\n"),
           gimp_filename_to_utf8 (progname));
  g_print (_("Options:\n"));
  g_print (_("  -h, --help               Output this help.\n"));
  g_print (_("  -v, --version            Output version information.\n"));
  g_print (_("  --verbose                Show startup messages.\n"));
  g_print (_("  --no-shm                 Do not use shared memory between GIMP and plugins.\n"));
  g_print (_("  --no-cpu-accel           Do not use special CPU accelerations.\n"));
  g_print (_("  -d, --no-data            Do not load brushes, gradients, palettes, patterns.\n"));
  g_print (_("  -f, --no-fonts           Do not load any fonts.\n"));
  g_print (_("  -i, --no-interface       Run without a user interface.\n"));
  g_print (_("  --display <display>      Use the designated X display.\n"));
  g_print (_("  -s, --no-splash          Do not show the startup window.\n"));
  g_print (_("  --session <name>         Use an alternate sessionrc file.\n"));
  g_print (_("  -g, --gimprc <gimprc>    Use an alternate gimprc file.\n"));
  g_print (_("  --system-gimprc <gimprc> Use an alternate system gimprc file.\n"));
  g_print (_("  --dump-gimprc            Output a gimprc file with default settings.\n"));
  g_print (_("  -c, --console-messages   Display warnings to console instead of a dialog box.\n"));
  g_print (_("  --debug-handlers         Enable non-fatal debugging signal handlers.\n"));
  g_print (_("  --stack-trace-mode <never | query | always>\n"
             "                           Debugging mode for fatal signals.\n"));
  g_print (_("  --pdb-compat-mode <off | on | warn>\n"
             "                           Procedural Database compatibility mode.\n"));
  g_print (_("  --batch-interpreter <procedure>\n"
             "                           The procedure to process batch commands with.\n"));
  g_print (_("  -b, --batch <commands>   Process commands in batch mode.\n"));
  g_print ("\n");
}


#ifdef G_OS_WIN32

/* In case we build this as a windowed application */

#ifdef __GNUC__
#  ifndef _stdcall
#    define _stdcall  __attribute__((stdcall))
#  endif
#endif

int _stdcall
WinMain (struct HINSTANCE__ *hInstance,
	 struct HINSTANCE__ *hPrevInstance,
	 char               *lpszCmdLine,
	 int                 nCmdShow)
{
  return main (__argc, __argv);
}

#endif /* G_OS_WIN32 */


#ifndef G_OS_WIN32

/* gimp core signal handler for fatal signals */

static void
gimp_sigfatal_handler (gint sig_num)
{
  switch (sig_num)
    {
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGABRT:
    case SIGTERM:
      gimp_terminate (g_strsignal (sig_num));
      break;

    case SIGBUS:
    case SIGSEGV:
    case SIGFPE:
    default:
      gimp_fatal_error (g_strsignal (sig_num));
      break;
    }
}

/* gimp core signal handler for death-of-child signals */

static void
gimp_sigchld_handler (gint sig_num)
{
  gint pid;
  gint status;

  while (TRUE)
    {
      pid = waitpid (WAIT_ANY, &status, WNOHANG);

      if (pid <= 0)
	break;
    }
}

#endif /* ! G_OS_WIN32 */
