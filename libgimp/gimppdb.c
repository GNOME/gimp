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
#include "gimppdbprocedure.h"
#include "gimpplugin-private.h"


/**
 * SECTION: gimppdb
 * @title: GimpPDB
 * @short_description: Functions for querying and changing procedural
 *                     database (PDB) entries.
 *
 * Functions for querying and changing procedural database (PDB)
 * entries.
 **/


struct _GimpPDBPrivate
{
  GimpPlugIn *plug_in;

  GHashTable *procedures;
};


static void   gimp_pdb_finalize (GObject *object);


G_DEFINE_TYPE_WITH_PRIVATE (GimpPDB, gimp_pdb, G_TYPE_OBJECT)

#define parent_class gimp_pdb_parent_class


static void
gimp_pdb_class_init (GimpPDBClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_pdb_finalize;
}

static void
gimp_pdb_init (GimpPDB *pdb)
{
  pdb->priv = gimp_pdb_get_instance_private (pdb);

  pdb->priv->procedures = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                 g_free, g_object_unref);
}

static void
gimp_pdb_finalize (GObject *object)
{
  GimpPDB *pdb = GIMP_PDB (object);

  g_clear_object (&pdb->priv->plug_in);
  g_clear_pointer (&pdb->priv->procedures, g_hash_table_unref);

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
 * @pdb:            A #GimpPDB instance.
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
  g_return_val_if_fail (procedure_name != NULL, FALSE);

  return _gimp_pdb_proc_exists (procedure_name);
}

GimpProcedure *
gimp_pdb_lookup_procedure (GimpPDB     *pdb,
                           const gchar *procedure_name)
{
  GimpProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (procedure_name != NULL, NULL);

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

GimpValueArray *
gimp_pdb_run_procedure (GimpPDB     *pdb,
                        const gchar *procedure_name,
                        GType        first_type,
                        ...)
{
  GimpValueArray *return_values;
  va_list         args;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (procedure_name != NULL, NULL);

  va_start (args, first_type);

  return_values = gimp_pdb_run_procedure_valist (pdb, procedure_name,
                                                 first_type, args);

  va_end (args);

  return return_values;
}

GimpValueArray *
gimp_pdb_run_procedure_valist (GimpPDB     *pdb,
                               const gchar *procedure_name,
                               GType        first_type,
                               va_list      args)
{
  GimpValueArray *arguments;
  GimpValueArray *return_values;
  GType           type;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (procedure_name != NULL, NULL);

  arguments = gimp_value_array_new (0);

  type = first_type;

  while (type != G_TYPE_NONE)
    {
      GValue  value     = G_VALUE_INIT;
      gchar  *error_msg = NULL;

      g_value_init (&value, type);

      G_VALUE_COLLECT (&value, args, G_VALUE_NOCOPY_CONTENTS, &error_msg);

      if (error_msg)
        {
          GError *error = g_error_new_literal (GIMP_PDB_ERROR,
                                               GIMP_PDB_ERROR_INTERNAL_ERROR,
                                               error_msg);
          g_printerr ("%s: %s", G_STRFUNC, error_msg);
          g_free (error_msg);

          gimp_value_array_unref (arguments);

          return_values = gimp_procedure_new_return_values (NULL,
                                                            GIMP_PDB_CALLING_ERROR,
                                                            error);
          va_end (args);

          return return_values;
        }

      gimp_value_array_append (arguments, &value);
      g_value_unset (&value);

      type = va_arg (args, GType);
    }

  return_values = gimp_pdb_run_procedure_array (pdb, procedure_name,
                                                arguments);
  gimp_value_array_unref (arguments);

  return return_values;
}

/**
 * gimp_pdb_run_procedure_array: (rename-to gimp_pdb_run_procedure)
 * @pdb:            the #GimpPDB object.
 * @procedure_name: the procedure registered name.
 * @arguments:      the call arguments.
 *
 * Runs the procedure names @procedure_name with @arguments.
 *
 * Returns: (transfer full): the returned values for the procedure call.
 */
GimpValueArray *
gimp_pdb_run_procedure_array (GimpPDB              *pdb,
                              const gchar          *procedure_name,
                              const GimpValueArray *arguments)
{
  GPProcRun        proc_run;
  GPProcReturn    *proc_return;
  GimpWireMessage  msg;
  GimpValueArray  *return_values;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (procedure_name != NULL, NULL);
  g_return_val_if_fail (arguments != NULL, NULL);

  proc_run.name    = (gchar *) procedure_name;
  proc_run.nparams = gimp_value_array_length (arguments);
  proc_run.params  = _gimp_value_array_to_gp_params (arguments, FALSE);

  if (! gp_proc_run_write (_gimp_plug_in_get_write_channel (pdb->priv->plug_in),
                           &proc_run, pdb->priv->plug_in))
    gimp_quit ();

  _gimp_plug_in_read_expect_msg (pdb->priv->plug_in, &msg, GP_PROC_RETURN);

  proc_return = msg.data;

  return_values = _gimp_gp_params_to_value_array (NULL,
                                                  NULL, 0,
                                                  proc_return->params,
                                                  proc_return->nparams,
                                                  TRUE, FALSE);

  gimp_wire_destroy (&msg);

  _gimp_set_pdb_error (return_values);

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

GQuark
_gimp_pdb_error_quark (void)
{
  return g_quark_from_static_string ("gimp-pdb-error-quark");
}


/*  Cruft API  */

/**
 * gimp_pdb_temp_name:
 *
 * Generates a unique temporary PDB name.
 *
 * This procedure generates a temporary PDB entry name that is
 * guaranteed to be unique.
 *
 * Returns: (transfer full): A unique temporary name for a temporary PDB entry.
 *          The returned value must be freed with g_free().
 **/
gchar *
gimp_pdb_temp_name (void)
{
  return _gimp_pdb_temp_name ();
}

/**
 * gimp_pdb_proc_exists:
 * @procedure_name: The procedure name.
 *
 * Checks if the specified procedure exists in the procedural database
 *
 * This procedure checks if the specified procedure is registered in
 * the procedural database.
 *
 * Returns: Whether a procedure of that name is registered.
 *
 * Since: 2.6
 **/
gboolean
gimp_pdb_proc_exists (const gchar *procedure_name)
{
  return _gimp_pdb_proc_exists (procedure_name);
}

/**
 * gimp_pdb_proc_info:
 * @procedure: The procedure name.
 * @blurb: A short blurb.
 * @help: Detailed procedure help.
 * @author: Author(s) of the procedure.
 * @copyright: The copyright.
 * @date: Copyright date.
 * @proc_type: The procedure type.
 * @num_args: The number of input arguments.
 * @num_values: The number of return values.
 * @args: The input arguments.
 * @return_vals: The return values.
 *
 * Queries the procedural database for information on the specified
 * procedure.
 *
 * This procedure returns information on the specified procedure. A
 * short blurb, detailed help, author(s), copyright information,
 * procedure type, number of input, and number of return values are
 * returned. Additionally this function returns specific information
 * about each input argument and return value.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_pdb_proc_info (const gchar      *procedure_name,
                    gchar           **blurb,
                    gchar           **help,
                    gchar           **author,
                    gchar           **copyright,
                    gchar           **date,
                    GimpPDBProcType  *proc_type,
                    gint             *num_args,
                    gint             *num_values,
                    GimpParamDef    **args,
                    GimpParamDef    **return_vals)
{
  gint i;
  gboolean success = TRUE;

  success = _gimp_pdb_proc_info (procedure_name,
                                 blurb,
                                 help,
                                 author,
                                 copyright,
                                 date,
                                 proc_type,
                                 num_args,
                                 num_values);

  if (success)
    {
      *args        = g_new (GimpParamDef, *num_args);
      *return_vals = g_new (GimpParamDef, *num_values);

      for (i = 0; i < *num_args; i++)
        {
          if (! gimp_pdb_proc_arg (procedure_name, i,
                                   &(*args)[i].type,
                                   &(*args)[i].name,
                                   &(*args)[i].description))
            {
              g_free (*args);
              g_free (*return_vals);

              return FALSE;
            }
        }

      for (i = 0; i < *num_values; i++)
        {
          if (! gimp_pdb_proc_val (procedure_name, i,
                                   &(*return_vals)[i].type,
                                   &(*return_vals)[i].name,
                                   &(*return_vals)[i].description))
            {
              g_free (*args);
              g_free (*return_vals);

              return FALSE;
            }
        }
    }

  return success;
}

/**
 * gimp_pdb_proc_arg:
 * @procedure_name: The procedure name.
 * @arg_num: The argument number.
 * @arg_type: (out): The type of argument.
 * @arg_name: (out) (transfer full): The name of the argument.
 * @arg_desc: (out) (transfer full): A description of the argument.
 *
 * Queries the procedural database for information on the specified
 * procedure's argument.
 *
 * This procedure returns information on the specified procedure's
 * argument. The argument type, name, and a description are retrieved.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_pdb_proc_arg (const gchar     *procedure_name,
                   gint             arg_num,
                   GimpPDBArgType  *arg_type,
                   gchar          **arg_name,
                   gchar          **arg_desc)
{
  return _gimp_pdb_proc_arg (procedure_name, arg_num,
                             arg_type, arg_name, arg_desc);
}

/**
 * gimp_pdb_proc_val:
 * @procedure_name: The procedure name.
 * @val_num: The return value number.
 * @val_type: (out): The type of return value.
 * @val_name: (out) (transfer full): The name of the return value.
 * @val_desc: (out) (transfer full): A description of the return value.
 *
 * Queries the procedural database for information on the specified
 * procedure's return value.
 *
 * This procedure returns information on the specified procedure's
 * return value. The return value type, name, and a description are
 * retrieved.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_pdb_proc_val (const gchar     *procedure_name,
                   gint             val_num,
                   GimpPDBArgType  *val_type,
                   gchar          **val_name,
                   gchar          **val_desc)
{
  return _gimp_pdb_proc_val (procedure_name, val_num,
                             val_type, val_name, val_desc);
}

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
  gint      size;
  guint8   *hack;
  gboolean  success;

  success = _gimp_pdb_get_data (identifier, &size, &hack);

  if (hack)
    {
      memcpy (data, (gconstpointer) hack, size * sizeof (guint8));
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
gimp_pdb_set_data (const gchar   *identifier,
                   gconstpointer  data,
                   guint32        bytes)
{
  return _gimp_pdb_set_data (identifier, bytes, data);
}
