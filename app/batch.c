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

#include <string.h>
#include <stdlib.h>

#include <glib-object.h>

#include "core/core-types.h"

#include "base/base.h"

#include "core/gimp.h"
#include "core/gimpparamspecs.h"

#include "batch.h"

#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"

#include "gimp-intl.h"


#define BATCH_DEFAULT_EVAL_PROC   "plug-in-script-fu-eval"


static gboolean  batch_exit_after_callback (Gimp          *gimp,
                                            gboolean       kill_it);
static void      batch_run_cmd             (Gimp          *gimp,
                                            const gchar   *proc_name,
                                            GimpProcedure *procedure,
                                            GimpRunMode    run_mode,
                                            const gchar   *cmd);


void
batch_run (Gimp         *gimp,
           const gchar  *batch_interpreter,
           const gchar **batch_commands)
{
  gulong  exit_id;

  if (! batch_commands || ! batch_commands[0])
    return;

  exit_id = g_signal_connect_after (gimp, "exit",
                                    G_CALLBACK (batch_exit_after_callback),
                                    NULL);

  if (! batch_interpreter)
    {
      batch_interpreter = g_getenv ("GIMP_BATCH_INTERPRETER");

      if (! batch_interpreter)
        {
          batch_interpreter = BATCH_DEFAULT_EVAL_PROC;

          if (gimp->be_verbose)
            g_printerr (_("No batch interpreter specified, using the default "
                          "'%s'.\n"), batch_interpreter);
        }
    }

  /*  script-fu text console, hardcoded for backward compatibility  */

  if (strcmp (batch_interpreter, "plug-in-script-fu-eval") == 0 &&
      strcmp (batch_commands[0], "-") == 0)
    {
      const gchar   *proc_name = "plug-in-script-fu-text-console";
      GimpProcedure *procedure = gimp_pdb_lookup_procedure (gimp->pdb,
                                                            proc_name);

      if (procedure)
        batch_run_cmd (gimp, proc_name, procedure,
                       GIMP_RUN_NONINTERACTIVE, NULL);
      else
        g_message (_("The batch interpreter '%s' is not available. "
                     "Batch mode disabled."), proc_name);
    }
  else
    {
      GimpProcedure *eval_proc = gimp_pdb_lookup_procedure (gimp->pdb,
                                                            batch_interpreter);

      if (eval_proc)
        {
          gint i;

          for (i = 0; batch_commands[i]; i++)
            batch_run_cmd (gimp, batch_interpreter, eval_proc,
                           GIMP_RUN_NONINTERACTIVE, batch_commands[i]);
        }
      else
        {
          g_message (_("The batch interpreter '%s' is not available. "
                       "Batch mode disabled."), batch_interpreter);
        }
    }

  g_signal_handler_disconnect (gimp, exit_id);
}


static gboolean
batch_exit_after_callback (Gimp     *gimp,
                           gboolean  kill_it)
{
  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRLOC);

  /*  make sure that the swap file is removed before we quit */
  base_exit ();
  exit (EXIT_SUCCESS);

  return TRUE;
}

static void
batch_run_cmd (Gimp          *gimp,
               const gchar   *proc_name,
               GimpProcedure *procedure,
               GimpRunMode    run_mode,
               const gchar   *cmd)
{
  GValueArray *args;
  GValueArray *return_vals;
  gint         i = 0;

  args = gimp_procedure_get_arguments (procedure);

  if (procedure->num_args > i && GIMP_IS_PARAM_SPEC_INT32 (procedure->args[i]))
    g_value_set_int (&args->values[i++], run_mode);

  if (procedure->num_args > i && GIMP_IS_PARAM_SPEC_STRING (procedure->args[i]))
    g_value_set_static_string (&args->values[i++], cmd);

  return_vals =
    gimp_pdb_execute_procedure_by_name_args (gimp->pdb,
                                             gimp_get_user_context (gimp),
                                             NULL,
                                             proc_name, args);

  switch (g_value_get_enum (&return_vals->values[0]))
    {
    case GIMP_PDB_EXECUTION_ERROR:
      g_printerr ("batch command: experienced an execution error.\n");
      break;

    case GIMP_PDB_CALLING_ERROR:
      g_printerr ("batch command: experienced a calling error.\n");
      break;

    case GIMP_PDB_SUCCESS:
      g_printerr ("batch command: executed successfully.\n");
      break;
    }

  g_value_array_free (return_vals);
  g_value_array_free (args);

  return;
}
