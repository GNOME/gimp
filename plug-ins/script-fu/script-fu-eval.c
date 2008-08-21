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

#include "libgimp/gimp.h"

#include "scheme-wrapper.h"
#include "script-fu-eval.h"

#include "script-fu-intl.h"


void
script_fu_eval_run (const gchar      *name,
                    gint              nparams,
                    const GimpParam  *params,
                    gint             *nreturn_vals,
                    GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunMode       run_mode;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;

  run_mode = params[0].data.d_int32;

  set_run_mode_constant (run_mode);

  switch (run_mode)
    {
    case GIMP_RUN_NONINTERACTIVE:
      /*  Disable Script-Fu output  */
      ts_register_output_func (NULL, NULL);
      if (ts_interpret_string (params[1].data.d_string) != 0)
        status = GIMP_PDB_EXECUTION_ERROR;
      break;

    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      status        = GIMP_PDB_CALLING_ERROR;
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = _("Script-Fu evaluation mode only allows "
                                  "non-interactive invocation");
      break;

    default:
      break;
    }

  values[0].data.d_status = status;
}
