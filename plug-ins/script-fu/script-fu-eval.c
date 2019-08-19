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

#include "scheme-wrapper.h"
#include "script-fu-eval.h"

#include "script-fu-intl.h"


GimpValueArray *
script_fu_eval_run (GimpProcedure        *procedure,
                    const GimpValueArray *args)
{
  GString           *output = g_string_new (NULL);
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  const gchar       *code;

  run_mode = GIMP_VALUES_GET_ENUM   (args, 0);
  code     = GIMP_VALUES_GET_STRING (args, 1);

  ts_set_run_mode (run_mode);
  ts_register_output_func (ts_gstring_output_func, output);

  switch (run_mode)
    {
    case GIMP_RUN_NONINTERACTIVE:
      if (ts_interpret_string (code) != 0)
        status = GIMP_PDB_EXECUTION_ERROR;
      break;

    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      status        = GIMP_PDB_CALLING_ERROR;
      g_string_assign (output, _("Script-Fu evaluation mode only allows "
                                 "non-interactive invocation"));
      break;

    default:
      break;
    }

  if (status != GIMP_PDB_SUCCESS && output->len > 0)
    {
      GError *error = g_error_new_literal (0, 0,
                                           g_string_free (output, FALSE));

      return gimp_procedure_new_return_values (procedure, status, error);
    }

  g_string_free (output, TRUE);

  return gimp_procedure_new_return_values (procedure, status, NULL);
}
