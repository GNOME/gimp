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

#include <string.h>
#include <stdlib.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-batch.h"
#include "gimpparamspecs.h"

#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"

#include "gimp-intl.h"


#define BATCH_DEFAULT_EVAL_PROC   "plug-in-script-fu-eval"


static void  gimp_batch_exit_after_callback (Gimp          *gimp) G_GNUC_NORETURN;

static void  gimp_batch_run_cmd             (Gimp          *gimp,
                                             const gchar   *proc_name,
                                             GimpProcedure *procedure,
                                             GimpRunMode    run_mode,
                                             const gchar   *cmd);


void
gimp_batch_run (Gimp         *gimp,
                const gchar  *batch_interpreter,
                const gchar **batch_commands)
{
  gulong  exit_id;

  if (! batch_commands || ! batch_commands[0])
    return;

  exit_id = g_signal_connect_after (gimp, "exit",
                                    G_CALLBACK (gimp_batch_exit_after_callback),
                                    NULL);

  if (! batch_interpreter)
    {
      batch_interpreter = g_getenv ("GIMP_BATCH_INTERPRETER");

      if (! batch_interpreter)
        {
          batch_interpreter = BATCH_DEFAULT_EVAL_PROC;

          if (gimp->be_verbose)
            g_printerr ("No batch interpreter specified, using the default "
                        "'%s'.\n", batch_interpreter);
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
        gimp_batch_run_cmd (gimp, proc_name, procedure,
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
            gimp_batch_run_cmd (gimp, batch_interpreter, eval_proc,
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


/*
 * The purpose of this handler is to exit GIMP cleanly when the batch
 * procedure calls the gimp-exit procedure. Without this callback, the
 * message "batch command experienced an execution error" would appear
 * and gimp would hang forever.
 */
static void
gimp_batch_exit_after_callback (Gimp *gimp)
{
  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  gegl_exit ();

  exit (EXIT_SUCCESS);
}

static void
gimp_batch_run_cmd (Gimp          *gimp,
                    const gchar   *proc_name,
                    GimpProcedure *procedure,
                    GimpRunMode    run_mode,
                    const gchar   *cmd)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  GError         *error = NULL;
  gint            i     = 0;

  args = gimp_procedure_get_arguments (procedure);

  if (procedure->num_args > i &&
      GIMP_IS_PARAM_SPEC_INT32 (procedure->args[i]))
    g_value_set_int (gimp_value_array_index (args, i++), run_mode);

  if (procedure->num_args > i &&
      GIMP_IS_PARAM_SPEC_STRING (procedure->args[i]))
    g_value_set_static_string (gimp_value_array_index (args, i++), cmd);

  return_vals =
    gimp_pdb_execute_procedure_by_name_args (gimp->pdb,
                                             gimp_get_user_context (gimp),
                                             NULL, &error,
                                             proc_name, args);

  switch (g_value_get_enum (gimp_value_array_index (return_vals, 0)))
    {
    case GIMP_PDB_EXECUTION_ERROR:
      if (error)
        {
          g_printerr ("batch command experienced an execution error:\n"
                      "%s\n", error->message);
        }
      else
        {
          g_printerr ("batch command experienced an execution error\n");
        }
      break;

    case GIMP_PDB_CALLING_ERROR:
      if (error)
        {
          g_printerr ("batch command experienced a calling error:\n"
                      "%s\n", error->message);
        }
      else
        {
          g_printerr ("batch command experienced a calling error\n");
        }
      break;

    case GIMP_PDB_SUCCESS:
      g_printerr ("batch command executed successfully\n");
      break;
    }

  gimp_value_array_unref (return_vals);
  gimp_value_array_unref (args);

  if (error)
    g_error_free (error);

  return;
}
