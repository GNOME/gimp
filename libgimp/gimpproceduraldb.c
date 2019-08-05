/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpproceduraldb.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h> /* memcmp */

#include "gimp.h"

/**
 * gimp_procedural_db_proc_info:
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
gimp_procedural_db_proc_info (const gchar      *procedure,
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

  success = _gimp_procedural_db_proc_info (procedure,
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
          if (! gimp_procedural_db_proc_arg (procedure,
                                             i,
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
          if (! gimp_procedural_db_proc_val (procedure,
                                             i,
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
 * gimp_procedural_db_get_data:
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
gimp_procedural_db_get_data (const gchar *identifier,
                             gpointer     data)
{
  gint      size;
  guint8   *hack;
  gboolean  success;

  success = _gimp_procedural_db_get_data (identifier,
                                          &size,
                                          &hack);
  if (hack)
    {
      memcpy (data, (gconstpointer) hack, size * sizeof (guint8));
      g_free (hack);
    }

  return success;
}

/**
 * gimp_procedural_db_set_data:
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
gimp_procedural_db_set_data (const gchar   *identifier,
                             gconstpointer  data,
                             guint32        bytes)
{
  return _gimp_procedural_db_set_data (identifier,
                                       bytes,
                                       data);
}
