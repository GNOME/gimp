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

#include <string.h>
#include <stdlib.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-batch.h"
#include "ligmaparamspecs.h"

#include "pdb/ligmapdb.h"
#include "pdb/ligmaprocedure.h"

#include "plug-in/ligmapluginmanager.h"
#include "plug-in/ligmapluginprocedure.h"

#include "ligma-intl.h"


static void  ligma_batch_exit_after_callback (Ligma          *ligma) G_GNUC_NORETURN;

static gint  ligma_batch_run_cmd             (Ligma          *ligma,
                                             const gchar   *proc_name,
                                             LigmaProcedure *procedure,
                                             LigmaRunMode    run_mode,
                                             const gchar   *cmd);


gint
ligma_batch_run (Ligma         *ligma,
                const gchar  *batch_interpreter,
                const gchar **batch_commands)
{
  LigmaProcedure *eval_proc;
  GSList        *batch_procedures;
  GSList        *iter;
  gulong         exit_id;
  gint           retval = EXIT_SUCCESS;

  if (! batch_commands || ! batch_commands[0])
    return retval;

  batch_procedures = ligma_plug_in_manager_get_batch_procedures (ligma->plug_in_manager);
  if (g_slist_length (batch_procedures) == 0)
    {
      g_message (_("No batch interpreters are available. "
                   "Batch mode disabled."));
      retval = 69; /* EX_UNAVAILABLE - service unavailable (sysexits.h) */
      return retval;
    }

  if (! batch_interpreter)
    {
      batch_interpreter = g_getenv ("LIGMA_BATCH_INTERPRETER");

      if (! batch_interpreter)
        {
          if (g_slist_length (batch_procedures) == 1)
            {
              batch_interpreter = ligma_object_get_name (batch_procedures->data);;

              if (ligma->be_verbose)
                g_printerr (_("No batch interpreter specified, using "
                              "'%s'.\n"), batch_interpreter);
            }
          else
            {
              retval = 64; /* EX_USAGE - command line usage error */
              g_print ("%s\n\n%s\n",
                       _("No batch interpreter specified."),
                       _("Available interpreters are:"));

              for (iter = batch_procedures; iter; iter = iter->next)
                {
                  LigmaPlugInProcedure *proc = iter->data;
                  gchar               *locale_name;

                  locale_name = g_locale_from_utf8 (proc->batch_interpreter_name,
                                                    -1, NULL, NULL, NULL);

                  g_print ("- %s (%s)\n",
                           ligma_object_get_name (iter->data),
                           locale_name ? locale_name : proc->batch_interpreter_name);

                  g_free (locale_name);
                }

              g_print ("\n%s\n",
                       _("Specify one of these interpreters as --batch-interpreter option."));

              return retval;
            }
        }
    }
  for (iter = batch_procedures; iter; iter = iter->next)
    {
      if (g_strcmp0 (ligma_object_get_name (iter->data),
                     batch_interpreter) == 0)
        break;
    }

  if (iter == NULL)
    {
      retval = 69; /* EX_UNAVAILABLE - service unavailable (sysexits.h) */
      g_print (_("The procedure '%s' is not a valid batch interpreter."),
                 batch_interpreter);
      g_print ("\n%s\n\n%s\n",
               _("Batch mode disabled."),
               _("Available interpreters are:"));

      for (iter = batch_procedures; iter; iter = iter->next)
        {
          LigmaPlugInProcedure *proc = iter->data;
          gchar               *locale_name;

          locale_name = g_locale_from_utf8 (proc->batch_interpreter_name,
                                            -1, NULL, NULL, NULL);

          g_print ("- %s (%s)\n",
                   ligma_object_get_name (iter->data),
                   locale_name ? locale_name : proc->batch_interpreter_name);

          g_free (locale_name);
        }

      g_print ("\n%s\n",
               _("Specify one of these interpreters as --batch-interpreter option."));

      return retval;
    }

  exit_id = g_signal_connect_after (ligma, "exit",
                                    G_CALLBACK (ligma_batch_exit_after_callback),
                                    NULL);

  eval_proc = ligma_pdb_lookup_procedure (ligma->pdb, batch_interpreter);
  if (eval_proc)
    {
      gint i;

      retval = EXIT_SUCCESS;
      for (i = 0; batch_commands[i]; i++)
        {
          retval = ligma_batch_run_cmd (ligma, batch_interpreter, eval_proc,
                                       LIGMA_RUN_NONINTERACTIVE, batch_commands[i]);

          /* In case of several commands, stop and return last
           * failed command.
           */
          if (retval != EXIT_SUCCESS)
            {
              g_printerr ("Stopping at failing batch command [%d]: %s\n",
                          i, batch_commands[i]);
              break;
            }
        }
    }
  else
    {
      retval = 69; /* EX_UNAVAILABLE - service unavailable (sysexits.h) */
      g_message (_("The batch interpreter '%s' is not available. "
                   "Batch mode disabled."), batch_interpreter);
    }

  g_signal_handler_disconnect (ligma, exit_id);

  return retval;
}


/*
 * The purpose of this handler is to exit LIGMA cleanly when the batch
 * procedure calls the ligma-exit procedure. Without this callback, the
 * message "batch command experienced an execution error" would appear
 * and ligma would hang forever.
 */
static void
ligma_batch_exit_after_callback (Ligma *ligma)
{
  if (ligma->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  gegl_exit ();

  exit (EXIT_SUCCESS);
}

static inline gboolean
LIGMA_IS_PARAM_SPEC_RUN_MODE (GParamSpec *pspec)
{
  return (G_IS_PARAM_SPEC_ENUM (pspec) &&
          pspec->value_type == LIGMA_TYPE_RUN_MODE);
}

static gint
ligma_batch_run_cmd (Ligma          *ligma,
                    const gchar   *proc_name,
                    LigmaProcedure *procedure,
                    LigmaRunMode    run_mode,
                    const gchar   *cmd)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  GError         *error  = NULL;
  gint            i      = 0;
  gint            retval = EXIT_SUCCESS;

  args = ligma_procedure_get_arguments (procedure);

  if (procedure->num_args > i &&
      LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[i]))
    {
      g_value_set_enum (ligma_value_array_index (args, i++), run_mode);
    }

  if (procedure->num_args > i &&
      G_IS_PARAM_SPEC_STRING (procedure->args[i]))
    {
      g_value_set_static_string (ligma_value_array_index (args, i++), cmd);
    }

  return_vals =
    ligma_pdb_execute_procedure_by_name_args (ligma->pdb,
                                             ligma_get_user_context (ligma),
                                             NULL, &error,
                                             proc_name, args);

  switch (g_value_get_enum (ligma_value_array_index (return_vals, 0)))
    {
    case LIGMA_PDB_EXECUTION_ERROR:
      /* Using Linux's standard exit code as found in /usr/include/sysexits.h
       * Since other platforms may not have the header, I simply
       * hardcode the few cases.
       */
      retval = 70; /* EX_SOFTWARE - internal software error */
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

    case LIGMA_PDB_CALLING_ERROR:
      retval = 64; /* EX_USAGE - command line usage error */
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

    case LIGMA_PDB_SUCCESS:
      retval = EXIT_SUCCESS;
      g_printerr ("batch command executed successfully\n");
      break;

    case LIGMA_PDB_CANCEL:
      /* Not in sysexits.h, but usually used for 'Script terminated by
       * Control-C'. See: https://tldp.org/LDP/abs/html/exitcodes.html
       */
      retval = 130;
      break;

    case LIGMA_PDB_PASS_THROUGH:
      retval = EXIT_FAILURE; /* Catchall. */
      break;
    }

  ligma_value_array_unref (return_vals);
  ligma_value_array_unref (args);

  if (error)
    g_error_free (error);

  return retval;
}
