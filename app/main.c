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
#endif   /*  WAIT_ANY  */

#include <glib.h>

#include "libgimp/gimpfeatures.h"
#include "libgimp/gimpenv.h"

#include "appenv.h"
#include "app_procs.h"
#include "errors.h"
#include "install.h"

#include "libgimp/gimpintl.h"

#ifdef G_OS_WIN32
#include <windows.h>
#else
static RETSIGTYPE on_signal (gint);
#ifdef SIGCHLD
static RETSIGTYPE on_sig_child (gint);
#endif
#endif
static void       init            (void);
static void	  on_error        (const gchar    *domain,
				   GLogLevelFlags  flags,
				   const gchar    *msg,
				   gpointer        user_data);

/* GLOBAL data */
gint no_interface      = FALSE;
gint no_data           = FALSE;
gint no_splash         = FALSE;
gint no_splash_image   = FALSE;
gint be_verbose        = FALSE;
gint use_shm           = FALSE;
gint use_debug_handler = FALSE;
gint console_messages  = FALSE;
gint restore_session   = FALSE;

GimpSet *image_context = NULL;

MessageHandlerType message_handler = CONSOLE;

gchar  *prog_name 		= NULL; /* The path name we are invoked with */
gchar  *alternate_gimprc        = NULL;
gchar  *alternate_system_gimprc = NULL;
gchar **batch_cmds              = NULL;


/* LOCAL data */
static gint    gimp_argc = 0;
static gchar **gimp_argv = NULL;

/*
 *  argv processing: 
 *      Arguments are either switches, their associated
 *      values, or image files.  As switches and their
 *      associated values are processed, those slots in
 *      the argv[] array are NULLed. We do this because
 *      unparsed args are treated as images to load on
 *      startup.
 *
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
  gint show_version = FALSE;
  gint show_help    = FALSE;
  gint i, j;
#ifdef HAVE_PUTENV
  gchar *display_env;
#endif

  g_atexit (g_mem_profile);

  /* Initialize variables */

  prog_name = argv[0];

  /* Initialize i18n support */
  INIT_LOCALE ("gimp");

#ifdef ENABLE_NLS
  bindtextdomain ("gimp-libgimp", LOCALEDIR);
#endif

  gtk_init (&argc, &argv);

  setlocale (LC_NUMERIC, "C");  /* gtk seems to zap this during init.. */

#ifdef HAVE_PUTENV
  display_env = g_strconcat ("DISPLAY=", gdk_get_display (), NULL);
  putenv (display_env);
#endif

#if defined (HAVE_SHM_H) || defined (G_OS_WIN32)
  use_shm = TRUE;
#endif

  batch_cmds    = g_new (char *, argc);
  batch_cmds[0] = NULL;

  for (i = 1; i < argc; i++)
    {
      if ((strcmp (argv[i], "--no-interface") == 0) ||
	  (strcmp (argv[i], "-n") == 0))
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
      else if (strcmp (argv[i], "--version") == 0 ||
	       strcmp (argv[i], "-v") == 0)
	{
	  show_version = TRUE;
 	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--no-data") == 0)
	{
	  no_data = TRUE;
 	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--no-splash") == 0)
	{
	  no_splash = TRUE;
 	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--no-splash-image") == 0)
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
      else if (strcmp (argv[i], "--console-messages") == 0)
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
      /*
       *    ANYTHING ELSE starting with a '-' is an error.
       */
      else if (argv[i][0] == '-')
	{
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
    g_print ( "%s %s\n", _("GIMP version"), GIMP_VERSION);

  if (show_help)
    {
      g_print (_("Usage: %s [option ...] [files ...]\n"), argv[0]);
      g_print (_("Valid options are:\n"));
      g_print (_("  -h --help                Output this help.\n"));
      g_print (_("  -v --version             Output version info.\n"));
      g_print (_("  -b --batch <commands>    Run in batch mode.\n"));
      g_print (_("  -g --gimprc <gimprc>     Use an alternate gimprc file.\n"));
      g_print (_("  -n --no-interface        Run without a user interface.\n"));
      g_print (_("  -r --restore-session     Try to restore saved session.\n"));
      g_print (_("  --no-data                Do not load patterns, gradients, palettes, brushes.\n"));
      g_print (_("  --verbose                Show startup messages.\n"));
      g_print (_("  --no-splash              Do not show the startup window.\n"));
      g_print (_("  --no-splash-image        Do not add an image to the startup window.\n"));
      g_print (_("  --no-shm                 Do not use shared memory between GIMP and its plugins.\n"));
      g_print (_("  --no-xshm                Do not use the X Shared Memory extension.\n"));
      g_print (_("  --console-messages       Display warnings to console instead of a dialog box.\n"));
      g_print (_("  --debug-handlers         Enable debugging signal handlers.\n"));
      g_print (_("  --display <display>      Use the designated X display.\n\n"));
      g_print (_("  --system-gimprc <gimprc> Use an alternate system gimprc file.\n"));
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

  g_set_message_handler ((GPrintFunc) gimp_message_func);

#ifndef G_OS_WIN32
  /* No use catching these on Win32, the user won't get any 
   * stack trace from glib anyhow. It's better to let Windows inform
   * about the program error, and offer debugging (if the use
   * has installed MSVC or some other compiler that knows how to
   * install itself as a handler for program errors).
   */

  /* Handle some signals */
#ifdef SIGHUP
  signal (SIGHUP, on_signal);
#endif
#ifdef SIGINT
  signal (SIGINT, on_signal);
#endif
#ifdef SIGQUIT
  signal (SIGQUIT, on_signal);
#endif
#ifdef SIGABRT
  signal (SIGABRT, on_signal);
#endif
#ifdef SIGBUS
  signal (SIGBUS, on_signal);
#endif
#ifdef SIGSEGV
  signal (SIGSEGV, on_signal);
#endif
#ifdef SIGPIPE
  signal (SIGPIPE, on_signal);
#endif
#ifdef SIGTERM
  signal (SIGTERM, on_signal);
#endif
#ifdef SIGFPE
  signal (SIGFPE, on_signal);
#endif

#ifdef SIGCHLD
  /* Handle child exits */
  signal (SIGCHLD, on_sig_child);
#endif

#endif

  g_log_set_handler (NULL, G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL,
		     on_error, NULL);

  /* Keep the command line arguments--for use in gimp_init */
  gimp_argc = argc - 1;
  gimp_argv = argv + 1;

  /* Check the installation */
  install_verify (init);

  /* Main application loop */
  if (!app_exit_finish_done ())
    gtk_main ();

  return 0;
}

#ifdef G_OS_WIN32

/* In case we build this as a windowed application */

#ifdef __GNUC__
#define _stdcall  __attribute__((stdcall))
#endif

int _stdcall
WinMain (struct HINSTANCE__ *hInstance,
	 struct HINSTANCE__ *hPrevInstance,
	 char               *lpszCmdLine,
	 int                 nCmdShow)
{
  return main (__argc, __argv);
}

#endif

static void
init (void)
{
  /*  Continue initializing  */
  gimp_init (gimp_argc, gimp_argv);
}


static void
on_error (const gchar    *domain,
	  GLogLevelFlags  flags,
	  const gchar    *msg,
	  gpointer        user_data)
{
  gimp_fatal_error ("%s", msg);
}

static gboolean caught_fatal_sig = FALSE;

#ifndef G_OS_WIN32

static RETSIGTYPE
on_signal (gint sig_num)
{
  if (caught_fatal_sig)
    kill (getpid (), sig_num);
  caught_fatal_sig = TRUE;

  switch (sig_num)
    {
#ifdef SIGHUP
    case SIGHUP:
      gimp_terminate ("sighup caught");
      break;
#endif
#ifdef SIGINT
    case SIGINT:
      gimp_terminate ("sigint caught");
      break;
#endif
#ifdef SIGQUIT
    case SIGQUIT:
      gimp_terminate ("sigquit caught");
      break;
#endif
#ifdef SIGABRT
    case SIGABRT:
      gimp_terminate ("sigabrt caught");
      break;
#endif
#ifdef SIGBUS
    case SIGBUS:
      gimp_fatal_error ("sigbus caught");
      break;
#endif
#ifdef SIGSEGV
    case SIGSEGV:
      gimp_fatal_error ("sigsegv caught");
      break;
#endif
#ifdef SIGPIPE
    case SIGPIPE:
      gimp_terminate ("sigpipe caught");
      break;
#endif
#ifdef SIGTERM
    case SIGTERM:
      gimp_terminate ("sigterm caught");
      break;
#endif
#ifdef SIGFPE
    case SIGFPE:
      gimp_fatal_error ("sigfpe caught");
      break;
#endif
    default:
      gimp_fatal_error ("unknown signal");
      break;
    }
}

#ifdef SIGCHLD
static RETSIGTYPE
on_sig_child (gint sig_num)
{
  gint pid;
  gint status;

  while (1)
    {
      pid = waitpid (WAIT_ANY, &status, WNOHANG);
      if (pid <= 0)
	break;
    }
}
#endif /* SIGCHLD */

#endif /* !G_OS_WIN32 */
