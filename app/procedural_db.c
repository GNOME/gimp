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
#include "regex.h"
#include "appenv.h"
#include "app_procs.h"
#include "general.h"
#include "gdisplay.h"
#include "plug_in.h"
#include "procedural_db.h"

/*  Query structure  */
typedef struct _PDBQuery PDBQuery;

struct _PDBQuery
{
  regex_t name_regex;
  regex_t blurb_regex;
  regex_t help_regex;
  regex_t author_regex;
  regex_t copyright_regex;
  regex_t date_regex;
  regex_t proc_type_regex;

  char**  list_of_procs;
  int     num_procs;
};

typedef struct _PDBData PDBData;

struct _PDBData
{
  char * identifier;
  int    bytes;
  char * data;
};

/*  Local functions  */
static Argument * procedural_db_dump (Argument *);
static void       procedural_db_print_entry (gpointer, gpointer, gpointer);
static void       output_string (char *);
static Argument * procedural_db_proc_info (Argument *);
static Argument * procedural_db_proc_arg (Argument *);
static Argument * procedural_db_proc_val (Argument *);
static Argument * procedural_db_get_data (Argument *);
static Argument * procedural_db_set_data (Argument *);
static Argument * procedural_db_query (Argument *);
static void       procedural_db_query_entry (gpointer, gpointer, gpointer);
static int        match_strings (regex_t *, char *);
static guint      procedural_db_hash_func (gpointer key);
static gint       procedural_db_compare_func (gpointer a, gpointer b);


/*  Local data  */
static GHashTable *procedural_ht = NULL;
static FILE *procedural_db_out = NULL;
static GList *data_list = NULL;

static char *type_str[] =
{
  "PDB_INT32",
  "PDB_INT16",
  "PDB_INT8",
  "PDB_FLOAT",
  "PDB_STRING",
  "PDB_INT32ARRAY",
  "PDB_INT16ARRAY",
  "PDB_INT8ARRAY",
  "PDB_FLOATARRAY",
  "PDB_STRINGARRAY",
  "PDB_COLOR",
  "PDB_REGION",
  "PDB_DISPLAY",
  "PDB_IMAGE",
  "PDB_LAYER",
  "PDB_CHANNEL",
  "PDB_DRAWABLE",
  "PDB_SELECTION",
  "PDB_BOUNDARY",
  "PDB_PATH",
  "PDB_STATUS",
  "PDB_END"
};

static char *proc_type_str[] =
{
  "Internal GIMP procedure",
  "GIMP Plug-In",
  "GIMP Extension",
  "Temporary Procedure"
};

/************************/
/*  PROCEDURAL_DB_DUMP  */

static ProcArg procedural_db_dump_args[] =
{
  { PDB_STRING,
    "filename",
    "the dump filename"
  }
};

ProcRecord procedural_db_dump_proc =
{
  "gimp_procedural_db_dump",
  "Dumps the current contents of the procedural database",
  "This procedure dumps the contents of the procedural database to the specified file.  The file will contain all of the information provided for each registered procedure.  This file is in a format appropriate for use with the supplied \"pdb_self_doc.el\" Elisp script, which generates a texinfo document.",
  "Spencer Kimball & Josh MacDonald",
  "Spencer Kimball & Josh MacDonald & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  procedural_db_dump_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { procedural_db_dump } },
};


/*****************************/
/*  PROCEDURAL_DB_PROC_INFO  */

static ProcArg procedural_db_proc_info_args[] =
{
  { PDB_STRING,
    "procedure",
    "the procedure name"
  }
};

static ProcArg procedural_db_proc_info_out_args[] =
{
  { PDB_STRING,
    "blurb",
    "a short blurb"
  },
  { PDB_STRING,
    "help",
    "detailed procedure help"
  },
  { PDB_STRING,
    "author",
    "author(s) of the procedure"
  },
  { PDB_STRING,
    "copyright",
    "the copyright"
  },
  { PDB_STRING,
    "date",
    "copyright date"
  },
  { PDB_INT32,
    "proc_type",
    "the procedure type: { INTERNAL (0), PLUGIN (1), EXTENSION (2) }",
  },
  { PDB_INT32,
    "num_args",
    "the number of input arguments"
  },
  { PDB_INT32,
    "num_rvals",
    "the number of return values"
  }
};

ProcRecord procedural_db_proc_info_proc =
{
  "gimp_procedural_db_proc_info",
  "Queries the procedural database for information on the specified procedure",
  "This procedure returns information on the specified procedure.  A short blurb, detailed help, author(s), copyright information, procedure type, number of input, and number of return values are returned.  For specific information on each input argument and return value, use the 'gimp_procedural_db_query_proc_arg' and 'gimp_procedural_db_query_proc_val' procedures",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  procedural_db_proc_info_args,

  /*  Output arguments  */
  8,
  procedural_db_proc_info_out_args,

  /*  Exec method  */
  { { procedural_db_proc_info } },
};


/**********************************/
/*  PROCEDURAL_DB_PROC_ARG  */

static ProcArg procedural_db_proc_arg_args[] =
{
  { PDB_STRING,
    "procedure",
    "the procedure name"
  },
  { PDB_INT32,
    "arg_num",
    "the argument number"
  }
};

static ProcArg procedural_db_proc_arg_out_args[] =
{
  { PDB_INT32,
    "arg_type",
    "the type of argument { PDB_INT32 (0), PDB_INT16 (1), PDB_INT8 (2), PDB_FLOAT (3), PDB_STRING (4), PDB_INT32ARRAY (5), PDB_INT16ARRAY (6), PDB_INT8ARRAY (7), PDB_FLOATARRAY (8), PDB_STRINGARRAY (9), PDB_COLOR (10), PDB_REGION (11), PDB_DISPLAY (12), PDB_IMAGE (13), PDB_LAYER (14), PDB_CHANNEL (15), PDB_DRAWABLE (16), PDB_SELECTION (17), PDB_BOUNDARY (18), PDB_PATH (19), PDB_STATUS (20) }"
  },
  { PDB_STRING,
    "arg_name",
    "the name of the argument"
  },
  { PDB_STRING,
    "arg_desc",
    "a description of the argument"
  }
};

ProcRecord procedural_db_proc_arg_proc =
{
  "gimp_procedural_db_proc_arg",
  "Queries the procedural database for information on the specified procedure's argument",
  "This procedure returns information on the specified procedure's argument.  The argument type, name, and a description are retrieved.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  procedural_db_proc_arg_args,

  /*  Output arguments  */
  3,
  procedural_db_proc_arg_out_args,

  /*  Exec method  */
  { { procedural_db_proc_arg } },
};


/****************************/
/*  PROCEDURAL_DB_PROC_VAL  */

static ProcArg procedural_db_proc_val_args[] =
{
  { PDB_STRING,
    "procedure",
    "the procedure name"
  },
  { PDB_INT32,
    "val_num",
    "the return value number"
  }
};

static ProcArg procedural_db_proc_val_out_args[] =
{
  { PDB_INT32,
    "val_type",
    "the type of return value { PDB_INT32 (0), PDB_INT16 (1), PDB_INT8 (2), PDB_FLOAT (3), PDB_STRING (4), PDB_INT32ARRAY (5), PDB_INT16ARRAY (6), PDB_INT8ARRAY (7), PDB_FLOATARRAY (8), PDB_STRINGARRAY (9), PDB_COLOR (10), PDB_REGION (11), PDB_DISPLAY (12), PDB_IMAGE (13), PDB_LAYER (14), PDB_CHANNEL (15), PDB_DRAWABLE (16), PDB_SELECTION (17), PDB_BOUNDARY (18), PDB_PATH (19), PDB_STATUS (20) }"
  },
  { PDB_STRING,
    "val_name",
    "the name of the return value"
  },
  { PDB_STRING,
    "val_desc",
    "a description of the return value"
  }
};

ProcRecord procedural_db_proc_val_proc =
{
  "gimp_procedural_db_proc_val",
  "Queries the procedural database for information on the specified procedure's return value",
  "This procedure returns information on the specified procedure's return value.  The return value type, name, and a description are retrieved.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  procedural_db_proc_val_args,

  /*  Output arguments  */
  3,
  procedural_db_proc_val_out_args,

  /*  Exec method  */
  { { procedural_db_proc_val } },
};


/****************************/
/*  PROCEDURAL_DB_GET_DATA  */

static ProcArg procedural_db_get_data_args[] =
{
  { PDB_STRING,
    "identifier",
    "the identifier associated with data"
  },
};

static ProcArg procedural_db_get_data_out_args[] =
{
  { PDB_INT32,
    "bytes",
    "the number of bytes in the data"
  },
  { PDB_INT8ARRAY,
    "data",
    "a byte array containing data"
  }
};

ProcRecord procedural_db_get_data_proc =
{
  "gimp_procedural_db_get_data",
  "Returns data associated with the specified identifier",
  "This procedure returns any data which may have been associated with the specified identifier.  The data is a variable length array of bytes.  If no data has been associated with the identifier, an error is returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  procedural_db_get_data_args,

  /*  Output arguments  */
  2,
  procedural_db_get_data_out_args,

  /*  Exec method  */
  { { procedural_db_get_data } },
};


/****************************/
/*  PROCEDURAL_DB_SET_DATA  */

static ProcArg procedural_db_set_data_args[] =
{
  { PDB_STRING,
    "identifier",
    "the identifier for association with data"
  },
  { PDB_INT32,
    "bytes",
    "the number of bytes in data"
  },
  { PDB_INT8ARRAY,
    "data",
    "the data"
  }
};

ProcRecord procedural_db_set_data_proc =
{
  "gimp_procedural_db_set_data",
  "Associates the specified identifier with the supplied data",
  "This procedure associates the supplied data with the provided identifier.  The data may be subsequently retrieved by a call to 'procedural_db_get_data'.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  procedural_db_set_data_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { procedural_db_set_data } },
};


/*************************/
/*  PROCEDURAL_DB_QUERY  */

static ProcArg procedural_db_query_args[] =
{
  { PDB_STRING,
    "name",
    "the regex for procedure name"
  },
  { PDB_STRING,
    "blurb",
    "the regex for procedure blurb"
  },
  { PDB_STRING,
    "help",
    "the regex for procedure help"
  },
  { PDB_STRING,
    "author",
    "the regex for procedure author"
  },
  { PDB_STRING,
    "copyright",
    "the regex for procedure copyright"
  },
  { PDB_STRING,
    "date",
    "the regex for procedure date"
  },
  { PDB_STRING,
    "proc_type",
    "the regex for procedure type: {'Internal GIMP procedure', 'GIMP Plug-In', 'GIMP Extension'}"
  }
};

static ProcArg procedural_db_query_out_args[] =
{
  { PDB_INT32,
    "num_matches",
    "the number of matching procedures"
  },
  { PDB_STRINGARRAY,
    "procedure_names",
    "the list of procedure names"
  }
};

ProcRecord procedural_db_query_proc =
{
  "gimp_procedural_db_query",
  "Queries the procedural database for its contents using regular expression matching",
  "This procedure queries the contents of the procedural database.  It is supplied with seven arguments matching procedures on {name, blurb, help, author, copyright, date, procedure type}.  This is accomplished using regular expression matching.  For instance, to find all procedures with \"jpeg\" listed in the blurb, all seven arguments can be supplied as \".*\", except for the second, which can be supplied as \".*jpeg.*\".  There are two return arguments for this procedure.  The first is the number of procedures matching the query.  The second is a concatenated list of procedure names corresponding to those matching the query.  If no matching entries are found, then the returned string is NULL and the number of entries is 0.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  7,
  procedural_db_query_args,

  /*  Output arguments  */
  2,
  procedural_db_query_out_args,

  /*  Exec method  */
  { { procedural_db_query } },
};


void
procedural_db_init ()
{
  app_init_update_status("Procedural Database", NULL, -1);
  if (!procedural_ht)
    procedural_ht = g_hash_table_new (procedural_db_hash_func,
				      procedural_db_compare_func);
}

void
procedural_db_free ()
{
  if (procedural_ht)
    g_hash_table_destroy (procedural_ht);
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

  while (list)
    {
      if ((procedure = (ProcRecord *) list->data) == NULL)
	{
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

	      g_warning ("PDB calling error %s", procedure->name);

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
	  return_args = plug_in_run (procedure, args, TRUE, FALSE);
	  break;

	case PDB_EXTENSION:
	  return_args = plug_in_run (procedure, args, TRUE, FALSE);
	  break;

	case PDB_TEMPORARY:
	  return_args = plug_in_run (procedure, args, TRUE, FALSE);
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
	  g_warning ("Incorrect arguments passed to procedural_db_run_proc");
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
	case PDB_STATUS:
	case PDB_END:
	  break;
	}
    }

  g_free (args);
}

static Argument *
procedural_db_dump (Argument *args)
{
  char *filename;
  int success = TRUE;

  filename = (char *) args[0].value.pdb_pointer;

  if (filename)
    {
      if (! (procedural_db_out = fopen (filename, "w")))
	success = FALSE;
    }
  else
    success = FALSE;

  if (success)
    {
      g_hash_table_foreach (procedural_ht, procedural_db_print_entry, NULL);
      fclose (procedural_db_out);
    }

  return procedural_db_return_args (&procedural_db_dump_proc, success);
}

static void
procedural_db_print_entry (gpointer key,
			   gpointer value,
			   gpointer user_data)
{
  int i;
  ProcRecord *procedure;
  GList *list = (GList *) value;
  int num = 0;
  char *buf;

  while (list)
    {
      num++;
      procedure = (ProcRecord*) list->data;
      list = list->next;

      fprintf (procedural_db_out, "\n(register-procedure ");

      if (list || num != 1)
	{
	  buf = g_new (char, strlen (procedure->name) + 10);
	  sprintf (buf, "%s <%d>", procedure->name, num);
	  output_string (buf);
	  g_free (buf);
	}
      else
	output_string (procedure->name);

      output_string (procedure->blurb);
      output_string (procedure->help);
      output_string (procedure->author);
      output_string (procedure->copyright);
      output_string (procedure->date);
      output_string (proc_type_str[(int) procedure->proc_type]);
      fprintf (procedural_db_out, "( ");
      for (i = 0; i < procedure->num_args; i++)
	{
	  fprintf (procedural_db_out, "( ");

	  output_string (procedure->args[i].name );
	  output_string (type_str[procedure->args[i].arg_type]);
	  output_string (procedure->args[i].description);

	  fprintf (procedural_db_out, " ) ");
	}
      fprintf (procedural_db_out, " ) ");

      fprintf (procedural_db_out, "( ");
      for (i = 0; i < procedure->num_values; i++)
	{
	  fprintf (procedural_db_out, "( ");

	  output_string (procedure->values[i].name );
	  output_string (type_str[procedure->values[i].arg_type]);
	  output_string (procedure->values[i].description);

	  fprintf (procedural_db_out, " ) ");
	}
      fprintf (procedural_db_out, " ) ");
      fprintf (procedural_db_out, " ) ");
    }
}

static void
output_string (char *string)
{
  fprintf (procedural_db_out, "\"");
  while (*string)
    {
      switch (*string)
	{
	case '\\' : fprintf (procedural_db_out, "\\\\"); break;
	case '\"' : fprintf (procedural_db_out, "\\\""); break;
	case '{'  : fprintf (procedural_db_out, "@{"); break;
	case '@'  : fprintf (procedural_db_out, "@@"); break;
	case '}'  : fprintf (procedural_db_out, "@}"); break;
	default:
	  fprintf (procedural_db_out, "%c", *string);
	}
      string++;
    }
  fprintf (procedural_db_out, "\"\n");
}

static Argument *
procedural_db_proc_info (Argument *args)
{
  Argument *return_args;
  char *proc_name;
  ProcRecord *proc;

  proc_name = args[0].value.pdb_pointer;
  proc = procedural_db_lookup (proc_name);
  if (proc != NULL)
    {
      return_args = procedural_db_return_args (&procedural_db_proc_info_proc, TRUE);
      return_args[1].value.pdb_pointer = g_strdup (proc->blurb);
      return_args[2].value.pdb_pointer = g_strdup (proc->help);
      return_args[3].value.pdb_pointer = g_strdup (proc->author);
      return_args[4].value.pdb_pointer = g_strdup (proc->copyright);
      return_args[5].value.pdb_pointer = g_strdup (proc->date);
      return_args[6].value.pdb_int = proc->proc_type;
      return_args[7].value.pdb_int = proc->num_args;
      return_args[8].value.pdb_int = proc->num_values;
    }
  else
    return_args = procedural_db_return_args (&procedural_db_proc_info_proc, FALSE);

  return return_args;
}

static Argument *
procedural_db_proc_arg (Argument *args)
{
  Argument *return_args;
  ProcArg *arg;
  int arg_num;
  char *proc_name;
  ProcRecord *proc;

  proc_name = args[0].value.pdb_pointer;
  arg_num = args[1].value.pdb_int;
  proc = procedural_db_lookup (proc_name);

  if (proc != NULL && (arg_num >= 0 && arg_num < proc->num_args))
    {
      arg = &proc->args[arg_num];
      return_args = procedural_db_return_args (&procedural_db_proc_arg_proc, TRUE);
      return_args[1].value.pdb_int = arg->arg_type;
      return_args[2].value.pdb_pointer = g_strdup (arg->name);
      return_args[3].value.pdb_pointer = g_strdup (arg->description);
    }
  else
    return_args = procedural_db_return_args (&procedural_db_proc_arg_proc, FALSE);

  return return_args;
}

static Argument *
procedural_db_proc_val (Argument *args)
{
  Argument *return_args;
  ProcArg *val;
  int val_num;
  char *proc_name;
  ProcRecord *proc;

  proc_name = args[0].value.pdb_pointer;
  val_num = args[1].value.pdb_int;
  proc = procedural_db_lookup (proc_name);

  if (proc != NULL && (val_num >= 0 && val_num < proc->num_values))
    {
      val = &proc->values[val_num];
      return_args = procedural_db_return_args (&procedural_db_proc_val_proc, TRUE);
      return_args[1].value.pdb_int = val->arg_type;
      return_args[2].value.pdb_pointer = g_strdup (val->name);
      return_args[3].value.pdb_pointer = g_strdup (val->description);
    }
  else
    return_args = procedural_db_return_args (&procedural_db_proc_val_proc, FALSE);

  return return_args;
}

static Argument *
procedural_db_get_data (Argument *args)
{
  Argument *return_args;
  PDBData *data;
  char *data_copy;
  char *identifier;
  GList *list;

  identifier = args[0].value.pdb_pointer;
  list = data_list;
  while (list)
    {
      data = (PDBData *) list->data;
      list = list->next;

      if (strcmp (data->identifier, identifier) == 0)
	{
	  return_args = procedural_db_return_args (&procedural_db_get_data_proc, TRUE);
	  return_args[1].value.pdb_int = data->bytes;
	  data_copy = g_new (char, data->bytes);
	  memcpy (data_copy, data->data, data->bytes);
	  return_args[2].value.pdb_pointer = data_copy;

	  return return_args;
	}
    }

  return_args = procedural_db_return_args (&procedural_db_proc_val_proc, FALSE);
  return return_args;
}

static Argument *
procedural_db_set_data (Argument *args)
{
  Argument *return_args;
  PDBData *data = NULL;
  char *identifier;
  GList *list;

  identifier = args[0].value.pdb_pointer;

  list = data_list;
  while (list)
    {
      if (strcmp (((PDBData *) list->data)->identifier, identifier) == 0)
	data = (PDBData *) list->data;

      list = list->next;
    }

  /*  If there is not already data with the specified identifier, create a new one  */
  if (data == NULL)
    {
      data = (PDBData *) g_new (PDBData, 1);
      data_list = g_list_append (data_list, data);
    }
  /*  If there is, free the data  */
  else
    g_free (data->data);

  data->identifier = g_strdup (identifier);
  data->bytes = args[1].value.pdb_int;
  data->data = g_new (char, data->bytes);
  memcpy (data->data, (char *) args[2].value.pdb_pointer, data->bytes);

  return_args = procedural_db_return_args (&procedural_db_proc_val_proc, TRUE);

  return return_args;
}

static Argument *
procedural_db_query (Argument *args)
{
  Argument *return_args;
  PDBQuery pdb_query;

  regcomp (&pdb_query.name_regex, args[0].value.pdb_pointer, 0);
  regcomp (&pdb_query.blurb_regex, args[1].value.pdb_pointer, 0);
  regcomp (&pdb_query.help_regex, args[2].value.pdb_pointer, 0);
  regcomp (&pdb_query.author_regex, args[3].value.pdb_pointer, 0);
  regcomp (&pdb_query.copyright_regex, args[4].value.pdb_pointer, 0);
  regcomp (&pdb_query.date_regex, args[5].value.pdb_pointer, 0);
  regcomp (&pdb_query.proc_type_regex, args[6].value.pdb_pointer, 0);
  pdb_query.list_of_procs = NULL;
  pdb_query.num_procs = 0;

  g_hash_table_foreach (procedural_ht, procedural_db_query_entry, &pdb_query);

  free (pdb_query.name_regex.buffer);
  free (pdb_query.blurb_regex.buffer);
  free (pdb_query.help_regex.buffer);
  free (pdb_query.author_regex.buffer);
  free (pdb_query.copyright_regex.buffer);
  free (pdb_query.date_regex.buffer);
  free (pdb_query.proc_type_regex.buffer);

  return_args = procedural_db_return_args (&procedural_db_query_proc, TRUE);

  return_args[1].value.pdb_int = pdb_query.num_procs;
  return_args[2].value.pdb_pointer = pdb_query.list_of_procs;

  return return_args;
}

static void
procedural_db_query_entry (gpointer key,
			   gpointer value,
			   gpointer user_data)
{
  GList *list;
  ProcRecord *proc;
  PDBQuery *pdb_query;
  int new_length;

  list = (GList *) value;
  proc = (ProcRecord *) list->data;
  pdb_query = (PDBQuery *) user_data;

  if (!match_strings (&pdb_query->name_regex, proc->name) &&
      !match_strings (&pdb_query->blurb_regex, proc->blurb) &&
      !match_strings (&pdb_query->help_regex, proc->help) &&
      !match_strings (&pdb_query->author_regex, proc->author) &&
      !match_strings (&pdb_query->copyright_regex, proc->copyright) &&
      !match_strings (&pdb_query->date_regex, proc->date) &&
      !match_strings (&pdb_query->proc_type_regex, proc_type_str[(int) proc->proc_type]))
    {
      new_length = (proc->name) ? (strlen (proc->name) + 1) : 0;

      if (new_length)
	{
	  pdb_query->num_procs++;
	  pdb_query->list_of_procs = g_realloc (pdb_query->list_of_procs,
					       (sizeof (char **) * pdb_query->num_procs));
	  pdb_query->list_of_procs[pdb_query->num_procs - 1] = g_strdup (proc->name);
	}
    }
}

static int
match_strings (regex_t * preg,
	       char *    a)
{
  return regexec (preg, a, 0, NULL, 0);
}

static guint
procedural_db_hash_func (gpointer key)
{
  gchar *string;
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

  string = (gchar *) key;
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

static gint
procedural_db_compare_func (gpointer a,
			    gpointer b)
{
  return (strcmp ((char *) a, (char *) b) == 0);
}
