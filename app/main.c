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

#ifndef  WAIT_ANY
#define  WAIT_ANY -1
#endif

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "config/gimpconfig.h"

#include "gui/gui.h"

#include "appenv.h"
#include "app_procs.h"
#include "errors.h"

#include "libgimp/gimpintl.h"


#ifdef G_OS_WIN32
#include <windows.h>
#else
static void   gimp_sigfatal_handler (gint sig_num);
static void   gimp_sigchld_handler  (gint sig_num);
#endif


/*  command line options  */
gboolean         no_interface            = FALSE;
gboolean         no_data                 = FALSE;
gboolean         no_splash               = FALSE;
gboolean         no_splash_image         = FALSE;
gboolean         be_verbose              = FALSE;
gboolean         use_shm                 = FALSE;
gboolean         use_debug_handler       = FALSE;
gboolean         console_messages        = FALSE;
gboolean         restore_session         = FALSE;
StackTraceMode   stack_trace_mode        = STACK_TRACE_QUERY;
gchar           *alternate_gimprc        = NULL;
gchar           *alternate_system_gimprc = NULL;
gchar          **batch_cmds              = NULL;

/*  other global variables  */
gchar              *prog_name       = NULL;  /* our executable name */
MessageHandlerType  message_handler = CONSOLE;


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
  gboolean  show_version = FALSE;
  gboolean  show_help    = FALSE;
  gint      i, j;

#if 0
  g_mem_set_vtable (glib_mem_profiler_table);
  g_atexit (g_mem_profile);
#endif

  /* Initialize variables */

  prog_name = argv[0];

  /* Initialize i18n support */
  INIT_LOCALE (GETTEXT_PACKAGE);

#ifdef ENABLE_NLS
  bindtextdomain ("gimp-libgimp", LOCALEDIR);
#endif

  /*  check argv[] for "--no-interface" before trying to initialize gtk+  */
  for (i = 1; i < argc; i++)
    {
      if ((strcmp (argv[i], "--no-interface") == 0) ||
	  (strcmp (argv[i], "-i") == 0))
	{
	  no_interface = TRUE;
	}
    }

  if (no_interface)
    {
      setlocale (LC_ALL, "");
      g_type_init ();
    }
  else
    {
      gui_libs_init (&argc, &argv);
    }

  /* test code for GimpConfig, will go away */
  {
    GimpConfig *config;
    gchar      *filename;

    config = g_object_new (GIMP_TYPE_CONFIG, NULL);

    filename = gimp_personal_rc_file ("foorc");
    gimp_object_set_name (GIMP_OBJECT (config), filename);
    g_free (filename);

    g_signal_connect_swapped (G_OBJECT (config), "notify",
                              G_CALLBACK (g_print),
                              "GimpConfig property changed\n");

    gimp_config_serialize (config);
    gimp_config_deserialize (config);

    g_object_unref (config);
  }

#if defined (HAVE_SHM_H) || defined (G_OS_WIN32)
  use_shm = TRUE;
#endif

  batch_cmds    = g_new (char *, argc);
  batch_cmds[0] = NULL;

  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--g-fatal-warnings") == 0)
	{
          GLogLevelFlags fatal_mask;

          fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
          fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
          g_log_set_always_fatal (fatal_mask);
 	  argv[i] = NULL;
	}
      else if ((strcmp (argv[i], "--no-interface") == 0) ||
               (strcmp (argv[i], "-i") == 0))
	{
	  no_interface = TRUE;
 	  argv[i] = NULL;
	}
      else if ((strcmp (argv[i], "--batch") == 0) ||
	       (strcmp (argv[i], "-b") == 0))
	{
	  argv[i] = NULL;
	  for (j = 0, i++ ; i < argc; j++, i++)
	    {
	      batch_cmds[j] = argv[i];
	      argv[i] = NULL;
	    }
	  batch_cmds[j] = NULL;

	  if (batch_cmds[0] == NULL)  /* We need at least one batch command */
	    show_help = TRUE;
	}
      else if (strcmp (argv[i], "--system-gimprc") == 0)  
	{
 	  argv[i] = NULL;
	  if (argc <= ++i) 
            {
	      show_help = TRUE;
	    }
          else 
            {
	      alternate_system_gimprc = argv[i];
	      argv[i] = NULL;
            }
	} 
      else if ((strcmp (argv[i], "--gimprc") == 0) || 
               (strcmp (argv[i], "-g") == 0))
	{
	  argv[i] = NULL;
	  if (argc <= ++i) 
            {
	      show_help = TRUE;
	    }
          else
            {
	      alternate_gimprc = argv[i];
	      argv[i] = NULL;
            }
	}
      else if ((strcmp (argv[i], "--help") == 0) ||
	       (strcmp (argv[i], "-h") == 0))
	{
	  show_help = TRUE;
 	  argv[i] = NULL;
	}
      else if ((strcmp (argv[i], "--version") == 0) ||
               (strcmp (argv[i], "-v") == 0))
	{
	  show_version = TRUE;
 	  argv[i] = NULL;
	}
      else if ((strcmp (argv[i], "--no-data") == 0) ||
	       (strcmp (argv[i], "-d") == 0))
	{
	  no_data = TRUE;
 	  argv[i] = NULL;
	}
      else if ((strcmp (argv[i], "--no-splash") == 0) ||
	       (strcmp (argv[i], "-s") == 0))
	{
	  no_splash = TRUE;
 	  argv[i] = NULL;
	}
      else if ((strcmp (argv[i], "--no-splash-image") == 0) ||
	       (strcmp (argv[i], "-S") == 0))
	{
	  no_splash_image = TRUE;
 	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--verbose") == 0)
	{
	  be_verbose = TRUE;
 	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--no-shm") == 0)
	{
	  use_shm = FALSE;
 	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--debug-handlers") == 0)
	{
	  use_debug_handler = TRUE;
 	  argv[i] = NULL;
	}
      else if ((strcmp (argv[i], "--console-messages") == 0) ||
	       (strcmp (argv[i], "-c") == 0))
	{
	  console_messages = TRUE;
 	  argv[i] = NULL;
	}
      else if ((strcmp (argv[i], "--restore-session") == 0) ||
	       (strcmp (argv[i], "-r") == 0))
	{
	  restore_session = TRUE;
 	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--enable-stack-trace") == 0)  
	{
 	  argv[i] = NULL;
	  if (argc <= ++i) 
            {
	      show_help = TRUE;
	    }
          else 
            {
	      if (! strcmp (argv[i], "never"))
		stack_trace_mode = STACK_TRACE_NEVER;
	      else if (! strcmp (argv[i], "query"))
		stack_trace_mode = STACK_TRACE_QUERY;
	      else if (! strcmp (argv[i], "always"))
		stack_trace_mode = STACK_TRACE_ALWAYS;
	      else
		show_help = TRUE;

	      argv[i] = NULL;
            }
	}
      else if (strcmp (argv[i], "--") == 0)
        {
          /*
           *  everything after "--" is a parameter (i.e. image to load)
           */
          argv[i] = NULL;
          break;
        }
      else if (argv[i][0] == '-')
	{
          /*
           *  anything else starting with a '-' is an error.
           */
	  g_print (_("\nInvalid option \"%s\"\n"), argv[i]);
	  show_help = TRUE;
	}
    }

#ifdef G_OS_WIN32
  /* Common windoze apps don't have a console at all. So does Gimp 
   * - if appropiate. This allows to compile as console application
   * with all it's benfits (like inheriting the console) but hide
   * it, if the user doesn't want it.
   */
  if (!show_help && !show_version && !be_verbose && !console_messages)
    FreeConsole ();
#endif

  if (show_version)
    {
      g_print ( "%s %s\n", _("GIMP version"), GIMP_VERSION);
    }

  if (show_help)
    {
      g_print (_("\nUsage: %s [option ... ] [file ... ]\n\n"), argv[0]);
      g_print (_("Options:\n"));
      g_print (_("  -b, --batch <commands>   Run in batch mode.\n"));
      g_print (_("  -c, --console-messages   Display warnings to console instead of a dialog box.\n"));
      g_print (_("  -d, --no-data            Do not load brushes, gradients, palettes, patterns.\n"));
      g_print (_("  -i, --no-interface       Run without a user interface.\n"));
      g_print (_("  -g, --gimprc <gimprc>    Use an alternate gimprc file.\n"));
      g_print (_("  -h, --help               Output this help.\n"));
      g_print (_("  -r, --restore-session    Try to restore saved session.\n"));
      g_print (_("  -s, --no-splash          Do not show the startup window.\n"));
      g_print (_("  -S, --no-splash-image    Do not add an image to the startup window.\n"));
      g_print (_("  -v, --version            Output version information.\n"));
      g_print (_("  --verbose                Show startup messages.\n"));	  
      g_print (_("  --no-shm                 Do not use shared memory between GIMP and plugins.\n"));
      g_print (_("  --no-xshm                Do not use the X Shared Memory extension.\n"));
      g_print (_("  --debug-handlers         Enable non-fatal debugging signal handlers.\n"));
      g_print (_("  --display <display>      Use the designated X display.\n"));
      g_print (_("  --system-gimprc <gimprc> Use an alternate system gimprc file.\n"));
      g_print ("  --enable-stack-trace <never | query | always>\n");
      g_print (_("                           Debugging mode for fatal signals.\n\n"));
    }

  if (show_version || show_help)
    {
#ifdef G_OS_WIN32
      /* Give them time to read the message if it was printed in a
       * separate console window. I would really love to have
       * some way of asking for confirmation to close the console
       * window.
       */
      HANDLE console;
      DWORD mode;

      console = GetStdHandle (STD_OUTPUT_HANDLE);
      if (GetConsoleMode (console, &mode) != 0)
	{
	  g_print (_("(This console window will close in ten seconds)\n"));
	  Sleep(10000);
	}
#endif
      exit (0);
    }

  g_log_set_handler ("Gimp",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     NULL);
  g_log_set_handler ("Gimp-Base",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     NULL);
  g_log_set_handler ("Gimp-Paint-Funcs",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     NULL);
  g_log_set_handler ("Gimp-Core",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     NULL);
  g_log_set_handler ("Gimp-File",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     NULL);
  g_log_set_handler ("Gimp-XCF",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     NULL);
  g_log_set_handler ("Gimp-PDB",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     NULL);
  g_log_set_handler ("Gimp-Widgets",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     NULL);
  g_log_set_handler ("Gimp-Tools",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     NULL);
  g_log_set_handler ("Gimp-Display",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     NULL);
  g_log_set_handler ("Gimp-GUI",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     NULL);

  g_log_set_handler (NULL,
		     G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL,
		     gimp_error_log_func,
		     NULL);

#ifndef G_OS_WIN32

  /* No use catching these on Win32, the user won't get any 
   * stack trace from glib anyhow. It's better to let Windows inform
   * about the program error, and offer debugging (if the user
   * has installed MSVC or some other compiler that knows how to
   * install itself as a handler for program errors).
   */

  /* Handle fatal signals */

  gimp_signal_private (SIGHUP,  gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGINT,  gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGQUIT, gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGABRT, gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGBUS,  gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGSEGV, gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGTERM, gimp_sigfatal_handler, 0);
  gimp_signal_private (SIGFPE,  gimp_sigfatal_handler, 0);

  /* Ignore SIGPIPE because plug_in.c handles broken pipes */

  gimp_signal_private (SIGPIPE, SIG_IGN, 0);

  /* Collect dead children */

  gimp_signal_private (SIGCHLD, gimp_sigchld_handler, SA_RESTART);

#endif /* G_OS_WIN32 */

  /* Initialize the application */
  app_init (argc - 1,
	    argv + 1);

  return 0;
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
