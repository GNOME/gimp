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

#include <stdio.h>
#include <errno.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "libgimp/gimp.h"

#include "scheme-wrapper.h"
#include "script-fu-text-console.h"

#include "script-fu-intl.h"

GimpValueArray *
script_fu_text_console_run (GimpProcedure        *procedure,
                            const GimpValueArray *args)
{
  /*  Enable Script-Fu output  */
  ts_register_output_func (ts_stdout_output_func, NULL);

  ts_print_welcome ();

  gimp_plug_in_set_pdb_error_handler (gimp_procedure_get_plug_in (procedure),
                                      GIMP_PDB_ERROR_HANDLER_PLUGIN);

  /*  Run the interface  */
  ts_interpret_stdin ();

  gimp_plug_in_set_pdb_error_handler (gimp_procedure_get_plug_in (procedure),
                                      GIMP_PDB_ERROR_HANDLER_INTERNAL);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}
