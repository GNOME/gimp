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

#include <errno.h>
#include <stdio.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "libgimp/gimp.h"

#include "siod-wrapper.h"

#include "script-fu-intl.h"


static void script_fu_text_console_interface (void);


void
script_fu_text_console_run (const gchar      *name,
			    gint              nparams,
			    const GimpParam  *params,
			    gint             *nreturn_vals,
			    GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunMode       run_mode;

  run_mode = params[0].data.d_int32;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Enable SIOD output  */
      siod_set_output_file (stdout);
      siod_set_verbose_level (2);
      siod_print_welcome ();

      /*  Run the interface  */
      script_fu_text_console_interface ();

      break;

    case GIMP_RUN_WITH_LAST_VALS:
    case GIMP_RUN_NONINTERACTIVE:
      status = GIMP_PDB_CALLING_ERROR;
      g_message (_("Script-Fu console mode allows only interactive invocation"));
      break;

    default:
      break;
    }

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;
}

static gboolean
read_command (GString *command)
{
  guchar c;
  gint   next;
  gint   level = 0;

  g_string_truncate (command, 0);

  while ((next = fgetc (stdin)) != EOF)
    {
      c = (guchar) next;

      switch (c)
        {
        case '\n':
          if (level == 0)
            return TRUE;
          c = ' ';
          break;

        case '(':
          level++;
          break;

        case ')':
          level--;
          break;

        default:
          break;
        }

      g_string_append_c (command, c);
    }

  if (errno)
    g_printerr ("error reading from stdin: %s\n", g_strerror (errno));

  return FALSE;
}

static void 
script_fu_text_console_interface (void)
{
  GString *command = g_string_new (NULL);

  while (read_command (command))
    {
      if (command->len > 0)
        siod_interpret_string (command->str);
    }

  g_string_free (command, TRUE);
}
