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


static gboolean gimp_option_stack_trace_mode (const gchar  *option_name,
                                              const gchar  *value,
                                              gpointer      data,
                                              GError      **error);
static gboolean gimp_option_pdb_compat_mode  (const gchar  *option_name,
                                              const gchar  *value,
                                              gpointer      data,
                                              GError      **error);
static gboolean gimp_option_dump_gimprc      (const gchar  *option_name,
                                              const gchar  *value,
                                              gpointer      data,
                                              GError      **error);

static void     gimp_init_i18n               (void);
static void     gimp_init_malloc             (void);
static void     gimp_init_signal_handlers    (void);
static void     gimp_init_log_handlers       (void);

static void     gimp_show_version            (void) G_GNUC_NORETURN;

#ifndef G_OS_WIN32
static void     gimp_sigfatal_handler        (gint sig_num) G_GNUC_NORETURN;
#endif


static const gchar        *system_gimprc      = NULL;
static const gchar        *user_gimprc        = NULL;
static const gchar        *session_name       = NULL;
static const gchar        *batch_interpreter  = NULL;
static const gchar       **batch_commands     = NULL;
static const gchar       **filenames          = NULL;
static gboolean            fatal_warnings     = FALSE;
static gboolean            no_interface       = FALSE;
static gboolean            no_data            = FALSE;
static gboolean            no_fonts           = FALSE;
static gboolean            no_splash          = FALSE;
static gboolean            be_verbose         = FALSE;
#if defined (USE_SYSV_SHM) || defined (USE_POSIX_SHM) || defined (G_OS_WIN32)
static gboolean            use_shm            = TRUE;
#else
static gboolean            use_shm            = FALSE;
#endif
static gboolean            use_cpu_accel      = TRUE;
static gboolean            console_messages   = FALSE;
static gboolean            use_debug_handler  = FALSE;

#ifdef GIMP_UNSTABLE
static GimpStackTraceMode  stack_trace_mode = GIMP_STACK_TRACE_QUERY;
static GimpPDBCompatMode   pdb_compat_mode  = GIMP_PDB_COMPAT_WARN;
#else
static GimpStackTraceMode  stack_trace_mode = GIMP_STACK_TRACE_NEVER;
static GimpPDBCompatMode   pdb_compat_mode  = GIMP_PDB_COMPAT_ON;
#endif


static const GOptionEntry main_entries[] =
{
  { "version", 'v', 0,
    G_OPTION_ARG_CALLBACK, (GOptionArgFunc) gimp_show_version,
    N_("Show version information and exit"), NULL
  },
  {
    "verbose", 0, 0,
    G_OPTION_ARG_NONE, &be_verbose,
    N_("Be more verbose"), NULL
  },
  {
    "no-interface", 'i', 0,
    G_OPTION_ARG_NONE, &no_interface,
    N_("Run without a user interface"), NULL
  },
  {
    "no-data", 'd', 0,
    G_OPTION_ARG_NONE, &no_data,
    N_("Do not load brushes, gradients, palettes, patterns"), NULL
  },
  {
    "no-fonts", 'f', 0,
    G_OPTION_ARG_NONE, &no_fonts,
    N_("Do not load any fonts"), NULL
  },
  {
    "no-splash", 's', 0,
    G_OPTION_ARG_NONE, &no_splash,
    N_("Do not show a startup window"), NULL
  },
  {
    "no-shm", 0, G_OPTION_FLAG_REVERSE,
    G_OPTION_ARG_NONE, &use_shm,
    N_("Do not use shared memory between GIMP and plugins"), NULL
  },
  {
    "no-cpu-accel", 0, G_OPTION_FLAG_REVERSE,
    G_OPTION_ARG_NONE, &use_cpu_accel,
    N_("Do not use special CPU acceleration functions"), NULL
  },
  {
    "session", 0, 0,
    G_OPTION_ARG_FILENAME, &session_name,
    N_("Use an alternate sessionrc file"), "<name>"
  },
  {
    "gimprc", 0, 0,
    G_OPTION_ARG_FILENAME, &user_gimprc,
    N_("Use an alternate user gimprc file"), "<filename>"
  },
  {
    "system-gimprc", 0, 0,
    G_OPTION_ARG_FILENAME, &system_gimprc,
    N_("Use an alternate system gimprc file"), "<filename>"
  },
  {
    "batch", 'b', 0,
    G_OPTION_ARG_STRING_ARRAY, &batch_commands,
    N_("Batch command to run (can be used multiple times)"), "<command>"
  },
  {
    "batch-interpreter", 0, 0,
    G_OPTION_ARG_STRING, &batch_interpreter,
    N_("The procedure to process batch commands with"), "<procedure>"
  },
  {
    "console-messages", 0, 0,
    G_OPTION_ARG_NONE, &console_messages,
    N_("Send messages to console instead of using a dialog box"), NULL
  },
  {
    "pdb-compat-mode", 0, 0,
    G_OPTION_ARG_CALLBACK, gimp_option_pdb_compat_mode,
    N_("Procedural Database compatibility mode"),
    "<never | query | always>"
  },
  {
    "stack-trace-mode", 0, 0,
    G_OPTION_ARG_CALLBACK, gimp_option_stack_trace_mode,
    N_("Debugging mode for fatal signals"), NULL
  },
  {
    "debug-handlers", 0, 0,
    G_OPTION_ARG_NONE, &use_debug_handler,
    N_("Enable non-fatal debugging signal handlers"), NULL
  },
  /*  GTK+ also looks for --g-fatal-warnings, but we want it for
   *  non-interactive use also.
   */
  {
    "g-fatal-warnings", 0, G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_NONE, &fatal_warnings,
    NULL, NULL
  },
  {
    "dump-gimprc", 0, 0,
    G_OPTION_ARG_CALLBACK, gimp_option_dump_gimprc,
    N_("Output a gimprc file with default settings"), NULL
  },
  {
    "dump-gimprc-system", 0, G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_CALLBACK, gimp_option_dump_gimprc,
    NULL, NULL
  },
  {
    "dump-gimprc-manpage", 0, G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_CALLBACK, gimp_option_dump_gimprc,
    NULL, NULL
  },
  {
    G_OPTION_REMAINING, 0, 0,
    G_OPTION_ARG_FILENAME_ARRAY, &filenames,
    NULL, NULL
  },
  { NULL }
};


int
main (int    argc,
      char **argv)
{
  GOptionContext *context;
  GError         *error = NULL;
  const gchar    *abort_message;
  gint            i;

  gimp_init_malloc ();

  gimp_init_i18n ();

  /* set the application name */
  g_set_application_name (_("The GIMP"));

  /* Check argv[] for "--no-interface" before trying to initialize gtk+. */
  for (i = 1; i < argc; i++)
    {
      const gchar *arg = argv[i];

      if (arg[0] != '-')
        continue;

      if ((strcmp (arg, "--no-interface") == 0) || (strcmp (arg, "-i") == 0))
	{
	  no_interface = TRUE;
	}
      else if ((strcmp (arg, "--version") == 0) || (strcmp (arg, "-v") == 0))
	{
	  gimp_show_version ();
	  app_exit (EXIT_SUCCESS);
	}
    }

  /* initialize some libraries (depending on the --no-interface option) */
  if (! app_libs_init (&no_interface, &argc, &argv))
    {
      const gchar *msg;

      msg = _("GIMP could not initialize the graphical user interface.\n"
              "Make sure a proper setup for your display environment exists.");
      g_print ("%s\n\n", msg);

      app_exit (EXIT_FAILURE);
    }

  /* do some sanity checks */
  abort_message = sanity_check ();
  if (abort_message)
    app_abort (no_interface, abort_message);

  /* parse the command-line options */
  context = g_option_context_new ("[FILE|URI...]");
  g_option_context_add_main_entries (context, main_entries, GETTEXT_PACKAGE);

#ifdef __GNUC__
#warning FIXME: add this code as soon as we depend on gtk+-2.6
#endif
  /* g_option_context_add_group (context, gtk_get_option_group (TRUE));
   */

  if (! g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);

      app_exit (EXIT_FAILURE);
    }

  gimp_init_signal_handlers ();

  gimp_init_log_handlers ();

  gimp_errors_init (argv[0], use_debug_handler, stack_trace_mode);

  app_run (argv[0],
           filenames,
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

  g_option_context_free (context);

  return EXIT_SUCCESS;
}

static void
gimp_init_malloc (void)
{
#if 0
  g_mem_set_vtable (glib_mem_profiler_table);
  g_atexit (g_mem_profile);
#endif

#ifdef __GLIBC__
  /* Tweak memory allocation so that memory allocated in chunks >= 4k
   * (64x64 pixel 1bpp tile) gets returned to the system when free'd ().
   */
  mallopt (M_MMAP_THRESHOLD, 64 * 64 - 1);
#endif
}

static void
gimp_init_i18n (void)
{
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
}

static void
gimp_init_signal_handlers (void)
{
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

  /* Restart syscalls on SIGCHLD */
  gimp_signal_private (SIGCHLD, SIG_DFL, SA_RESTART);

#endif /* G_OS_WIN32 */
}

static void
gimp_init_log_handlers (void)
{
  if (fatal_warnings)
    {
      GLogLevelFlags fatal_mask;

      fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
      fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
      g_log_set_always_fatal (fatal_mask);
    }
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

static gboolean
gimp_option_dump_gimprc (const gchar  *option_name,
                         const gchar  *value,
                         gpointer      data,
                         GError      **error)
{
  GimpConfigDumpFormat format = GIMP_CONFIG_DUMP_NONE;

  if (strcmp (option_name, "--dump-gimprc") == 0)
    format = GIMP_CONFIG_DUMP_GIMPRC;
  if (strcmp (option_name, "--dump-gimprc-system") == 0)
    format = GIMP_CONFIG_DUMP_GIMPRC_SYSTEM;
  else if (strcmp (option_name, "--dump-gimprc-manpage") == 0)
    format = GIMP_CONFIG_DUMP_GIMPRC_MANPAGE;

  if (format)
    {
      Gimp     *gimp;
      gboolean  success;

      gimp = g_object_new (GIMP_TYPE_GIMP, NULL);

      units_init (gimp);

      success = gimp_config_dump (format);

      g_object_unref (gimp);

      app_exit (success ? EXIT_SUCCESS : EXIT_FAILURE);
    }

  return FALSE;
}

static void
gimp_show_version (void)
{
  g_print (_("%s version %s"), "GIMP", GIMP_VERSION);
  g_print ("\n");

  app_exit (EXIT_SUCCESS);
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

#endif /* ! G_OS_WIN32 */
