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
#include "siod-wrapper.h"
#include "libgimp/gimp.h"
#include <stdio.h>
#include "script-fu-intl.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <glib.h>

static void script_fu_text_console_interface (void);

void
script_fu_text_console_run (char     *name,
			    int       nparams,
			    GimpParam   *params,
			    int      *nreturn_vals,
			    GimpParam  **return_vals)
{
  static GimpParam values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunModeType run_mode;

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
      gimp_message (_("Script-Fu console mode allows only interactive invocation"));
      break;

    default:
      break;
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static char *
read_command (void)
{
  unsigned char c;
  GString *string;
  char *retval;
  int left = 0, right = 0;

  string = g_string_new ("");

  c = (unsigned char )fgetc (stdin);
  while (!((c == '\n') && (left == right)))
    {
     
      if (c == '(')
	left++;
      else if (c == ')')
	right++;
      
      if (c != '\n') 
	string = g_string_append_c (string, c);
      else
	string = g_string_append_c (string, ' ');

      c = (unsigned char )fgetc (stdin);      
    }
    
  retval = string->str;
  g_string_free (string, 0);

  return retval;
}

static void 
script_fu_text_console_interface (void)
{
  char *command;

  while (1)
    {
      command = read_command ();
      siod_interpret_string (command);
      g_free (command);
    }
}



