/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpproceduraldb.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gimp.h"

gboolean
gimp_procedural_db_proc_info (gchar            *procedure,
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

gboolean
gimp_procedural_db_get_data (gchar    *identifier,
			     gpointer  data)
{
  gint      size;
  guint8   *hack;
  gboolean  success;

  success = _gimp_procedural_db_get_data (identifier,
					  &size,
					  &hack);
  if (hack)
    {
      memcpy (data, (gpointer) hack, size * sizeof (guint8));
      g_free (hack);
    }
  
  return success;
}

gboolean
gimp_procedural_db_set_data (gchar    *identifier,
			     gpointer  data,
			     guint32   bytes)
{
  return _gimp_procedural_db_set_data (identifier,
				       bytes,
				       data);
}
