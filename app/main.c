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
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "libgimp/gserialize.h"

#ifndef  WAIT_ANY
#define  WAIT_ANY -1
#endif   /*  WAIT_ANY  */

#include "libgimp/gimpfeatures.h"

#include "appenv.h"
#include "app_procs.h"
#include "errors.h"
#include "install.h"

#include "libgimp/gimpintl.h"

static RETSIGTYPE on_signal (int);
static RETSIGTYPE on_sig_child (int);
static void       init (void);
static void       test_gserialize();

/* GLOBAL data */
int no_interface;
int no_data;
int no_splash;
int no_splash_image;
int be_verbose;
int use_shm;
int use_debug_handler;
int console_messages;
int restore_session;
GimpSet* image_context;

MessageHandlerType message_handler;

char *prog_name;		/* The path name we are invoked with */
char *alternate_gimprc;
char *alternate_system_gimprc;
char **batch_cmds;

/* LOCAL data */
static int gimp_argc;
static char **gimp_argv;

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
main (int argc, char **argv)
{
  int show_version;
  int show_help;
  int i, j;
#ifdef HAVE_PUTENV
  gchar *display_name, *display_env;
#endif

  g_atexit (g_mem_profile);

  /* Initialize variables */
  prog_name = argv[0];

  /* Initialize Gtk toolkit */
  gtk_set_locale ();
  setlocale(LC_NUMERIC, "C");  /* must use dot, not comma, as decimal separator */
  /* Initialize i18n support */

#ifdef HAVE_LC_MESSAGES
  setlocale(LC_MESSAGES, "");
#endif
  bindtextdomain("gimp", LOCALEDIR);
  textdomain("gimp");

  gtk_init (&argc, &argv);

#ifdef HAVE_LIBGLE
  gle_init (&argc, &argv);
#endif /* !HAVE_LIBGLE */

#ifdef HAVE_PUTENV
  display_name = gdk_get_display ();
  display_env = g_new (gchar, strlen (display_name) + 9);
  *display_env = 0;
  strcat (display_env, "DISPLAY=");
  strcat (display_env, display_name);
  putenv (display_env);
#endif

  no_interface = FALSE;
  no_data = FALSE;
  no_splash = FALSE;
  no_splash_image = FALSE;
#ifdef HAVE_SHM_H
  use_shm = TRUE;
#else
  use_shm = FALSE;
#endif
  use_debug_handler = FALSE;
  restore_session = FALSE;
  console_messages = FALSE;

  message_handler = CONSOLE;

  batch_cmds = g_new (char*, argc);
  batch_cmds[0] = NULL;
  
  alternate_gimprc = NULL;
  alternate_system_gimprc = NULL;

  show_version = FALSE;
  show_help = FALSE;

  test_gserialize();

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

  if (show_version)
    g_print ( "%s %s\n", _("GIMP version"), GIMP_VERSION);

  if (show_help)
    {
      g_print (_("\007Usage: %s [option ...] [files ...]\n"), argv[0]);
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
    exit (0);

  g_set_message_handler ((GPrintFunc) message_func);

  /* Handle some signals */
  signal (SIGHUP, on_signal);
  signal (SIGINT, on_signal);
  signal (SIGQUIT, on_signal);
  signal (SIGABRT, on_signal);
  signal (SIGBUS, on_signal);
  signal (SIGSEGV, on_signal);
  signal (SIGPIPE, on_signal);
  signal (SIGTERM, on_signal);
  signal (SIGFPE, on_signal);

  /* Handle child exits */
  signal (SIGCHLD, on_sig_child);

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

static void
init ()
{
  /*  Continue initializing  */
  gimp_init (gimp_argc, gimp_argv);
}

static int caught_fatal_sig = 0;

static RETSIGTYPE
on_signal (int sig_num)
{
  if (caught_fatal_sig)
/*    raise (sig_num);*/
    kill (getpid (), sig_num);
  caught_fatal_sig = 1;

  switch (sig_num)
    {
    case SIGHUP:
      terminate (_("sighup caught"));
      break;
    case SIGINT:
      terminate (_("sigint caught"));
      break;
    case SIGQUIT:
      terminate (_("sigquit caught"));
      break;
    case SIGABRT:
      terminate (_("sigabrt caught"));
      break;
    case SIGBUS:
      fatal_error (_("sigbus caught"));
      break;
    case SIGSEGV:
      fatal_error (_("sigsegv caught"));
      break;
    case SIGPIPE:
      terminate (_("sigpipe caught"));
      break;
    case SIGTERM:
      terminate (_("sigterm caught"));
      break;
    case SIGFPE:
      fatal_error (_("sigfpe caught"));
      break;
    default:
      fatal_error (_("unknown signal"));
      break;
    }
}

static RETSIGTYPE
on_sig_child (int sig_num)
{
  int pid;
  int status;

  while (1)
    {
      pid = waitpid (WAIT_ANY, &status, WNOHANG);
      if (pid <= 0)
	break;
    }
}

typedef struct 
{
  gint32 test_gint32;
  float test_float;
  char *test_string;
  guint32  test_length;
  guint16 *test_array;
} test_struct;

static void test_gserialize()
{
  GSerialDescription *test_struct_descript;
  test_struct *ts, *to;
  char ser_1[] = {3, 1, 2, 3, 4, 4, 64, 83, 51, 51, 101, 0, 0, 0, 4, 102, 111, 111, 0, 103, 0, 0, 0, 2, 5, 6, 7, 8 };
  ts = g_malloc(sizeof(test_struct));
  to = g_malloc(sizeof(test_struct));
  test_struct_descript 
    = g_new_serial_description("test_struct",
			       g_serial_item(GSERIAL_INT32,
					     test_struct,
					     test_gint32),
			       g_serial_item(GSERIAL_FLOAT,
					     test_struct,
					     test_float),
			       g_serial_item(GSERIAL_STRING,
					     test_struct,
					     test_string),
			       g_serial_vlen_array(GSERIAL_INT16ARRAY,
						   test_struct,
						   test_array,
						   test_length),
			       NULL);


  ts->test_gint32 = 0x01020304;
  ts->test_float = 3.3f;
  ts->test_string = "foo";
  ts->test_length = 2;
  ts->test_array  = g_malloc(sizeof(short) *2);
  ts->test_array[0] = 0x0506;
  ts->test_array[1] = 0x0708;

  g_deserialize(test_struct_descript,  (char *)(void*)to, ser_1);

  if (to->test_gint32 != ts->test_gint32)
    g_message("gint32 test failed (please email your system configuration to jaycox@earthlink.net): %d\n", to->test_gint32);
  if (to->test_float != ts->test_float)
    g_message("float test failed (please email your system configuration to jaycox@earthlink.net): %f\n", to->test_float);
  if (strcmp(to->test_string, ts->test_string) != 0)
    g_message("string test failed (please email your system configuration to jaycox@earthlink.net): %s\n", to->test_string);
  if (to->test_length != ts->test_length)
    g_message("array length test failed(please email your system configuration to jaycox@earthlink.net): %d\n", to->test_length);
  if (to->test_array[0] != ts->test_array[0])
    g_message("int16array value 0 test failed(please email your system configuration to jaycox@earthlink.net): %d\n", to->test_array[0]);
  g_return_if_fail (to->test_array[1] == ts->test_array[1]);
  if (to->test_array[1] != ts->test_array[1])
    g_message("int16array value 1 test failed(please email your system configuration to jaycox@earthlink.net): %d\n", to->test_array[1]);
  /*  really should free the memory... */
  g_message("Passed serialization test\n");
}/* 
    67108864 */
