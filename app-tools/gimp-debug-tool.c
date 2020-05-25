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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * GimpDebug simply displays a dialog with debug data (backtraces,
 * version, etc.), proposing to create a bug report. The reason why it
 * is a separate executable is simply that when the program crashed,
 * even though some actions are possible before exit() by catching fatal
 * errors and signals, it may not be possible to allocate memory
 * anymore. Therefore creating a new dialog is an impossible action.
 * So we call instead a separate program, then exit.
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



int
main (int    argc,
      char **argv)
{
  const gchar *program;
  const gchar *pid;
  const gchar *reason;
  const gchar *message;
  const gchar *bt_file      = NULL;
  const gchar *last_version = NULL;
  const gchar *release_date = NULL;
  gchar       *trace        = NULL;
  gchar       *error;
  GtkWidget   *dialog;

  if (argc != 6 && argc != 8)
    {
      g_print ("Usage: gimp-debug-tool-2.0 [PROGRAM] [PID] [REASON] [MESSAGE] [BT_FILE] "
               "([LAST_VERSION] [RELEASE_TIMESTAMP])\n");
      exit (EXIT_FAILURE);
    }

  program = argv[1];
  pid     = argv[2];
  reason  = argv[3];
  message = argv[4];

  error   = g_strdup_printf ("%s: %s", reason, message);

  bt_file = argv[5];
  g_file_get_contents (bt_file, &trace, NULL, NULL);

  if (trace == NULL || strlen (trace) == 0)
    exit (EXIT_FAILURE);

  if (argc == 8)
    {
      last_version = argv[6];
      release_date = argv[7];
    }

  gtk_init (&argc, &argv);

  dialog = gimp_critical_dialog_new (_("GIMP Crash Debug"), last_version,
                                     release_date ? g_ascii_strtoll (release_date, NULL, 10) : -1);
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
