/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "pdb-types.h"

#include "core/gimpparamspecs.h"

#include "gimpargument.h"


/*  public functions  */

void
gimp_argument_init (Argument *arg,
                    ProcArg  *proc_arg)
{
  g_return_if_fail (arg != NULL);
  g_return_if_fail (proc_arg != NULL);

  arg->type = proc_arg->type;
  g_value_init (&arg->value, proc_arg->pspec->value_type);
}

void
gimp_argument_init_compat (Argument       *arg,
                           GimpPDBArgType  arg_type)
{
  g_return_if_fail (arg != NULL);

  arg->type = arg_type;

  switch (arg_type)
    {
    case GIMP_PDB_INT32:
    case GIMP_PDB_INT16:
      g_value_init (&arg->value, G_TYPE_INT);
      break;

    case GIMP_PDB_INT8:
      g_value_init (&arg->value, G_TYPE_UINT);
      break;

    case GIMP_PDB_FLOAT:
      g_value_init (&arg->value, G_TYPE_DOUBLE);
      break;

    case GIMP_PDB_STRING:
      g_value_init (&arg->value, G_TYPE_STRING);
      break;

    case GIMP_PDB_INT32ARRAY:
    case GIMP_PDB_INT16ARRAY:
    case GIMP_PDB_INT8ARRAY:
    case GIMP_PDB_FLOATARRAY:
    case GIMP_PDB_STRINGARRAY:
      g_value_init (&arg->value, G_TYPE_POINTER);
      break;

    case GIMP_PDB_COLOR:
      g_value_init (&arg->value, GIMP_TYPE_RGB);
      break;

    case GIMP_PDB_REGION:
    case GIMP_PDB_BOUNDARY:
      break;

    case GIMP_PDB_DISPLAY:
    case GIMP_PDB_IMAGE:
    case GIMP_PDB_LAYER:
    case GIMP_PDB_CHANNEL:
    case GIMP_PDB_DRAWABLE:
    case GIMP_PDB_SELECTION:
    case GIMP_PDB_VECTORS:
      g_value_init (&arg->value, G_TYPE_INT);
      break;

    case GIMP_PDB_PARASITE:
      g_value_init (&arg->value, GIMP_TYPE_PARASITE);
      break;

    case GIMP_PDB_STATUS:
      g_value_init (&arg->value, GIMP_TYPE_PDB_STATUS_TYPE);
      break;

    case GIMP_PDB_END:
      break;
    }
}

void
gimp_arguments_destroy (Argument *args,
                        gint      n_args,
                        gboolean  full_destroy)
{
  gint i;

  if (! args && n_args)
    return;

  for (i = n_args - 1; i >= 0; i--)
    {
      switch (args[i].type)
        {
        case GIMP_PDB_INT32:
        case GIMP_PDB_INT16:
        case GIMP_PDB_INT8:
        case GIMP_PDB_FLOAT:
        case GIMP_PDB_STRING:
          break;

        case GIMP_PDB_INT32ARRAY:
        case GIMP_PDB_INT16ARRAY:
        case GIMP_PDB_INT8ARRAY:
        case GIMP_PDB_FLOATARRAY:
          if (full_destroy)
            g_free (g_value_get_pointer (&args[i].value));
          break;

        case GIMP_PDB_STRINGARRAY:
          if (full_destroy)
            {
              gchar **array;
              gint    count;
              gint    j;

              count = g_value_get_int (&args[i - 1].value);
              array = g_value_get_pointer (&args[i].value);

              for (j = 0; j < count; j++)
                g_free (array[j]);

              g_free (array);
            }
          break;

        case GIMP_PDB_COLOR:
        case GIMP_PDB_REGION:
        case GIMP_PDB_DISPLAY:
        case GIMP_PDB_IMAGE:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_BOUNDARY:
        case GIMP_PDB_VECTORS:
        case GIMP_PDB_PARASITE:
        case GIMP_PDB_STATUS:
        case GIMP_PDB_END:
          break;
        }

      g_value_unset (&args[i].value);
    }

  g_free (args);
}
