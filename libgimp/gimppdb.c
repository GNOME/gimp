/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapdb.c
 * Copyright (C) 2019 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmaprotocol.h"
#include "libligmabase/ligmawire.h"

#include "ligma-private.h"
#include "ligmagpparams.h"
#include "ligmapdb-private.h"
#include "ligmapdb_pdb.h"
#include "ligmapdbprocedure.h"
#include "ligmaplugin-private.h"

#include "libligma-intl.h"


/**
 * LigmaPDB:
 *
 * Provides access to the Procedural DataBase (PDB).
 */


struct _LigmaPDBPrivate
{
  LigmaPlugIn         *plug_in;

  GHashTable         *procedures;

  LigmaPDBStatusType   error_status;
  gchar              *error_message;
};


static void   ligma_pdb_dispose   (GObject        *object);
static void   ligma_pdb_finalize  (GObject        *object);

static void   ligma_pdb_set_error (LigmaPDB        *pdb,
                                  LigmaValueArray *return_values);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaPDB, ligma_pdb, G_TYPE_OBJECT)

#define parent_class ligma_pdb_parent_class


static void
ligma_pdb_class_init (LigmaPDBClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = ligma_pdb_dispose;
  object_class->finalize = ligma_pdb_finalize;
}

static void
ligma_pdb_init (LigmaPDB *pdb)
{
  pdb->priv = ligma_pdb_get_instance_private (pdb);

  pdb->priv->procedures = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                 g_free, g_object_unref);

  pdb->priv->error_status = LIGMA_PDB_SUCCESS;
}

static void
ligma_pdb_dispose (GObject *object)
{
  LigmaPDB *pdb = LIGMA_PDB (object);

  g_clear_pointer (&pdb->priv->procedures, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_pdb_finalize (GObject *object)
{
  LigmaPDB *pdb = LIGMA_PDB (object);

  g_clear_object (&pdb->priv->plug_in);
  g_clear_pointer (&pdb->priv->error_message, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

LigmaPDB *
_ligma_pdb_new (LigmaPlugIn *plug_in)
{
  LigmaPDB *pdb;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);

  pdb = g_object_new (LIGMA_TYPE_PDB, NULL);

  pdb->priv->plug_in = g_object_ref (plug_in);

  return pdb;
}

LigmaPlugIn *
_ligma_pdb_get_plug_in (LigmaPDB *pdb)
{
  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);

  return pdb->priv->plug_in;
}

/**
 * ligma_pdb_procedure_exists:
 * @pdb:            A PDB instance.
 * @procedure_name: A procedure name
 *
 * This function checks if a procedure exists in the procedural
 * database.
 *
 * Return: %TRUE if the procedure exists, %FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
ligma_pdb_procedure_exists (LigmaPDB     *pdb,
                           const gchar *procedure_name)
{
  g_return_val_if_fail (LIGMA_IS_PDB (pdb), FALSE);
  g_return_val_if_fail (ligma_is_canonical_identifier (procedure_name), FALSE);

  return _ligma_pdb_proc_exists (procedure_name);
}

/**
 * ligma_pdb_lookup_procedure:
 * @pdb:            A #LigmaPDB instance.
 * @procedure_name: A procedure name
 *
 * This function returns the [class@Procedure] which is registered
 * with @procedure_name if it exists, or returns %NULL otherwise.
 *
 * The returned [class@Procedure] is owned by @pdb and must not be modified.
 *
 * Return: (nullable) (transfer none): A [class@Procedure], or %NULL.
 *
 * Since: 3.0
 **/
LigmaProcedure *
ligma_pdb_lookup_procedure (LigmaPDB     *pdb,
                           const gchar *procedure_name)
{
  LigmaProcedure *procedure;

  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (procedure_name), NULL);

  procedure = g_hash_table_lookup (pdb->priv->procedures, procedure_name);

  if (! procedure)
    {
      procedure = _ligma_pdb_procedure_new (pdb, procedure_name);

      if (procedure)
        g_hash_table_insert (pdb->priv->procedures,
                             g_strdup (procedure_name), procedure);
    }

  return procedure;
}

/**
 * ligma_pdb_run_procedure: (skip)
 * @pdb:            the #LigmaPDB object.
 * @procedure_name: the procedure registered name.
 * @first_type:     the #GType of the first argument, or #G_TYPE_NONE.
 * @...:            the call arguments.
 *
 * Runs the procedure named @procedure_name with arguments given as
 * list of (#GType, value) pairs, terminated by #G_TYPE_NONE.
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
LigmaValueArray *
ligma_pdb_run_procedure (LigmaPDB     *pdb,
                        const gchar *procedure_name,
                        GType        first_type,
                        ...)
{
  LigmaValueArray *return_values;
  va_list         args;

  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (procedure_name), NULL);

  va_start (args, first_type);

  return_values = ligma_pdb_run_procedure_valist (pdb, procedure_name,
                                                 first_type, args);

  va_end (args);

  return return_values;
}

/**
 * ligma_pdb_run_procedure_valist: (skip)
 * @pdb:            the #LigmaPDB object.
 * @procedure_name: the procedure registered name.
 * @first_type:     the #GType of the first argument, or #G_TYPE_NONE.
 * @args:           the call arguments.
 *
 * Runs the procedure named @procedure_name with @args given in the
 * order as passed to [method@PDB.run_procedure].
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
LigmaValueArray *
ligma_pdb_run_procedure_valist (LigmaPDB     *pdb,
                               const gchar *procedure_name,
                               GType        first_type,
                               va_list      args)
{
  LigmaValueArray *arguments;
  LigmaValueArray *return_values;
  gchar          *error_msg = NULL;

  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (procedure_name), NULL);

  arguments = ligma_value_array_new_from_types_valist (&error_msg,
                                                      first_type,
                                                      args);

  if (! arguments)
    {
      GError *error = g_error_new_literal (LIGMA_PDB_ERROR,
                                           LIGMA_PDB_ERROR_INTERNAL_ERROR,
                                           error_msg);
      g_printerr ("%s: %s", G_STRFUNC, error_msg);
      g_free (error_msg);

      return ligma_procedure_new_return_values (NULL,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }

  return_values = ligma_pdb_run_procedure_array (pdb, procedure_name,
                                                arguments);
  ligma_value_array_unref (arguments);

  return return_values;
}

/**
 * ligma_pdb_run_procedure_argv: (rename-to ligma_pdb_run_procedure)
 * @pdb:                                              the #LigmaPDB object.
 * @procedure_name:                                   the registered name to call.
 * @arguments: (array length=n_arguments) (nullable): the call arguments or %NULL.
 * @n_arguments:                                      the number of arguments.
 *
 * Runs the procedure named @procedure_name with @arguments.
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
LigmaValueArray *
ligma_pdb_run_procedure_argv (LigmaPDB      *pdb,
                             const gchar  *procedure_name,
                             const GValue *arguments,
                             gint          n_arguments)
{
  LigmaValueArray *args;
  LigmaValueArray *return_values;

  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (procedure_name), NULL);
  /* Not require arguments != NULL.
   * ligma_value_array_new_from_values(NULL, 0) will return empty GValueArray.
   */

  args = ligma_value_array_new_from_values (arguments, n_arguments);
  return_values = ligma_pdb_run_procedure_array (pdb, procedure_name, args);
  ligma_value_array_unref (args);

  return return_values;
}

/**
 * ligma_pdb_run_procedure_array:
 * @pdb:            the #LigmaPDB object.
 * @procedure_name: the procedure registered name.
 * @arguments:      the call arguments.
 *
 * Runs the procedure named @procedure_name with @arguments.
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
LigmaValueArray *
ligma_pdb_run_procedure_array (LigmaPDB              *pdb,
                              const gchar          *procedure_name,
                              const LigmaValueArray *arguments)
{
  GPProcRun        proc_run;
  GPProcReturn    *proc_return;
  LigmaWireMessage  msg;
  LigmaValueArray  *return_values;

  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (procedure_name), NULL);
  g_return_val_if_fail (arguments != NULL, NULL);

  proc_run.name     = (gchar *) procedure_name;
  proc_run.n_params = ligma_value_array_length (arguments);
  proc_run.params   = _ligma_value_array_to_gp_params (arguments, FALSE);

  if (! gp_proc_run_write (_ligma_plug_in_get_write_channel (pdb->priv->plug_in),
                           &proc_run, pdb->priv->plug_in))
    ligma_quit ();

  _ligma_gp_params_free (proc_run.params, proc_run.n_params, FALSE);

  _ligma_plug_in_read_expect_msg (pdb->priv->plug_in, &msg, GP_PROC_RETURN);

  proc_return = msg.data;

  return_values = _ligma_gp_params_to_value_array (NULL,
                                                  NULL, 0,
                                                  proc_return->params,
                                                  proc_return->n_params,
                                                  TRUE);

  ligma_wire_destroy (&msg);

  ligma_pdb_set_error (pdb, return_values);

  return return_values;
}

/**
 * ligma_pdb_run_procedure_config:
 * @pdb:            the #LigmaPDB object.
 * @procedure_name: the registered name to call.
 * @config:         a config object obtained with ligma_procedure_create_config().
 *
 * Runs the procedure named @procedure_name with @config.
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
LigmaValueArray *
ligma_pdb_run_procedure_config (LigmaPDB             *pdb,
                               const gchar         *procedure_name,
                               LigmaProcedureConfig *config)
{
  LigmaProcedure  *procedure;
  LigmaValueArray *args;
  LigmaValueArray *return_values;

  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (procedure_name), NULL);
  g_return_val_if_fail (LIGMA_IS_PROCEDURE_CONFIG (config), NULL);

  procedure = ligma_pdb_lookup_procedure (pdb, procedure_name);

  g_return_val_if_fail (ligma_procedure_config_get_procedure (config) == procedure,
                        NULL);

  args = ligma_procedure_new_arguments (procedure);

  ligma_procedure_config_get_values (config, args);
  return_values = ligma_pdb_run_procedure_array (pdb, procedure_name, args);

  ligma_value_array_unref (args);

  return return_values;
}

/**
 * ligma_pdb_temp_procedure_name:
 * @pdb: the #LigmaPDB object.
 *
 * Generates a unique temporary PDB name.
 *
 * This function generates a temporary PDB entry name that is
 * guaranteed to be unique.
 *
 * Returns: (transfer full): A unique temporary name for a temporary
 *          PDB entry. The returned value must be freed with
 *          g_free().
 *
 * Since: 3.0
 **/
gchar *
ligma_pdb_temp_procedure_name (LigmaPDB *pdb)
{
  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);

  return _ligma_pdb_temp_name ();
}

/**
 * ligma_pdb_dump_to_file:
 * @pdb:  A #LigmaPDB.
 * @file: The dump file.
 *
 * Dumps the current contents of the procedural database
 *
 * This procedure dumps the contents of the procedural database to the
 * specified @file. The file will contain all of the information
 * provided for each registered procedure.
 *
 * Returns: TRUE on success.
 *
 * Since: 3.0
 **/
gboolean
ligma_pdb_dump_to_file (LigmaPDB *pdb,
                       GFile   *file)
{
  g_return_val_if_fail (LIGMA_IS_PDB (pdb), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  return _ligma_pdb_dump (file);
}

/**
 * ligma_pdb_query_procedures:
 * @pdb:         A #LigmaPDB.
 * @name:        The regex for procedure name.
 * @blurb:       The regex for procedure blurb.
 * @help:        The regex for procedure help.
 * @help_id:     The regex for procedure help-id.
 * @authors:     The regex for procedure authors.
 * @copyright:   The regex for procedure copyright.
 * @date:        The regex for procedure date.
 * @proc_type:   The regex for procedure type: { 'Internal LIGMA procedure', 'LIGMA Plug-in', 'LIGMA Extension', 'Temporary Procedure' }.
 *
 * Queries the procedural database for its contents using regular
 * expression matching.
 *
 * This function queries the contents of the procedural database. It
 * is supplied with eight arguments matching procedures on
 *
 * { name, blurb, help, help-id, authors, copyright, date, procedure type}.
 *
 * This is accomplished using regular expression matching. For
 * instance, to find all procedures with "jpeg" listed in the blurb,
 * all seven arguments can be supplied as ".*", except for the second,
 * which can be supplied as ".*jpeg.*". There are two return arguments
 * for this procedure. The first is the number of procedures matching
 * the query. The second is a concatenated list of procedure names
 * corresponding to those matching the query. If no matching entries
 * are found, then the returned string is NULL and the number of
 * entries is 0.
 *
 * Returns: (array zero-terminated=1) (transfer full): The list
 *          of procedure names. Free with g_strfreev().
 *
 * Since: 3.0
 **/
gchar **
ligma_pdb_query_procedures (LigmaPDB     *pdb,
                           const gchar *name,
                           const gchar *blurb,
                           const gchar *help,
                           const gchar *help_id,
                           const gchar *authors,
                           const gchar *copyright,
                           const gchar *date,
                           const gchar *proc_type)
{
  gchar **matches;

  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);

  _ligma_pdb_query (name,
                   blurb, help, /* FIXME help_id */
                   authors, copyright, date,
                   proc_type,
                   &matches);

  return matches;
}

GQuark
_ligma_pdb_error_quark (void)
{
  return g_quark_from_static_string ("ligma-pdb-error-quark");
}


/*  Temporary API, to go away before 3.0  */

/**
 * ligma_pdb_get_last_error:
 * @pdb: a #LigmaPDB.
 *
 * Retrieves the error message from the last procedure call.
 *
 * If a procedure call fails, then it might pass an error message with
 * the return values. Plug-ins that are using the libligma C wrappers
 * don't access the procedure return values directly. Thus #LigmaPDB
 * stores the error message and makes it available with this
 * function. The next procedure call unsets the error message again.
 *
 * The returned string is owned by @pdb and must not be freed or
 * modified.
 *
 * Returns: the error message
 *
 * Since: 3.0
 **/
const gchar *
ligma_pdb_get_last_error (LigmaPDB *pdb)
{
  g_return_val_if_fail (LIGMA_IS_PDB (pdb), NULL);

  if (pdb->priv->error_message && strlen (pdb->priv->error_message))
    return pdb->priv->error_message;

  switch (pdb->priv->error_status)
    {
    case LIGMA_PDB_SUCCESS:
      /*  procedure executed successfully  */
      return _("success");

    case LIGMA_PDB_EXECUTION_ERROR:
      /*  procedure execution failed       */
      return _("execution error");

    case LIGMA_PDB_CALLING_ERROR:
      /*  procedure called incorrectly     */
      return _("calling error");

    case LIGMA_PDB_CANCEL:
      /*  procedure execution cancelled    */
      return _("cancelled");

    default:
      return "invalid return status";
    }
}

/**
 * ligma_pdb_get_last_status:
 * @pdb: a #LigmaPDB.
 *
 * Retrieves the status from the last procedure call.
 *
 * Returns: the #LigmaPDBStatusType.
 *
 * Since: 3.0
 **/
LigmaPDBStatusType
ligma_pdb_get_last_status (LigmaPDB *pdb)
{
  g_return_val_if_fail (LIGMA_IS_PDB (pdb), LIGMA_PDB_SUCCESS);

  return pdb->priv->error_status;
}

/*  Cruft API  */

/**
 * ligma_pdb_get_data:
 * @identifier: The identifier associated with data.
 * @data: A byte array containing data.
 *
 * Returns data associated with the specified identifier.
 *
 * This procedure returns any data which may have been associated with
 * the specified identifier. The data is copied into the given memory
 * location.
 *
 * Returns: TRUE on success, FALSE if no data has been associated with
 * the identifier
 */
gboolean
ligma_pdb_get_data (const gchar *identifier,
                   gpointer     data)
{
  gint      size;
  guint8   *hack;
  gboolean  success;

  success = _ligma_pdb_get_data (identifier, &size, &hack);

  if (hack)
    {
      memcpy (data, (gconstpointer) hack, size * sizeof (guint8));
      g_free (hack);
    }

  return success;
}

/**
 * ligma_pdb_get_data_size:
 * @identifier: The identifier associated with data.
 *
 * Returns size of data associated with the specified identifier.
 *
 * This procedure returns the size of any data which may have been
 * associated with the specified identifier. If no data has been
 * associated with the identifier, an error is returned.
 *
 * Returns: The number of bytes in the data.
 **/
gint
ligma_pdb_get_data_size (const gchar *identifier)
{
  return _ligma_pdb_get_data_size (identifier);
}

/**
 * ligma_pdb_set_data:
 * @identifier: The identifier associated with data.
 * @data: A byte array containing data.
 * @bytes: The number of bytes in the data
 *
 * Associates the specified identifier with the supplied data.
 *
 * This procedure associates the supplied data with the provided
 * identifier. The data may be subsequently retrieved by a call to
 * 'procedural-db-get-data'.
 *
 * Returns: TRUE on success.
 */
gboolean
ligma_pdb_set_data (const gchar   *identifier,
                   gconstpointer  data,
                   guint32        bytes)
{
  return _ligma_pdb_set_data (identifier, bytes, data);
}


/*  private functions  */

static void
ligma_pdb_set_error (LigmaPDB        *pdb,
                    LigmaValueArray *return_values)
{
  g_clear_pointer (&pdb->priv->error_message, g_free);
  pdb->priv->error_status = LIGMA_PDB_SUCCESS;

  if (ligma_value_array_length (return_values) > 0)
    {
      pdb->priv->error_status = LIGMA_VALUES_GET_ENUM (return_values, 0);

      switch (pdb->priv->error_status)
        {
        case LIGMA_PDB_SUCCESS:
        case LIGMA_PDB_PASS_THROUGH:
          break;

        case LIGMA_PDB_EXECUTION_ERROR:
        case LIGMA_PDB_CALLING_ERROR:
        case LIGMA_PDB_CANCEL:
          if (ligma_value_array_length (return_values) > 1)
            {
              GValue *value = ligma_value_array_index (return_values, 1);

              if (G_VALUE_HOLDS_STRING (value))
                pdb->priv->error_message = g_value_dup_string (value);
            }
          break;
        }
    }
}
