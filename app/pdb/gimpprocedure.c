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

#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

#include <gtk/gtk.h>

#include "core/core-types.h"

#include "core/gimp.h"

#include "app_procs.h"
#include "context_manager.h"
#include "plug_in.h"

#include "procedural_db.h"

#include "libgimp/gimpintl.h"


/*  Local functions  */
static guint   procedural_db_hash_func (gconstpointer key);


void
procedural_db_init (Gimp *gimp)
{
  g_return_if_fail (gimp != NULL);
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->procedural_ht = g_hash_table_new (procedural_db_hash_func, g_str_equal);
}

static void
procedural_db_free_entry (gpointer key,
			  gpointer value,
			  gpointer user_data)
{
  if (value)
    g_list_free (value);
}

void
procedural_db_free (Gimp *gimp)
{
  if (gimp->procedural_ht)
    {
      g_hash_table_foreach (gimp->procedural_ht, procedural_db_free_entry, NULL);
      g_hash_table_destroy (gimp->procedural_ht);
  
      gimp->procedural_ht = NULL;
    }
}

void
procedural_db_register (Gimp       *gimp,
			ProcRecord *procedure)
{
  GList *list;

  list = g_hash_table_lookup (gimp->procedural_ht, (gpointer) procedure->name);
  list = g_list_prepend (list, (gpointer) procedure);

  g_hash_table_insert (gimp->procedural_ht,
		       (gpointer) procedure->name,
		       (gpointer) list);
}

void
procedural_db_unregister (Gimp        *gimp,
			  const gchar *name)
{
  GList *list;

  list = g_hash_table_lookup (gimp->procedural_ht, (gpointer) name);
  if (list)
    {
      list = g_list_remove (list, list->data);

      if (list)
	g_hash_table_insert (gimp->procedural_ht,
			     (gpointer) name,
			     (gpointer) list);
      else 
	g_hash_table_remove (gimp->procedural_ht,
			     (gpointer) name);
    }
}

ProcRecord *
procedural_db_lookup (Gimp        *gimp,
		      const gchar *name)
{
  GList      *list;
  ProcRecord *procedure;

  list = g_hash_table_lookup (gimp->procedural_ht, (gpointer) name);
  if (list != NULL)
    procedure = (ProcRecord *) list->data;
  else
    procedure = NULL;

  return procedure;
}

Argument *
procedural_db_execute (Gimp        *gimp,
		       const gchar *name,
		       Argument    *args)
{
  ProcRecord *procedure;
  Argument   *return_args;
  GList      *list;
  gint        i;

  return_args = NULL;

  list = g_hash_table_lookup (gimp->procedural_ht, (gpointer) name);

  if (list == NULL)
    {
      g_message (_("PDB calling error %s not found"), name);
      
      return_args = g_new (Argument, 1);
      return_args->arg_type = GIMP_PDB_STATUS;
      return_args->value.pdb_int = GIMP_PDB_CALLING_ERROR;
      return return_args;
    }
  
  while (list)
    {
      if ((procedure = (ProcRecord *) list->data) == NULL)
	{
	  g_message (_("PDB calling error %s not found"), name);

	  return_args = g_new (Argument, 1);
	  return_args->arg_type = GIMP_PDB_STATUS;
	  return_args->value.pdb_int = GIMP_PDB_CALLING_ERROR;
	  return return_args;
	}
      list = list->next;

      /*  check the arguments  */
      for (i = 0; i < procedure->num_args; i++)
	{
	  if (args[i].arg_type != procedure->args[i].arg_type)
	    {
	      return_args = g_new (Argument, 1);
	      return_args->arg_type = GIMP_PDB_STATUS;
	      return_args->value.pdb_int = GIMP_PDB_CALLING_ERROR;

	      g_message (_("PDB calling error %s"), procedure->name);

	      return return_args;
	    }
	}

      /*  call the procedure  */
      switch (procedure->proc_type)
	{
	case GIMP_INTERNAL:
	  return_args = (* procedure->exec_method.internal.marshal_func) (gimp, args);
	  break;

	case GIMP_PLUGIN:
	  return_args = plug_in_run (procedure, args, procedure->num_args, TRUE, FALSE, -1);
	  break;

	case GIMP_EXTENSION:
	  return_args = plug_in_run (procedure, args, procedure->num_args, TRUE, FALSE, -1);
	  break;

	case GIMP_TEMPORARY:
	  return_args = plug_in_run (procedure, args, procedure->num_args, TRUE, FALSE, -1);
	  break;

	default:
	  return_args = g_new (Argument, 1);
	  return_args->arg_type = GIMP_PDB_STATUS;
	  return_args->value.pdb_int = GIMP_PDB_EXECUTION_ERROR;
	  break;
	}

      if ((return_args[0].value.pdb_int != GIMP_PDB_SUCCESS) &&
	  (procedure->num_values > 0))
	memset (&return_args[1], 0, sizeof (Argument) * procedure->num_values);

      /*  Check if the return value is a PDB_PASS_THROUGH, in which case run the
       *  next procedure in the list
       */
      if (return_args[0].value.pdb_int != GIMP_PDB_PASS_THROUGH)
	break;
      else if (list)  /*  Pass through, so destroy return values and run another procedure  */
	procedural_db_destroy_args (return_args, procedure->num_values);
    }

  return return_args;
}

Argument *
procedural_db_run_proc (Gimp        *gimp,
			const gchar *name,
			gint        *nreturn_vals,
			...)
{
  ProcRecord *proc;
  Argument   *params;
  Argument   *return_vals;
  va_list     args;
  gint        i;

  if ((proc = procedural_db_lookup (gimp, name)) == NULL)
    {
      return_vals = g_new (Argument, 1);
      return_vals->arg_type = GIMP_PDB_STATUS;
      return_vals->value.pdb_int = GIMP_PDB_CALLING_ERROR;
      return return_vals;
    }

  /*  allocate the parameter array  */
  params = g_new (Argument, proc->num_args);

  va_start (args, nreturn_vals);

  for (i = 0; i < proc->num_args; i++)
    {
      if (proc->args[i].arg_type != (params[i].arg_type = va_arg (args, GimpPDBArgType)))
	{
	  g_message (_("Incorrect arguments passed to procedural_db_run_proc:\n"
		       "Argument %d to '%s' should be a %s, but got passed a %s"),
		     i+1, proc->name,
		     pdb_type_name (proc->args[i].arg_type),
		     pdb_type_name (params[i].arg_type));
	  g_free (params);
	  return NULL;
	}

      switch (proc->args[i].arg_type)
	{
	case GIMP_PDB_INT32:
	case GIMP_PDB_INT16:
	case GIMP_PDB_INT8:
        case GIMP_PDB_DISPLAY:
	  params[i].value.pdb_int = (gint32) va_arg (args, gint);
	  break;
        case GIMP_PDB_FLOAT:
          params[i].value.pdb_float = (gdouble) va_arg (args, gdouble);
          break;
        case GIMP_PDB_STRING:
        case GIMP_PDB_INT32ARRAY:
        case GIMP_PDB_INT16ARRAY:
        case GIMP_PDB_INT8ARRAY:
        case GIMP_PDB_FLOATARRAY:
        case GIMP_PDB_STRINGARRAY:
          params[i].value.pdb_pointer = va_arg (args, gpointer);
          break;
        case GIMP_PDB_COLOR:
	  params[i].value.pdb_color = va_arg (args, GimpRGB);
          break;
        case GIMP_PDB_REGION:
          break;
        case GIMP_PDB_IMAGE:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_BOUNDARY:
        case GIMP_PDB_PATH:
	  params[i].value.pdb_int = (gint32) va_arg (args, gint);
	  break;
        case GIMP_PDB_PARASITE:
          params[i].value.pdb_pointer = va_arg (args, gpointer);
          break;
        case GIMP_PDB_STATUS:
	  params[i].value.pdb_int = (gint32) va_arg (args, gint);
	  break;
	case GIMP_PDB_END:
	  break;
	}
    }

  va_end (args);

  *nreturn_vals = proc->num_values;

  return_vals = procedural_db_execute (gimp, name, params);

  g_free (params);

  return return_vals;
}

Argument *
procedural_db_return_args (ProcRecord *procedure,
			   gboolean    success)
{
  Argument *return_args;
  gint      i;

  return_args = g_new (Argument, procedure->num_values + 1);

  if (success)
    {
      return_args[0].arg_type = GIMP_PDB_STATUS;
      return_args[0].value.pdb_int = GIMP_PDB_SUCCESS;
    }
  else
    {
      return_args[0].arg_type = GIMP_PDB_STATUS;
      return_args[0].value.pdb_int = GIMP_PDB_EXECUTION_ERROR;
    }

  /*  Set the arg types for the return values  */
  for (i = 0; i < procedure->num_values; i++)
    return_args[i+1].arg_type = procedure->values[i].arg_type;

  return return_args;
}

void
procedural_db_destroy_args (Argument *args,
			    int       nargs)
{
  gint    i, j;
  gint    prev_val = 0;
  gchar **strs;

  if (!args)
    return;

  for (i = 0; i < nargs; i++)
    {
      switch (args[i].arg_type)
	{
	case GIMP_PDB_INT32:
	  /*  Keep this around in case we need to free an array of strings  */
	  prev_val = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_INT16:
	case GIMP_PDB_INT8:
	case GIMP_PDB_FLOAT:
	  break;
	case GIMP_PDB_STRING:
	case GIMP_PDB_INT32ARRAY:
	case GIMP_PDB_INT16ARRAY:
	case GIMP_PDB_INT8ARRAY:
	case GIMP_PDB_FLOATARRAY:
	  g_free (args[i].value.pdb_pointer);
	  break;
	case GIMP_PDB_STRINGARRAY:
	  strs = (gchar **) args[i].value.pdb_pointer;
	  for (j = 0; j < prev_val; j++)
	    g_free (strs[j]);
	  g_free (strs);
	  break;
	case GIMP_PDB_COLOR:
	  g_free (args[i].value.pdb_pointer);
	  break;
	case GIMP_PDB_REGION:
	case GIMP_PDB_DISPLAY:
	case GIMP_PDB_IMAGE:
	case GIMP_PDB_LAYER:
	case GIMP_PDB_CHANNEL:
	case GIMP_PDB_DRAWABLE:
	case GIMP_PDB_SELECTION:
	case GIMP_PDB_BOUNDARY:
	case GIMP_PDB_PATH:
	case GIMP_PDB_PARASITE:
	case GIMP_PDB_STATUS:
	case GIMP_PDB_END:
	  break;
	}
    }

  g_free (args);
}

/* We could just use g_str_hash() here ... that uses a different
 * hash function, though
 */

static guint
procedural_db_hash_func (gconstpointer key)
{
  const gchar *string;
  guint        result;
  gint         c;

  /*
   * I tried a zillion different hash functions and asked many other
   * people for advice.  Many people had their own favorite functions,
   * all different, but no-one had much idea why they were good ones.
   * I chose the one below (multiply by 9 and add new character)
   * because of the following reasons:
   *
   * 1. Multiplying by 10 is perfect for keys that are decimal strings,
   *    and multiplying by 9 is just about as good.
   * 2. Times-9 is (shift-left-3) plus (old).  This means that each
   *    character's bits hang around in the low-order bits of the
   *    hash value for ever, plus they spread fairly rapidly up to
   *    the high-order bits to fill out the hash value.  This seems
   *    works well both for decimal and non-decimal strings.
   *
   * tclHash.c --
   *
   *      Implementation of in-memory hash tables for Tcl and Tcl-based
   *      applications.
   *
   * Copyright (c) 1991-1993 The Regents of the University of California.
   * Copyright (c) 1994 Sun Microsystems, Inc.
   */

  string = (const gchar *) key;
  result = 0;
  while (1)
    {
      c = *string;
      string++;
      if (c == 0)
	break;
      result += (result << 3) + c;
    }

  return result;
}
