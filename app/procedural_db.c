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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include "appenv.h"
#include "app_procs.h"
#include "general.h"
#include "gdisplay.h"
#include "plug_in.h"
#include "procedural_db.h"
#include "libgimp/parasite.h"
#include "config.h"

#include "libgimp/gimpintl.h"

GHashTable *procedural_ht = NULL;

/*  Local functions  */
static guint      procedural_db_hash_func (gconstpointer key);
static void	  pdb_id_init (void);


void
procedural_db_init (void)
{
  app_init_update_status(_("Procedural Database"), NULL, -1);
  if (!procedural_ht)
    procedural_ht = g_hash_table_new (procedural_db_hash_func, g_str_equal);
  pdb_id_init ();
}

void
procedural_db_free_entry (gpointer key,
			  gpointer value,
			  gpointer user_data)
{
  if (value)
    g_list_free (value);
}

void
procedural_db_free (void)
{
  if (procedural_ht)
    {
      g_hash_table_foreach (procedural_ht, procedural_db_free_entry, NULL);
      g_hash_table_destroy (procedural_ht);
    }
  
  procedural_ht = NULL;
}

void
procedural_db_register (ProcRecord *procedure)
{
  GList *list;

  if (!procedural_ht)
    procedural_db_init ();

  list = g_hash_table_lookup (procedural_ht, (gpointer) procedure->name);
  list = g_list_prepend (list, (gpointer) procedure);

  g_hash_table_insert (procedural_ht,
		       (gpointer) procedure->name,
		       (gpointer) list);
}

void
procedural_db_unregister (gchar *name)
{
  GList *list;

  list = g_hash_table_lookup (procedural_ht, (gpointer) name);
  if (list)
    {
      list = g_list_remove (list, list->data);

      if (list)
	g_hash_table_insert (procedural_ht,
			     (gpointer) name,
			     (gpointer) list);
      else 
	g_hash_table_remove (procedural_ht,
			     (gpointer) name);
    }
}

ProcRecord *
procedural_db_lookup (gchar *name)
{
  GList *list;
  ProcRecord *procedure;

  list = g_hash_table_lookup (procedural_ht, (gpointer) name);
  if (list != NULL)
    procedure = (ProcRecord *) list->data;
  else
    procedure = NULL;

  return procedure;
}

Argument *
procedural_db_execute (gchar    *name,
		       Argument *args)
{
  ProcRecord *procedure;
  Argument *return_args;
  GList *list;
  int i;

  return_args = NULL;

  list = g_hash_table_lookup (procedural_ht, (gpointer) name);

  if (list == NULL)
    {
      g_message (_("PDB calling error %s not found"), name);
      
      return_args = (Argument *) g_malloc (sizeof (Argument));
      return_args->arg_type = PDB_STATUS;
      return_args->value.pdb_int = PDB_CALLING_ERROR;
      return return_args;
    }
  
  while (list)
    {
      if ((procedure = (ProcRecord *) list->data) == NULL)
	{
	  g_message (_("PDB calling error %s not found"), name);

	  return_args = (Argument *) g_malloc (sizeof (Argument));
	  return_args->arg_type = PDB_STATUS;
	  return_args->value.pdb_int = PDB_CALLING_ERROR;
	  return return_args;
	}
      list = list->next;

      /*  check the arguments  */
      for (i = 0; i < procedure->num_args; i++)
	{
	  if (args[i].arg_type != procedure->args[i].arg_type)
	    {
	      return_args = (Argument *) g_malloc (sizeof (Argument));
	      return_args->arg_type = PDB_STATUS;
	      return_args->value.pdb_int = PDB_CALLING_ERROR;

	      g_message (_("PDB calling error %s"), procedure->name);

	      return return_args;
	    }
	}

      /*  call the procedure  */
      switch (procedure->proc_type)
	{
	case PDB_INTERNAL:
	  return_args = (* procedure->exec_method.internal.marshal_func) (args);
	  break;

	case PDB_PLUGIN:
	  return_args = plug_in_run (procedure, args, procedure->num_args, TRUE, FALSE, -1);
	  break;

	case PDB_EXTENSION:
	  return_args = plug_in_run (procedure, args, procedure->num_args, TRUE, FALSE, -1);
	  break;

	case PDB_TEMPORARY:
	  return_args = plug_in_run (procedure, args, procedure->num_args, TRUE, FALSE, -1);
	  break;

	default:
	  return_args = (Argument *) g_malloc (sizeof (Argument));
	  return_args->arg_type = PDB_STATUS;
	  return_args->value.pdb_int = PDB_EXECUTION_ERROR;
	  break;
	}

      if ((return_args[0].value.pdb_int != PDB_SUCCESS) &&
	  (procedure->num_values > 0))
	memset (&return_args[1], 0, sizeof (Argument) * procedure->num_values);

      /*  Check if the return value is a PDB_PASS_THROUGH, in which case run the
       *  next procedure in the list
       */
      if (return_args[0].value.pdb_int != PDB_PASS_THROUGH)
	break;
      else if (list)  /*  Pass through, so destroy return values and run another procedure  */
	procedural_db_destroy_args (return_args, procedure->num_values);
    }

  return return_args;
}

Argument *
procedural_db_run_proc (gchar *name,
			gint  *nreturn_vals,
			...)
{
  ProcRecord *proc;
  Argument *params;
  Argument *return_vals;
  va_list args;
  int i;

  if ((proc = procedural_db_lookup (name)) == NULL)
    {
      return_vals = (Argument *) g_malloc (sizeof (Argument));
      return_vals->arg_type = PDB_STATUS;
      return_vals->value.pdb_int = PDB_CALLING_ERROR;
      return return_vals;
    }

  /*  allocate the parameter array  */
  params = g_new (Argument, proc->num_args);

  va_start (args, nreturn_vals);

  for (i = 0; i < proc->num_args; i++)
    {
      if (proc->args[i].arg_type != (params[i].arg_type = va_arg (args, PDBArgType)))
	{
	  g_message (_("Incorrect arguments passed to procedural_db_run_proc:\nArgument %d to '%s' should be a %s, but got passed a %s"),
		     i+1, proc->name,
		     pdb_type_name (proc->args[i].arg_type),
		     pdb_type_name (params[i].arg_type));
	  g_free (params);
	  return NULL;
	}

      switch (proc->args[i].arg_type)
	{
	case PDB_INT32:
	case PDB_INT16:
	case PDB_INT8:
        case PDB_DISPLAY:
	  params[i].value.pdb_int = (gint32) va_arg (args, int);
	  break;
        case PDB_FLOAT:
          params[i].value.pdb_float = (gdouble) va_arg (args, double);
          break;
        case PDB_STRING:
        case PDB_INT32ARRAY:
        case PDB_INT16ARRAY:
        case PDB_INT8ARRAY:
        case PDB_FLOATARRAY:
        case PDB_STRINGARRAY:
        case PDB_COLOR:
          params[i].value.pdb_pointer = va_arg (args, void *);
          break;
        case PDB_REGION:
          break;
        case PDB_IMAGE:
        case PDB_LAYER:
        case PDB_CHANNEL:
        case PDB_DRAWABLE:
        case PDB_SELECTION:
        case PDB_BOUNDARY:
        case PDB_PATH:
	  params[i].value.pdb_int = (gint32) va_arg (args, int);
	  break;
        case PDB_PARASITE:
          params[i].value.pdb_pointer = va_arg (args, void *);
          break;
        case PDB_STATUS:
	  params[i].value.pdb_int = (gint32) va_arg (args, int);
	  break;
	case PDB_END:
	  break;
	}
    }

  va_end (args);

  *nreturn_vals = proc->num_values;

  return_vals = procedural_db_execute (name, params);

  g_free (params);

  return return_vals;
}

Argument *
procedural_db_return_args (ProcRecord *procedure,
			   int         success)
{
  Argument *return_args;
  int i;

  return_args = (Argument *) g_malloc (sizeof (Argument) * (procedure->num_values + 1));

  if (success)
    {
      return_args[0].arg_type = PDB_STATUS;
      return_args[0].value.pdb_int = PDB_SUCCESS;
    }
  else
    {
      return_args[0].arg_type = PDB_STATUS;
      return_args[0].value.pdb_int = PDB_EXECUTION_ERROR;
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
  int i, j;
  int prev_val = 0;
  char **strs;

  if (!args) return;

  for (i = 0; i < nargs; i++)
    {
      switch (args[i].arg_type)
	{
	case PDB_INT32:
	  /*  Keep this around in case we need to free an array of strings  */
	  prev_val = args[i].value.pdb_int;
	  break;
	case PDB_INT16:
	case PDB_INT8:
	case PDB_FLOAT:
	  break;
	case PDB_STRING:
	case PDB_INT32ARRAY:
	case PDB_INT16ARRAY:
	case PDB_INT8ARRAY:
	case PDB_FLOATARRAY:
	  g_free (args[i].value.pdb_pointer);
	  break;
	case PDB_STRINGARRAY:
	  strs = (char **) args[i].value.pdb_pointer;
	  for (j = 0; j < prev_val; j++)
	    g_free (strs[j]);
	  g_free (strs);
	  break;
	case PDB_COLOR:
	  g_free (args[i].value.pdb_pointer);
	  break;
	case PDB_REGION:
	case PDB_DISPLAY:
	case PDB_IMAGE:
	case PDB_LAYER:
	case PDB_CHANNEL:
	case PDB_DRAWABLE:
	case PDB_SELECTION:
	case PDB_BOUNDARY:
	case PDB_PATH:
	case PDB_PARASITE:
	case PDB_STATUS:
	case PDB_END:
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
  guint result;
  int c;

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


/* The id system's remnants ... */


static gint next_image_id;
/*
static gint next_drawable_id;
static gint next_display_id;
*/

static GHashTable* image_hash;
static GHashTable* drawable_hash;
static GHashTable* display_hash;

static guint
id_hash_func (gconstpointer id)
{
  return *((guint*) id);
}

static gboolean
id_cmp_func (gconstpointer id1,
    	     gconstpointer id2)
{
  return (*((guint*) id1) == *((guint*) id2));
}

static void
add_cb (GimpSet   *set,
    	GimpImage *gimage,
	gpointer   data)
{
  guint *id = g_new (guint, 1);
  *id = next_image_id++;
  gtk_object_set_data (GTK_OBJECT (gimage), "pdb_id", id);
  g_hash_table_insert (image_hash, id, gimage);
}

static void
remove_cb (GimpSet   *set,
    	   GimpImage *image,
	   gpointer   data)
{
  guint *id = gtk_object_get_data (GTK_OBJECT(image), "pdb_id");
  gtk_object_remove_data (GTK_OBJECT(image), "pdb_id");
  g_hash_table_remove (image_hash, id);
  g_free (id);
}

static void
pdb_id_init (void)
{
  image_hash    = g_hash_table_new (id_hash_func, id_cmp_func);
  drawable_hash = g_hash_table_new (id_hash_func, id_cmp_func);
  display_hash  = g_hash_table_new (id_hash_func, id_cmp_func);

  gtk_signal_connect (GTK_OBJECT (image_context), "add",
		      GTK_SIGNAL_FUNC (add_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (image_context), "remove",
		      GTK_SIGNAL_FUNC (remove_cb), NULL);
}


gint
pdb_image_to_id (GimpImage* gimage)
{
  guint *id = gtk_object_get_data (GTK_OBJECT (gimage), "pdb_id");
  return id ? (gint) *id : -1;
}
	
GimpImage*
pdb_id_to_image (gint id)
{
  return g_hash_table_lookup (image_hash, &id);
}
