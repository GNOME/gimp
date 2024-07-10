/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbatchprocedure.c
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

#include "gimp.h"
#include "gimpbatchprocedure.h"
#include "gimpprocedureconfig-private.h"
#include "gimppdb_pdb.h"


/**
 * GimpBatchProcedure:
 *
 * Batch procedures implement an interpreter able to run commands as input.
 *
 * In particular, batch procedures will be available on the command line
 * through the `--batch-interpreter` option to switch the chosen interpreter.
 * Then any command given through the `--batch` option will have to be in the
 * chosen language.
 *
 * It makes GIMP usable on the command line, but also to process small scripts
 * (without making full-featured plug-ins), fully in command line without
 * graphical interface.
 **/

struct _GimpBatchProcedure
{
  GimpProcedure    parent_instance;

  gchar           *interpreter_name;

  GimpBatchFunc    run_func;
  gpointer         run_data;
  GDestroyNotify   run_data_destroy;
};


static void   gimp_batch_procedure_constructed (GObject              *object);
static void   gimp_batch_procedure_finalize    (GObject              *object);

static void   gimp_batch_procedure_install     (GimpProcedure        *procedure);
static GimpValueArray *
              gimp_batch_procedure_run         (GimpProcedure        *procedure,
                                                const GimpValueArray *args);
static GimpProcedureConfig *
            gimp_batch_procedure_create_config (GimpProcedure        *procedure,
                                                GParamSpec          **args,
                                                gint                  n_args);

G_DEFINE_TYPE (GimpBatchProcedure, gimp_batch_procedure, GIMP_TYPE_PROCEDURE)

#define parent_class gimp_batch_procedure_parent_class


static void
gimp_batch_procedure_class_init (GimpBatchProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpProcedureClass *procedure_class = GIMP_PROCEDURE_CLASS (klass);

  object_class->constructed      = gimp_batch_procedure_constructed;
  object_class->finalize         = gimp_batch_procedure_finalize;

  procedure_class->install       = gimp_batch_procedure_install;
  procedure_class->run           = gimp_batch_procedure_run;
  procedure_class->create_config = gimp_batch_procedure_create_config;
}

static void
gimp_batch_procedure_init (GimpBatchProcedure *procedure)
{
}

static void
gimp_batch_procedure_constructed (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_procedure_add_enum_argument (procedure, "run-mode",
                                    "Run mode",
                                    "The run mode",
                                    GIMP_TYPE_RUN_MODE,
                                    GIMP_RUN_NONINTERACTIVE,
                                    G_PARAM_READWRITE);

  gimp_procedure_add_string_argument (procedure, "script",
                                      "Batch commands in the target language",
                                      "Batch commands in the target language, which will be run by the interpreter",
                                      "",
                                      G_PARAM_READWRITE);
}

static void
gimp_batch_procedure_finalize (GObject *object)
{
  GimpBatchProcedure *procedure = GIMP_BATCH_PROCEDURE (object);

  g_clear_pointer (&procedure->interpreter_name, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_batch_procedure_install (GimpProcedure *procedure)
{
  GimpBatchProcedure *proc = GIMP_BATCH_PROCEDURE (procedure);

  g_return_if_fail (proc->interpreter_name != NULL);

  GIMP_PROCEDURE_CLASS (parent_class)->install (procedure);

  _gimp_pdb_set_batch_interpreter (gimp_procedure_get_name (procedure),
                                   proc->interpreter_name);
}

#define ARG_OFFSET 2

static GimpValueArray *
gimp_batch_procedure_run (GimpProcedure        *procedure,
                          const GimpValueArray *args)
{
  GimpBatchProcedure  *batch_proc = GIMP_BATCH_PROCEDURE (procedure);
  GimpValueArray      *remaining;
  GimpValueArray      *return_values;
  GimpRunMode          run_mode;
  const gchar         *cmd;
  GimpProcedureConfig *config;
  GimpPDBStatusType    status = GIMP_PDB_EXECUTION_ERROR;
  gint                 i;

  run_mode = GIMP_VALUES_GET_ENUM   (args, 0);
  cmd      = GIMP_VALUES_GET_STRING (args, 1);

  remaining = gimp_value_array_new (gimp_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < gimp_value_array_length (args); i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      gimp_value_array_append (remaining, value);
    }

  config = _gimp_procedure_create_run_config (procedure);
  _gimp_procedure_config_begin_run (config, NULL, run_mode, remaining);
  gimp_value_array_unref (remaining);

  return_values = batch_proc->run_func (procedure, run_mode,
                                        cmd, config,
                                        batch_proc->run_data);
  if (return_values != NULL                       &&
      gimp_value_array_length (return_values) > 0 &&
      G_VALUE_HOLDS_ENUM (gimp_value_array_index (return_values, 0)))
    status = GIMP_VALUES_GET_ENUM (return_values, 0);

  _gimp_procedure_config_end_run (config, status);
  g_object_unref (config);

  return return_values;
}

static GimpProcedureConfig *
gimp_batch_procedure_create_config (GimpProcedure  *procedure,
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

  return GIMP_PROCEDURE_CLASS (parent_class)->create_config (procedure,
                                                             args,
                                                             n_args);
}

/*  public functions  */

/**
 * gimp_batch_procedure_new:
 * @plug_in:          a #GimpPlugIn.
 * @name:             the new procedure's name.
 * @interpreter_name: the public-facing name, e.g. "Python 3".
 * @proc_type:        the new procedure's #GimpPDBProcType.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new batch interpreter procedure named @name which will call
 * @run_func when invoked.
 *
 * See gimp_procedure_new() for information about @proc_type.
 *
 * #GimpBatchProcedure is a #GimpProcedure subclass that makes it easier
 * to write batch interpreter procedures.
 *
 * It automatically adds the standard
 *
 * (#GimpRunMode, #gchar)
 *
 * arguments of a batch procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via gimp_procedure_run(), it unpacks these standard
 * arguments and calls @run_func which is a #GimpBatchFunc. The "args"
 * #GimpValueArray of #GimpRunSaveFunc only contains additionally added
 * arguments.
 *
 * Returns: a new #GimpProcedure.
 *
 * Since: 3.0
 **/
GimpProcedure  *
gimp_batch_procedure_new (GimpPlugIn       *plug_in,
                          const gchar      *name,
                          const gchar      *interpreter_name,
                          GimpPDBProcType   proc_type,
                          GimpBatchFunc     run_func,
                          gpointer          run_data,
                          GDestroyNotify    run_data_destroy)
{
  GimpBatchProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (interpreter_name != NULL && g_utf8_validate (interpreter_name, -1, NULL), NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_EXTENSION, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_BATCH_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->run_func         = run_func;
  procedure->run_data         = run_data;
  procedure->run_data_destroy = run_data_destroy;

  gimp_batch_procedure_set_interpreter_name (procedure, interpreter_name);

  return GIMP_PROCEDURE (procedure);
}

/**
 * gimp_batch_procedure_set_interpreter_name:
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
 * gimp_batch_procedure_set_interpreter_name (procedure, _("Python 3"));
 * ```
 *
 * Some language would indeed localize even some technical terms or
 * acronyms, even if sometimes just to rewrite them with the local
 * writing system.
 *
 * Since: 3.0
 **/
void
gimp_batch_procedure_set_interpreter_name (GimpBatchProcedure *procedure,
                                           const gchar        *interpreter_name)
{
  g_return_if_fail (GIMP_IS_BATCH_PROCEDURE (procedure));
  g_return_if_fail (interpreter_name != NULL && g_utf8_validate (interpreter_name, -1, NULL));

  g_free (procedure->interpreter_name);
  procedure->interpreter_name = g_strdup (interpreter_name);
}

/**
 * gimp_batch_procedure_get_interpreter_name:
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
gimp_batch_procedure_get_interpreter_name (GimpBatchProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_BATCH_PROCEDURE (procedure), NULL);

  return procedure->interpreter_name;
}
