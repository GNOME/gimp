/* gimpdebug
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

/*
 * GimpDebug simply runs a debugger on a given executable and PID and
 * displays a dialog proposing to create a bug report. The reason why it
 * is a separate executable is simply that when the program crashed,
 * even though some actions are possible before exit() by catching fatal
 * errors and signals, it may not be possible anymore to allocate memory
 * anymore. Therefore creating a new dialog, or even just allocating a
 * string with the contents of the debugger are impossible actions.
 * So we call instead a separate program, then exit.
 *
 * Note: this initial version does not handle Windows yet.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>

#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>

#include "app/widgets/gimpcriticaldialog.h"


static gchar * gimp_debug_get_stack_trace (const gchar *full_prog_name,
                                           const gchar *pid);



int
main (int    argc,
      char **argv)
{
  const gchar *program;
  const gchar *pid;
  const gchar *reason;
  const gchar *message;
  const gchar *bt_file = NULL;
  gchar       *trace   = NULL;
  gchar       *error;
  GtkWidget   *dialog;

  if (argc != 5 && argc != 6)
    {
      g_print ("Usage: gimpdebug-2.0 [PROGRAM] [PID] [REASON] [MESSAGE] [BT_FILE]\n\n"
               "Note: the backtrace file is optional and only used in Windows.\n");
      exit (EXIT_FAILURE);
    }

  program = argv[1];
  pid     = argv[2];
  reason  = argv[3];
  message = argv[4];

  error   = g_strdup_printf ("%s: %s", reason, message);

  if (argc == 6)
    {
      bt_file = argv[5];
      g_file_get_contents (bt_file, &trace, NULL, NULL);
    }
  else
    {
      trace = gimp_debug_get_stack_trace (program, pid);
    }

  if (trace == NULL || strlen (trace) == 0)
    exit (EXIT_FAILURE);

  gtk_init (&argc, &argv);

  dialog = gimp_critical_dialog_new (_("GIMP Crash Debug"));
  gimp_critical_dialog_add (dialog, error, trace, TRUE, program,
                            g_ascii_strtoull (pid, NULL, 10));
  g_free (error);
  g_free (trace);
  g_signal_connect (dialog, "delete-event",
                    G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  gtk_widget_show (dialog);
  gtk_main ();

  exit (EXIT_SUCCESS);
}


static gchar *
gimp_debug_get_stack_trace (const gchar *full_prog_name,
                            const gchar *pid)
{
  gchar   *trace  = NULL;

  /* This works only on UNIX systems. On Windows, we'll have to find
   * another method, probably with DrMingW.
   */
#if defined(G_OS_UNIX)
  const gchar *args[7] = { "gdb", "-batch", "-ex", "backtrace full",
                       full_prog_name, pid, NULL };
  gchar       *gdb_stdout;

  if (g_spawn_sync (NULL, (gchar **) args, NULL,
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
