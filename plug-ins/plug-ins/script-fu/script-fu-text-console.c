/* GIMP - The GNU Image Manipulation Program
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

void
script_fu_text_console_run (const gchar      *name,
                            gint              nparams,
                            const GimpParam  *params,
                            gint             *nreturn_vals,
                            GimpParam       **return_vals)
{
  static GimpParam  values[1];

  /*  Enable Script-Fu output  */
  ts_register_output_func (ts_stdout_output_func, NULL);

  ts_print_welcome ();

  /*  Run the interface  */
  ts_interpret_stdin();

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;
}
