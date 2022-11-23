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

#include <libligma/ligma.h>

#include "script-fu-types.h"    /* SFScript */
#include "script-fu-lib.h"
#include "script-fu-script.h"

#include "script-fu-command.h"


/* Methods for interpreting commands.
 *
 * Usually there is a stack of calls similar to:
 *       script_fu_run_image_procedure (outer run func)
 * calls script_fu_interpret_image_proc
 * calls script_fu_run_command
 * calls ts_interpret_string
 * calls the inner run func in Scheme
 *
 * but script_fu_run_command is also called directly for loading scripts.
 *
 * FUTURE: see also similar code in script-fu-interface.c
 * which could be migrated here.
 */


/* Interpret a command.
 *
 * When errors during interpretation:
 * 1) set the error message from tinyscheme into GError at given handle.
 * 2) return FALSE
 * otherwise, return TRUE and discard any result of interpretation
 * ScriptFu return values only have a LigmaPDBStatus,
 * since ScriptFu plugin scripts can only be declared returning void.
 *
 * While interpreting, any errors from further calls to the PDB
 * can show error dialogs in any LIGMA gui,
 * unless the caller has taken responsibility with a prior call to
 * ligma_plug_in_set_pdb_error_handler
 *
 * FIXME: see script_fu_run_procedure.
 * It does not call ligma_plug_in_set_pdb_error_handler for NON-INTERACTIVE mode.
 */
gboolean
script_fu_run_command (const gchar  *command,
                       GError      **error)
{
  GString  *output;
  gboolean  success = FALSE;

  g_debug ("script_fu_run_command: %s", command);
  output = g_string_new (NULL);
  script_fu_redirect_output_to_gstr (output);

  if (script_fu_interpret_string (command))
    {
      g_set_error (error, LIGMA_PLUG_IN_ERROR, 0, "%s", output->str);
    }
  else
    {
      success = TRUE;
    }

  g_string_free (output, TRUE);

  return success;
}



/* Interpret a script that defines a LigmaImageProcedure.
 *
 * Similar to v2 code in script-fu-interface.c, except:
 * 1) builds a command from a GValueArray from a LigmaConfig,
 *    instead of from local array of SFArg.
 * 2) adds actual args image, drawable, etc. for LigmaImageProcedure
 */
LigmaValueArray *
script_fu_interpret_image_proc (
                            LigmaProcedure        *procedure,
                            SFScript             *script,
                            LigmaImage            *image,
                            guint                 n_drawables,
                            LigmaDrawable        **drawables,
                            const LigmaValueArray *args)
{
  gchar          *command;
  LigmaValueArray *result = NULL;
  gboolean        interpretation_result;
  GError         *error = NULL;

  command = script_fu_script_get_command_for_image_proc (script, image, n_drawables, drawables, args);

  /* Take responsibility for handling errors from the scripts further calls to PDB.
   * ScriptFu does not show an error dialog, but forwards errors back to LIGMA.
   * This only tells LIGMA that ScriptFu itself will forward LigmaPDBStatus errors from
   * this scripts calls to the PDB.
   * The onus is on this script's called PDB procedures to return errors in the LigmaPDBStatus.
   * Any that do not, but for example only call ligma-message, are breaching contract.
   */
  ligma_plug_in_set_pdb_error_handler (ligma_get_plug_in (),
                                      LIGMA_PDB_ERROR_HANDLER_PLUGIN);

  interpretation_result = script_fu_run_command (command, &error);
  g_free (command);
  if (! interpretation_result)
    {
      /* This is to the console.
       * script->name not localized.
       * error->message expected to be localized.
       * LIGMA will later display "PDB procedure failed: <message>" localized.
       */
      g_warning ("While executing %s: %s",
                 script->name,
                 error->message);
      /* A GError was allocated and this will take ownership. */
      result = ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_EXECUTION_ERROR,
                                                 error);
    }
  else
    {
      result = ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_SUCCESS,
                                                 NULL);
    }

  ligma_plug_in_set_pdb_error_handler (ligma_get_plug_in (),
                                      LIGMA_PDB_ERROR_HANDLER_INTERNAL);

  return result;
}
