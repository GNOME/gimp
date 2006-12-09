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
#include <errno.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "libgimp/gimp.h"

#include "scheme-wrapper.h"
#include "script-fu-text-console.h"

#include "script-fu-intl.h"


static void   script_fu_text_console_interface (void);


void
script_fu_text_console_run (const gchar      *name,
                            gint              nparams,
                            const GimpParam  *params,
                            gint             *nreturn_vals,
                            GimpParam       **return_vals)
{
  static GimpParam  values[1];

  /*  Enable Script-Fu output  */
  ts_set_output_file (stdout);
  ts_print_welcome ();

  /*  Run the interface  */
  script_fu_text_console_interface ();

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;
}

static gboolean
read_command (GString *command)
{
  gint next;
  gint level = 0;

  g_string_truncate (command, 0);

  while ((next = fgetc (stdin)) != EOF)
    {
      guchar c = (guchar) next;

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
        ts_interpret_string (command->str);
    }

  g_string_free (command, TRUE);
}
