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

#include "gimp.h"
#include "gimppdb-private.h"
#include "gimppdbprocedure.h"


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

GimpProcedure *
gimp_pdb_lookup (GimpPDB     *pdb,
                 const gchar *name)
{
  GimpProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PDB (pdb), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  procedure = g_hash_table_lookup (pdb->priv->procedures, name);

  if (! procedure)
    {
      procedure = _gimp_pdb_procedure_new (pdb, name);

      if (procedure)
        g_hash_table_insert (pdb->priv->procedures,
                             g_strdup (name), procedure);
    }

  return procedure;
}

GQuark
_gimp_pdb_error_quark (void)
{
  return g_quark_from_static_string ("gimp-pdb-error-quark");
}


/*  Cruft API  */

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
gimp_pdb_proc_info (const gchar      *procedure,
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

  success = _gimp_pdb_proc_info (procedure,
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
          if (! gimp_pdb_proc_arg (procedure, i,
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
          if (! gimp_pdb_proc_val (procedure, i,
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
