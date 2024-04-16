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

#include "libgimp/gimp.h"

#include "script-fu-eval.h"

#include "script-fu-lib.h"
#include "script-fu-intl.h"


GimpValueArray *
script_fu_eval_run (GimpProcedure        *procedure,
                    GimpRunMode           run_mode,
                    const gchar          *code,
                    GimpProcedureConfig  *config)
{
  script_fu_set_run_mode (run_mode);

  /* IO writes by script go to stdout. */
  script_fu_redirect_output_to_stdout ();

  switch (run_mode)
    {
    case GIMP_RUN_NONINTERACTIVE:
      if (script_fu_interpret_string (code) != 0)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_EXECUTION_ERROR,
                                                 script_fu_get_gerror ());
      else
        return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);

    default:
      {
        GError *error = g_error_new_literal (g_quark_from_string ("scriptfu"), 0,
           _("Script-Fu evaluation mode only allows non-interactive invocation"));

        return gimp_procedure_new_return_values (procedure, GIMP_PDB_CALLING_ERROR, error);
      }
    }
}
