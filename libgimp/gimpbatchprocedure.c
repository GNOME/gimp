/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabatchprocedure.c
 * Copyright (C) 2022 Jehan
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

#include "ligma.h"
#include "ligmabatchprocedure.h"
#include "ligmapdb_pdb.h"


struct _LigmaBatchProcedurePrivate
{
  gchar           *interpreter_name;

  LigmaBatchFunc    run_func;
  gpointer         run_data;
  GDestroyNotify   run_data_destroy;

};


static void   ligma_batch_procedure_constructed (GObject              *object);
static void   ligma_batch_procedure_finalize    (GObject              *object);

static void   ligma_batch_procedure_install     (LigmaProcedure        *procedure);
static LigmaValueArray *
              ligma_batch_procedure_run         (LigmaProcedure        *procedure,
                                                const LigmaValueArray *args);
static LigmaProcedureConfig *
            ligma_batch_procedure_create_config (LigmaProcedure        *procedure,
                                                GParamSpec          **args,
                                                gint                  n_args);

G_DEFINE_TYPE_WITH_PRIVATE (LigmaBatchProcedure, ligma_batch_procedure,
                            LIGMA_TYPE_PROCEDURE)

#define parent_class ligma_batch_procedure_parent_class


static void
ligma_batch_procedure_class_init (LigmaBatchProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  LigmaProcedureClass *procedure_class = LIGMA_PROCEDURE_CLASS (klass);

  object_class->constructed      = ligma_batch_procedure_constructed;
  object_class->finalize         = ligma_batch_procedure_finalize;

  procedure_class->install       = ligma_batch_procedure_install;
  procedure_class->run           = ligma_batch_procedure_run;
  procedure_class->create_config = ligma_batch_procedure_create_config;
}

static void
ligma_batch_procedure_init (LigmaBatchProcedure *procedure)
{
  procedure->priv = ligma_batch_procedure_get_instance_private (procedure);
}

static void
ligma_batch_procedure_constructed (GObject *object)
{
  LigmaProcedure *procedure = LIGMA_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  LIGMA_PROC_ARG_ENUM (procedure, "run-mode",
                      "Run mode",
                      "The run mode",
                      LIGMA_TYPE_RUN_MODE,
                      LIGMA_RUN_NONINTERACTIVE,
                      G_PARAM_READWRITE);

  LIGMA_PROC_ARG_STRING (procedure, "script",
                        "Batch commands in the target language",
                        "Batch commands in the target language, which will be run by the interpreter",
                        "",
                        G_PARAM_READWRITE);
}

static void
ligma_batch_procedure_finalize (GObject *object)
{
  LigmaBatchProcedure *procedure = LIGMA_BATCH_PROCEDURE (object);

  g_clear_pointer (&procedure->priv->interpreter_name, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_batch_procedure_install (LigmaProcedure *procedure)
{
  LigmaBatchProcedure *proc = LIGMA_BATCH_PROCEDURE (procedure);

  g_return_if_fail (proc->priv->interpreter_name != NULL);

  LIGMA_PROCEDURE_CLASS (parent_class)->install (procedure);

  _ligma_pdb_set_batch_interpreter (ligma_procedure_get_name (procedure),
                                   proc->priv->interpreter_name);
}

#define ARG_OFFSET 2

static LigmaValueArray *
ligma_batch_procedure_run (LigmaProcedure        *procedure,
                          const LigmaValueArray *args)
{
  LigmaBatchProcedure *batch_proc = LIGMA_BATCH_PROCEDURE (procedure);
  LigmaValueArray    *remaining;
  LigmaValueArray    *return_values;
  LigmaRunMode        run_mode;
  const gchar       *cmd;
  gint               i;

  run_mode = LIGMA_VALUES_GET_ENUM   (args, 0);
  cmd      = LIGMA_VALUES_GET_STRING (args, 1);

  remaining = ligma_value_array_new (ligma_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < ligma_value_array_length (args); i++)
    {
      GValue *value = ligma_value_array_index (args, i);

      ligma_value_array_append (remaining, value);
    }

  return_values = batch_proc->priv->run_func (procedure,
                                              run_mode,
                                              cmd,
                                              remaining,
                                              batch_proc->priv->run_data);
  ligma_value_array_unref (remaining);

  return return_values;
}

static LigmaProcedureConfig *
ligma_batch_procedure_create_config (LigmaProcedure  *procedure,
                                    GParamSpec    **args,
                                    gint            n_args)
{
  if (n_args > ARG_OFFSET)
    {
      args   += ARG_OFFSET;
      n_args -= ARG_OFFSET;
    }
  else
    {
      args   = NULL;
      n_args = 0;
    }

  return LIGMA_PROCEDURE_CLASS (parent_class)->create_config (procedure,
                                                             args,
                                                             n_args);
}

/*  public functions  */

/**
 * ligma_batch_procedure_new:
 * @plug_in:          a #LigmaPlugIn.
 * @name:             the new procedure's name.
 * @interpreter_name: the public-facing name, e.g. "Python 3".
 * @proc_type:        the new procedure's #LigmaPDBProcType.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new batch interpreter procedure named @name which will call
 * @run_func when invoked.
 *
 * See ligma_procedure_new() for information about @proc_type.
 *
 * #LigmaBatchProcedure is a #LigmaProcedure subclass that makes it easier
 * to write batch interpreter procedures.
 *
 * It automatically adds the standard
 *
 * (#LigmaRunMode, #gchar)
 *
 * arguments of a batch procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via ligma_procedure_run(), it unpacks these standard
 * arguments and calls @run_func which is a #LigmaBatchFunc. The "args"
 * #LigmaValueArray of #LigmaRunSaveFunc only contains additionally added
 * arguments.
 *
 * Returns: a new #LigmaProcedure.
 *
 * Since: 3.0
 **/
LigmaProcedure  *
ligma_batch_procedure_new (LigmaPlugIn       *plug_in,
                          const gchar      *name,
                          const gchar      *interpreter_name,
                          LigmaPDBProcType   proc_type,
                          LigmaBatchFunc     run_func,
                          gpointer          run_data,
                          GDestroyNotify    run_data_destroy)
{
  LigmaBatchProcedure *procedure;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (interpreter_name != NULL && g_utf8_validate (interpreter_name, -1, NULL), NULL);
  g_return_val_if_fail (proc_type != LIGMA_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != LIGMA_PDB_PROC_TYPE_EXTENSION, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (LIGMA_TYPE_BATCH_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->priv->run_func         = run_func;
  procedure->priv->run_data         = run_data;
  procedure->priv->run_data_destroy = run_data_destroy;

  ligma_batch_procedure_set_interpreter_name (procedure, interpreter_name);

  return LIGMA_PROCEDURE (procedure);
}

/**
 * ligma_batch_procedure_set_interpreter_name:
 * @procedure:        A batch procedure.
 * @interpreter_name: A public-facing name for the interpreter, e.g. "Python 3".
 *
 * Associates an interpreter name with a batch procedure.
 *
 * This name can be used for any public-facing strings, such as
 * graphical interface labels or command line usage. E.g. the command
 * line interface could list all available interface, displaying both a
 * procedure name and a "pretty printing" title.
 *
 * Note that since the format name is public-facing, it is recommended
 * to localize it at runtime, for instance through gettext, like:
 *
 * ```c
 * ligma_batch_procedure_set_interpreter_name (procedure, _("Python 3"));
 * ```
 *
 * Some language would indeed localize even some technical terms or
 * acronyms, even if sometimes just to rewrite them with the local
 * writing system.
 *
 * Since: 3.0
 **/
void
ligma_batch_procedure_set_interpreter_name (LigmaBatchProcedure *procedure,
                                           const gchar        *interpreter_name)
{
  g_return_if_fail (LIGMA_IS_BATCH_PROCEDURE (procedure));
  g_return_if_fail (interpreter_name != NULL && g_utf8_validate (interpreter_name, -1, NULL));

  g_free (procedure->priv->interpreter_name);
  procedure->priv->interpreter_name = g_strdup (interpreter_name);
}

/**
 * ligma_batch_procedure_get_interpreter_name:
 * @procedure: A batch procedure object.
 *
 * Returns the procedure's interpreter name, as set with
 * [method@BatchProcedure.set_interpreter_name].
 *
 * Returns: The procedure's interpreter name.
 *
 * Since: 3.0
 **/
const gchar *
ligma_batch_procedure_get_interpreter_name (LigmaBatchProcedure *procedure)
{
  g_return_val_if_fail (LIGMA_IS_BATCH_PROCEDURE (procedure), NULL);

  return procedure->priv->interpreter_name;
}
