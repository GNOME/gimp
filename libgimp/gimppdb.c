/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppdb.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#include <gobject/gvaluecollector.h>

#include "gimp.h"

#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "gimp-private.h"
#include "gimpgpparams.h"
#include "gimppdb-private.h"
#include "gimppdb_pdb.h"
#include "gimppdbprocedure.h"
#include "gimpplugin-private.h"
#include "gimpprocedureconfig-private.h"

#include "libgimp-intl.h"


/**
 * GimpPDB:
 *
 * Provides access to the Procedural DataBase (PDB).
 */


struct _GimpPDBPrivate
{
  GimpPlugIn         *plug_in;

  GHashTable         *procedures;

  GimpPDBStatusType   error_status;
  gchar              *error_message;
};


static void   gimp_pdb_dispose   (GObject        *object);
static void   gimp_pdb_finalize  (GObject        *object);

static void   gimp_pdb_set_error (GimpPDB        *pdb,
                                  GimpValueArray *return_values);


G_DEFINE_TYPE_WITH_PRIVATE (GimpPDB, gimp_pdb, G_TYPE_OBJECT)

#define parent_class gimp_pdb_parent_class


static void
gimp_pdb_class_init (GimpPDBClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = gimp_pdb_dispose;
  object_class->finalize = gimp_pdb_finalize;
}

static void
gimp_pdb_init (GimpPDB *pdb)
{
  pdb->priv = gimp_pdb_get_instance_private (pdb);

  pdb->priv->procedures = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                 g_free, g_object_unref);

  pdb->priv->error_status = GIMP_PDB_SUCCESS;
}

static void
gimp_pdb_dispose (GObject *object)
{
  GimpPDB *pdb = GIMP_PDB (object);

  g_clear_pointer (&pdb->priv->procedures, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_pdb_finalize (GObject *object)
{
  GimpPDB *pdb = GIMP_PDB (object);

  g_clear_object (&pdb->priv->plug_in);
  g_clear_pointer (&pdb->priv->error_message, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpPDB *
_gimp_pdb_new (GimpPlugIn *plug_in)
{
  GimpPDB *pdb;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  pdb = g_object_new (GIMP_TYPE_PDB, NULL);

  pdb->priv->plug_in = g_object_ref (plug_in);

  return pdb;
}

GimpPlugIn *
_gimp_pdb_get_plug_in (GimpPDB *pdb)
{
  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);

  return pdb->priv->plug_in;
}

/**
 * gimp_pdb_procedure_exists:
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
gimp_pdb_procedure_exists (GimpPDB     *pdb,
                           const gchar *procedure_name)
{
  g_return_val_if_fail (GIMP_IS_PDB (pdb), FALSE);
  g_return_val_if_fail (gimp_is_canonical_identifier (procedure_name), FALSE);

  return _gimp_pdb_proc_exists (procedure_name);
}

/**
 * gimp_pdb_lookup_procedure:
 * @pdb:            A #GimpPDB instance.
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
GimpProcedure *
gimp_pdb_lookup_procedure (GimpPDB     *pdb,
                           const gchar *procedure_name)
{
  GimpProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (procedure_name), NULL);

  procedure = g_hash_table_lookup (pdb->priv->procedures, procedure_name);

  if (! procedure)
    {
      procedure = _gimp_pdb_procedure_new (pdb, procedure_name);

      if (procedure)
        g_hash_table_insert (pdb->priv->procedures,
                             g_strdup (procedure_name), procedure);
    }

  return procedure;
}

/**
 * gimp_pdb_run_procedure: (skip)
 * @pdb:            the #GimpPDB object.
 * @procedure_name: the procedure registered name.
 * @first_arg_name: the name of an argument of @procedure_name.
 * @...:            the call arguments.
 *
 * Runs the procedure named @procedure_name with arguments given as
 * list of `(name, value)` pairs, terminated by %NULL.
 *
 * The order of arguments does not matter and if any argument is missing, its
 * default value will be used. The value type must correspond to the argument
 * type as registered for @procedure_name.
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
GimpValueArray *
gimp_pdb_run_procedure (GimpPDB     *pdb,
                        const gchar *procedure_name,
                        const gchar *first_arg_name,
                        ...)
{
  GimpValueArray *return_values;
  va_list         args;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (procedure_name), NULL);

  va_start (args, first_arg_name);

  return_values = gimp_pdb_run_procedure_valist (pdb, procedure_name,
                                                 first_arg_name, args);

  va_end (args);

  return return_values;
}

/**
 * gimp_pdb_run_procedure_valist: (skip)
 * @pdb:            the #GimpPDB object.
 * @procedure_name: the procedure registered name.
 * @first_arg_name: the name of an argument of @procedure_name.
 * @args:           the call arguments.
 *
 * Runs the procedure named @procedure_name with arguments name, type and value
 * given in the order as passed to [method@PDB.run_procedure].
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
GimpValueArray *
gimp_pdb_run_procedure_valist (GimpPDB     *pdb,
                               const gchar *procedure_name,
                               const gchar *first_arg_name,
                               va_list      args)
{
  GimpValueArray      *return_values;
  GimpProcedure       *procedure;
  GimpProcedureConfig *config;
  const gchar         *arg_name;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (procedure_name), NULL);

  procedure = gimp_pdb_lookup_procedure (pdb, procedure_name);
  config    = gimp_procedure_create_config (procedure);

  arg_name = first_arg_name;

  while (arg_name != NULL)
    {
      GParamSpec *pspec;
      gchar      *error = NULL;
      GValue      value = G_VALUE_INIT;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), arg_name);

      if (pspec == NULL)
        {
          g_warning ("%s: %s has no property named '%s'",
                     G_STRFUNC,
                     g_type_name (G_TYPE_FROM_INSTANCE (config)),
                     arg_name);
          g_clear_object (&config);
          return NULL;
        }

      g_value_init (&value, pspec->value_type);
      G_VALUE_COLLECT (&value, args, G_VALUE_NOCOPY_CONTENTS, &error);

      if (error)
        {
          g_warning ("%s: %s", G_STRFUNC, error);
          g_free (error);
          g_clear_object (&config);
          return NULL;
        }

      g_object_set_property (G_OBJECT (config), arg_name, &value);
      g_value_unset (&value);

      arg_name = va_arg (args, const gchar *);
    }

  return_values = gimp_pdb_run_procedure_config (pdb, procedure_name, config);
  g_clear_object (&config);

  return return_values;
}

/**
 * gimp_pdb_run_procedure_config:
 * @pdb:            the #GimpPDB object.
 * @procedure_name: the registered name to call.
 * @config:         a config object obtained with gimp_procedure_create_config().
 *
 * Runs the procedure named @procedure_name with @config.
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
GimpValueArray *
gimp_pdb_run_procedure_config (GimpPDB             *pdb,
                               const gchar         *procedure_name,
                               GimpProcedureConfig *config)
{
  GimpProcedure  *procedure;
  GimpValueArray *args;
  GimpValueArray *return_values;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (procedure_name), NULL);
  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), NULL);

  procedure = gimp_pdb_lookup_procedure (pdb, procedure_name);

  g_return_val_if_fail (gimp_procedure_config_get_procedure (config) == procedure,
                        NULL);

  args = gimp_procedure_new_arguments (procedure);

  _gimp_procedure_config_get_values (config, args);
  return_values = _gimp_pdb_run_procedure_array (pdb, procedure_name, args);

  gimp_value_array_unref (args);

  return return_values;
}

/**
 * gimp_pdb_temp_procedure_name:
 * @pdb: the #GimpPDB object.
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
gimp_pdb_temp_procedure_name (GimpPDB *pdb)
{
  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);

  return _gimp_pdb_temp_name ();
}

/**
 * gimp_pdb_dump_to_file:
 * @pdb:  A #GimpPDB.
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
gimp_pdb_dump_to_file (GimpPDB *pdb,
                       GFile   *file)
{
  g_return_val_if_fail (GIMP_IS_PDB (pdb), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  return _gimp_pdb_dump (file);
}

/**
 * gimp_pdb_query_procedures:
 * @pdb:         A #GimpPDB.
 * @name:        The regex for procedure name.
 * @blurb:       The regex for procedure blurb.
 * @help:        The regex for procedure help.
 * @help_id:     The regex for procedure help-id.
 * @authors:     The regex for procedure authors.
 * @copyright:   The regex for procedure copyright.
 * @date:        The regex for procedure date.
 * @proc_type:   The regex for procedure type: { 'Internal GIMP procedure', 'GIMP Plug-in', 'GIMP Extension', 'Temporary Procedure' }.
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
gimp_pdb_query_procedures (GimpPDB     *pdb,
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

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);

  _gimp_pdb_query (name,
                   blurb, help, /* FIXME help_id */
                   authors, copyright, date,
                   proc_type,
                   &matches);

  return matches;
}

GQuark
_gimp_pdb_error_quark (void)
{
  return g_quark_from_static_string ("gimp-pdb-error-quark");
}


/*  Temporary API, to go away before 3.0  */

/**
 * gimp_pdb_get_last_error:
 * @pdb: a #GimpPDB.
 *
 * Retrieves the error message from the last procedure call.
 *
 * If a procedure call fails, then it might pass an error message with
 * the return values. Plug-ins that are using the libgimp C wrappers
 * don't access the procedure return values directly. Thus #GimpPDB
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
gimp_pdb_get_last_error (GimpPDB *pdb)
{
  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);

  if (pdb->priv->error_message && strlen (pdb->priv->error_message))
    return pdb->priv->error_message;

  switch (pdb->priv->error_status)
    {
    case GIMP_PDB_SUCCESS:
      /*  procedure executed successfully  */
      return _("success");

    case GIMP_PDB_EXECUTION_ERROR:
      /*  procedure execution failed       */
      return _("execution error");

    case GIMP_PDB_CALLING_ERROR:
      /*  procedure called incorrectly     */
      return _("calling error");

    case GIMP_PDB_CANCEL:
      /*  procedure execution cancelled    */
      return _("cancelled");

    default:
      return "invalid return status";
    }
}

/**
 * gimp_pdb_get_last_status:
 * @pdb: a #GimpPDB.
 *
 * Retrieves the status from the last procedure call.
 *
 * Returns: the #GimpPDBStatusType.
 *
 * Since: 3.0
 **/
GimpPDBStatusType
gimp_pdb_get_last_status (GimpPDB *pdb)
{
  g_return_val_if_fail (GIMP_IS_PDB (pdb), GIMP_PDB_SUCCESS);

  return pdb->priv->error_status;
}

/*  Cruft API  */

/**
 * gimp_pdb_get_data:
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
gimp_pdb_get_data (const gchar *identifier,
                   gpointer     data)
{
  GBytes   *hack = NULL;
  gboolean  success;

  success = _gimp_pdb_get_data (identifier, &hack);

  if (hack)
    {
      memcpy (data, g_bytes_get_data (hack, NULL), g_bytes_get_size (hack));
      g_free (hack);
    }

  return success;
}

/**
 * gimp_pdb_get_data_size:
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
gimp_pdb_get_data_size (const gchar *identifier)
{
  return _gimp_pdb_get_data_size (identifier);
}

/**
 * gimp_pdb_set_data:
 * @identifier: The identifier associated with data.
 * @data: (array length=data_len): A byte array containing data.
 * @data_len: The number of bytes in the data
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
gimp_pdb_set_data (const gchar   *identifier,
                   gconstpointer  data,
                   guint32        data_len)
{
  GBytes   *bytes;
  gboolean  ret;

  bytes = g_bytes_new_static (data, data_len);
  ret = _gimp_pdb_set_data (identifier, bytes);
  g_bytes_unref (bytes);

  return ret;
}

/**
 * _gimp_pdb_run_procedure_array:
 * @pdb:            the #GimpPDB object.
 * @procedure_name: the procedure registered name.
 * @arguments:      the call arguments.
 *
 * Runs the procedure named @procedure_name with @arguments.
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
GimpValueArray *
_gimp_pdb_run_procedure_array (GimpPDB              *pdb,
                               const gchar          *procedure_name,
                               const GimpValueArray *arguments)
{
  GPProcRun        proc_run;
  GPProcReturn    *proc_return;
  GimpWireMessage  msg;
  GimpValueArray  *return_values;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (procedure_name), NULL);
  g_return_val_if_fail (arguments != NULL, NULL);

  proc_run.name     = (gchar *) procedure_name;
  proc_run.n_params = gimp_value_array_length (arguments);
  proc_run.params   = _gimp_value_array_to_gp_params (arguments, FALSE);

  if (! gp_proc_run_write (_gimp_plug_in_get_write_channel (pdb->priv->plug_in),
                           &proc_run, pdb->priv->plug_in))
    gimp_quit ();

  _gimp_gp_params_free (proc_run.params, proc_run.n_params, FALSE);

  _gimp_plug_in_read_expect_msg (pdb->priv->plug_in, &msg, GP_PROC_RETURN);

  proc_return = msg.data;

  return_values = _gimp_gp_params_to_value_array (NULL,
                                                  NULL, 0,
                                                  proc_return->params,
                                                  proc_return->n_params,
                                                  TRUE);

  gimp_wire_destroy (&msg);

  gimp_pdb_set_error (pdb, return_values);

  return return_values;
}


/*  private functions  */

static void
gimp_pdb_set_error (GimpPDB        *pdb,
                    GimpValueArray *return_values)
{
  g_clear_pointer (&pdb->priv->error_message, g_free);
  pdb->priv->error_status = GIMP_PDB_SUCCESS;

  if (gimp_value_array_length (return_values) > 0)
    {
      pdb->priv->error_status = GIMP_VALUES_GET_ENUM (return_values, 0);

      switch (pdb->priv->error_status)
        {
        case GIMP_PDB_SUCCESS:
        case GIMP_PDB_PASS_THROUGH:
          break;

        case GIMP_PDB_EXECUTION_ERROR:
        case GIMP_PDB_CALLING_ERROR:
        case GIMP_PDB_CANCEL:
          if (gimp_value_array_length (return_values) > 1)
            {
              GValue *value = gimp_value_array_index (return_values, 1);

              if (G_VALUE_HOLDS_STRING (value))
                pdb->priv->error_message = g_value_dup_string (value);
            }
          break;
        }
    }
}
