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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h> /* Gtk */

#include "script-fu-progress.h"

/* ScriptFu reports progress in app's status bar.
 *
 * ScriptFu alleviates scripts of explicit progress reporting,
 * by reporting progress whenever a script calls a PDB procedure.
 *
 * A script can also use the PDB progress functions,
 * e.g. install a callback.
 * If a script does not install a callback,
 * then the script's use of gimp_progress may interleave
 * with ScriptFu's own use.
 */


static gchar *last_command         = NULL;
static gint   command_count        = 0;
static gint   consec_command_count = 0;


/* Show progress by displaying a text.

 * This hides where the text is going.
 * Currently gimp_progress object displays to the app's status bar.
 * Formerly ScriptFu displayed to a progress bar in the plugin's dialog box.
 * In the future we might display somewhere else.
 *
 * Requires gimp_progress_init was called, else no effect.
 * Requires gimp_progress_install was NOT called, else just triggers
 * the installed callback.
 */
static void
script_fu_interface_progress_update (const gchar *text)
{
  g_debug ("%s calling progress_set_text", G_STRFUNC);
  gimp_progress_set_text (text);
}

/* Called when start a script run.
 * Not necessarily when a plugin process is starting.
 */
void
script_fu_progress_init (void)
{
  /* FUTURE: show the script's menu item?
   * Or the name of the procedure, but user's don't know the names.
   */
  gimp_progress_init ( "Script-Fu");

  /* If this is the long running extension-script-fu, reset these vars.
   *
   * On quitting extension-script-fu or independent interpreter,
   * we may leak last_command, but only on exit.
   */
  g_clear_pointer (&last_command, g_free);
  command_count = 0;
  consec_command_count = 0;
}

/* Called whenever the interpreter calls the PDB.
 * A pulse of progress is: displaying name of PDB procedure executed.
 * Flashing names is like a spinning wheel.
 *
 * User's don't know the procedure names, this is less than ideal.
 *
 * Exclude calls to gimp-progress-<foo>,
 * when the script is itself implementing its own progress reporting.
 * No scripts in the Gimp repo implement their own progress.
 * This too is probably overkill, the name would flash by quickly.
 *
 * We report progress regardless of INTERACTIVE mode.
 *
 * Also does some optimization:
 * omits report for a long sequence of the same command.
 * This is probably overkill that could be eliminated.
 */
void
script_fu_progress_report (const gchar *command)
{
  /* If command is same as previous. */
  if (last_command &&
      strcmp (last_command, command) == 0)
    {
      /* Form a string <command> <number> */
      command_count++;

      if (! g_str_has_prefix (command, "gimp-progress-"))
        {
          gchar *new_command;

          new_command = g_strdup_printf ("%s <%d>", command, command_count);
          script_fu_interface_progress_update (command);
          g_free (new_command);
        }
    }
  else
    {
      /* Not a consecutive command. */
      command_count = 1;

      g_free (last_command);
      last_command = g_strdup (command);

      if (! g_str_has_prefix (command, "gimp-progress-"))
        script_fu_interface_progress_update (command);
      else
        script_fu_interface_progress_update ("");
    }

  /* In v2, ScriptFu was displaying progress to its own progress bar
   * in ScriptFu's own dialog.
   * In v2 needed: while (gtk_events_pending ())  gtk_main_iteration ();
   * In v3 gimp_progress goes to another process, the gimp app,
   * having it's own event loop.
   */
}
