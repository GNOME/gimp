/* LIGMA - The GNU Image Manipulation Program
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

#include "libligma/ligma.h"

#include "script-fu-text-console.h"

#include "script-fu-intl.h"
#include "script-fu-lib.h"

LigmaValueArray *
script_fu_text_console_run (LigmaProcedure        *procedure,
                            const LigmaValueArray *args)
{
  script_fu_redirect_output_to_stdout ();

  script_fu_print_welcome ();

  ligma_plug_in_set_pdb_error_handler (ligma_procedure_get_plug_in (procedure),
                                      LIGMA_PDB_ERROR_HANDLER_PLUGIN);

  script_fu_run_read_eval_print_loop ();

  ligma_plug_in_set_pdb_error_handler (ligma_procedure_get_plug_in (procedure),
                                      LIGMA_PDB_ERROR_HANDLER_INTERNAL);

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}
