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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "libgimp/gimp.h"
#include "gtk/gtk.h"
#include "siod.h"
#include "script-fu-console.h"
#include "script-fu-scripts.h"
#include "script-fu-server.h"

/* External functions
 */
extern void gimp_extension_process (guint timeout);
extern void gimp_extension_ack     (void);

extern void  init_subrs (void);
extern void  init_trace (void);
extern void  init_regex (void);

extern long  nlength      (LISP obj);
extern LISP  leval_define (LISP args,
			   LISP env);

extern void  fput_st (FILE *f, char *st);

/* Declare local functions.
 */
static void      sfquit (void);
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static gint      init_interp      (void);
static void      init_gimp        (void);
static void      init_procedures  (void);
static void      init_constants   (void);
static void      convert_string   (char * str);

static int       sputs_fcn (char *st, void *dest);
static LISP      lprin1s   (LISP exp, char *dest);

static LISP      marshall_proc_db_call    (LISP a);
static LISP      script_fu_register_call  (LISP a);
static LISP      script_fu_quit_call      (LISP a);

static void      script_fu_auxillary_init (void);
static void      script_fu_refresh_proc   (char     *name,
					   int       nparams,
					   GParam   *params,
					   int      *nreturn_vals,
					   GParam  **return_vals);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  sfquit,  /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static char *siod_argv[] =
{
  "siod",
  "-h100000:10",
  "-g0",
  "-o1000",
  "-s200000",
  "-n2048",
  "-v0",
};

static gint script_fu_base = TRUE;
       gint script_fu_done = FALSE;
extern gint server_mode;


MAIN ()

static void
sfquit ()
{
}

static void
query ()
{
  static GParamDef console_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, [non-interactive]" }
  };
  static gint nconsole_args = sizeof (console_args) / sizeof (console_args[0]);

  static GParamDef server_args[] =
  {
    { PARAM_INT32, "run_mode", "[Interactive], non-interactive" },
    { PARAM_INT32, "port", "The port on which to listen for requests" },
    { PARAM_STRING, "logfile", "The file to log server activity to" }
  };
  static gint nserver_args = sizeof (server_args) / sizeof (server_args[0]);

  gimp_install_procedure ("extension_script_fu",
			  "A scheme interpreter for scripting GIMP operations",
			  "More help here later",
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1997",
			  NULL,
			  NULL,
			  PROC_EXTENSION,
			  0, 0, NULL, NULL);

  gimp_install_procedure ("extension_script_fu_console",
			  "Provides a console mode for script-fu development",
			  "Provides an interface which allows interactive scheme development.",
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1997",
			  "<Toolbox>/Xtns/Script-Fu/Console",
			  "",
			  PROC_EXTENSION,
			  nconsole_args, 0,
			  console_args, NULL);

  gimp_install_procedure ("extension_script_fu_server",
			  "Provides a server for remote script-fu operation",
			  "Provides a server for remote script-fu operation",
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1997",
			  "<Toolbox>/Xtns/Script-Fu/Server",
			  "",
			  PROC_EXTENSION,
			  nserver_args, 0,
			  server_args, NULL);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  /*  Determine before we allow scripts to register themselves
   *   whether this is the base, automatically installed script-fu extension
   */
  if (strcmp (name, "extension_script_fu") == 0)
    {
      /*  Setup auxillary temporary procedures for the base extension  */
      script_fu_auxillary_init ();

      script_fu_base = TRUE;
    }
  else
    script_fu_base = FALSE;

  /*  Init the interpreter  */
  init_interp ();

  /*  Load all of the available scripts  */
  script_fu_find_scripts ();

  /*
   *  The main, automatically installed script fu extension--
   *    For things like logos and effects that are runnable from GIMP menus
   */
  if (strcmp (name, "extension_script_fu") == 0)
    {
      static GParam values[1];
      GStatusType status = STATUS_SUCCESS;

      /*  Acknowledge that the extension is properly initialized  */
      gimp_extension_ack ();

      while (1)
	gimp_extension_process (0);

      *nreturn_vals = 1;
      *return_vals = values;

      values[0].type = PARAM_STATUS;
      values[0].data.d_status = status;
    }
  /*
   *  The script-fu console for interactive SIOD development
   */
  else if (strcmp (name, "extension_script_fu_console") == 0)
    {
      script_fu_console_run (name, nparams, param, nreturn_vals, return_vals);
    }
  /*
   *  The script-fu server for remote operation
   */
  else if (strcmp (name, "extension_script_fu_server") == 0)
    {
      script_fu_server_run (name, nparams, param, nreturn_vals, return_vals);
    }
}

static gint
init_interp (void)
{
  process_cla (sizeof (siod_argv) / sizeof (char *), siod_argv, 1);

  init_storage ();

  init_subrs ();
  init_trace ();
  init_regex ();
  init_gimp  ();

  return 0;
}

static void
init_gimp ()
{
  init_procedures ();
  init_constants ();
}

static void
init_procedures ()
{
  char **proc_list;
  char *proc_name;
  char *arg_name;
  char *proc_blurb;
  char *proc_help;
  char *proc_author;
  char *proc_copyright;
  char *proc_date;
  int proc_type;
  int nparams;
  int nreturn_vals;
  GParamDef *params;
  GParamDef *return_vals;
  int num_procs;
  int i;

  /*  register the database execution procedure  */
  init_lsubr ("gimp-proc-db-call", marshall_proc_db_call);
  init_lsubr ("script-fu-register", script_fu_register_call);
  init_lsubr ("script-fu-quit", script_fu_quit_call);

  gimp_query_database (".*", ".*", ".*", ".*", ".*", ".*", ".*", &num_procs, &proc_list);

  /*  Register each procedure as a scheme func  */
  for (i = 0; i < num_procs; i++)
    {
      proc_name = g_strdup (proc_list[i]);

      /*  lookup the procedure  */
      if (gimp_query_procedure (proc_name, &proc_blurb, &proc_help, &proc_author,
				&proc_copyright, &proc_date, &proc_type, &nparams, &nreturn_vals,
				&params, &return_vals) == TRUE)
	{
	  LISP args = NIL;
	  LISP code = NIL;
	  int j;

	  /*  convert the names to scheme-like naming conventions  */
	  convert_string (proc_name);

	  /*  create a new scheme func that calls gimp-proc-db-call  */

	  for (j = 0; j < nparams; j++)
	    {
	      arg_name = g_strdup (params[j].name);
	      convert_string (arg_name);
	      args = cons (cintern (arg_name), args);
	      code = cons (cintern (arg_name), code);
	    }

	  /*  reverse the list  */
	  args = nreverse (args);
	  code = nreverse (code);

	  /*  set the scheme-based procedure name  */
	  args = cons (cintern (proc_name), args);

	  /*  set the acture pdb procedure name  */
	  code = cons (cons (cintern ("quote"), cons (cintern (proc_list[i]), NIL)), code);
	  code = cons (cintern ("gimp-proc-db-call"), code);

	  leval_define (cons (args, cons (code, NIL)), NIL);

	  /*  free the queried information  */
	  g_free (proc_blurb);
	  g_free (proc_help);
	  g_free (proc_author);
	  g_free (proc_copyright);
	  g_free (proc_date);
	  g_free (params);
	  g_free (return_vals);
	}
    }

  g_free (proc_list);
}

static void
init_constants ()
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "gimp_data_dir",
				    PARAM_END);
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    setvar (cintern ("gimp-data-dir"), strcons (-1, return_vals[1].data.d_string), NIL);
  gimp_destroy_params (return_vals, nreturn_vals);

  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "gimp_plugin_dir",
				    PARAM_END);
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    setvar (cintern ("gimp-plugin-dir"), strcons (-1, return_vals[1].data.d_string), NIL);
  gimp_destroy_params (return_vals, nreturn_vals);

  setvar (cintern ("NORMAL"), flocons (0), NIL);
  setvar (cintern ("DISSOLVE"), flocons (1), NIL);
  setvar (cintern ("BEHIND"), flocons (2), NIL);
  setvar (cintern ("MULTIPLY"), flocons (3), NIL);
  setvar (cintern ("SCREEN"), flocons (4), NIL);
  setvar (cintern ("OVERLAY"), flocons (5), NIL);
  setvar (cintern ("DIFFERENCE"), flocons (6), NIL);
  setvar (cintern ("ADDITION"), flocons (7), NIL);
  setvar (cintern ("SUBTRACT"), flocons (8), NIL);
  setvar (cintern ("DARKEN-ONLY"), flocons (9), NIL);
  setvar (cintern ("LIGHTEN-ONLY"), flocons (10), NIL);
  setvar (cintern ("HUE"), flocons (11), NIL);
  setvar (cintern ("SATURATION"), flocons (12), NIL);
  setvar (cintern ("COLOR"), flocons (13), NIL);
  setvar (cintern ("VALUE"), flocons (14), NIL);

  setvar (cintern ("FG-BG-RGB"), flocons (0), NIL);
  setvar (cintern ("FG-BG-HSV"), flocons (1), NIL);
  setvar (cintern ("FG-TRANS"), flocons (2), NIL);
  setvar (cintern ("CUSTOM"), flocons (3), NIL);

  setvar (cintern ("LINEAR"), flocons (0), NIL);
  setvar (cintern ("BILINEAR"), flocons (1), NIL);
  setvar (cintern ("RADIAL"), flocons (2), NIL);
  setvar (cintern ("SQUARE"), flocons (3), NIL);
  setvar (cintern ("CONICAL-SYMMETRIC"), flocons (4), NIL);
  setvar (cintern ("CONICAL-ASYMMETRIC"), flocons (5), NIL);
  setvar (cintern ("SHAPEBURST-ANGULAR"), flocons (6), NIL);
  setvar (cintern ("SHAPEBURST-SPHERICAL"), flocons (7), NIL);
  setvar (cintern ("SHAPEBURST-DIMPLED"), flocons (8), NIL);

  setvar (cintern ("REPEAT-NONE"), flocons(0), NIL);
  setvar (cintern ("REPEAT-SAWTOOTH"), flocons(1), NIL);
  setvar (cintern ("REPEAT-TRIANGULAR"), flocons(2), NIL);

  setvar (cintern ("FG-BUCKET-FILL"), flocons (0), NIL);
  setvar (cintern ("BG-BUCKET-FILL"), flocons (1), NIL);
  setvar (cintern ("PATTERN-BUCKET-FILL"), flocons (2), NIL);

  setvar (cintern ("BG-IMAGE-FILL"), flocons (0), NIL);
  setvar (cintern ("WHITE-IMAGE-FILL"), flocons (1), NIL);
  setvar (cintern ("TRANS-IMAGE-FILL"), flocons (2), NIL);
  setvar (cintern ("NO-IMAGE-FILL"), flocons (3), NIL);

  setvar (cintern ("RGB"), flocons (0), NIL);
  setvar (cintern ("GRAY"), flocons (1), NIL);
  setvar (cintern ("INDEXED"), flocons (2), NIL);

  setvar (cintern ("RGB_IMAGE"), flocons (0), NIL);
  setvar (cintern ("RGBA_IMAGE"), flocons (1), NIL);
  setvar (cintern ("GRAY_IMAGE"), flocons (2), NIL);
  setvar (cintern ("GRAYA_IMAGE"), flocons (3), NIL);
  setvar (cintern ("INDEXED_IMAGE"), flocons (4), NIL);
  setvar (cintern ("INDEXEDA_IMAGE"), flocons (5), NIL);

  setvar (cintern ("RED-CHANNEL"), flocons (0), NIL);
  setvar (cintern ("GREEN-CHANNEL"), flocons (1), NIL);
  setvar (cintern ("BLUE-CHANNEL"), flocons (2), NIL);
  setvar (cintern ("GRAY-CHANNEL"), flocons (3), NIL);
  setvar (cintern ("INDEXED-CHANNEL"), flocons (4), NIL);

  setvar (cintern ("WHITE-MASK"), flocons (0), NIL);
  setvar (cintern ("BLACK-MASK"), flocons (1), NIL);
  setvar (cintern ("ALPHA-MASK"), flocons (2), NIL);

  setvar (cintern ("APPLY"), flocons (0), NIL);
  setvar (cintern ("DISCARD"), flocons (1), NIL);

  setvar (cintern ("EXPAND-AS-NECESSARY"), flocons (0), NIL);
  setvar (cintern ("CLIP-TO-IMAGE"), flocons (1), NIL);
  setvar (cintern ("CLIP-TO-BOTTOM-LAYER"), flocons (2), NIL);

  setvar (cintern ("ADD"), flocons (0), NIL);
  setvar (cintern ("SUB"), flocons (1), NIL);
  setvar (cintern ("REPLACE"), flocons (2), NIL);
  setvar (cintern ("INTERSECT"), flocons (3), NIL);

  setvar (cintern ("PIXELS"), flocons (0), NIL);
  setvar (cintern ("POINTS"), flocons (1), NIL);

  setvar (cintern ("IMAGE-CLONE"), flocons (0), NIL);
  setvar (cintern ("PATTERN-CLONE"), flocons (1), NIL);

  setvar (cintern ("BLUR"), flocons (0), NIL);
  setvar (cintern ("SHARPEN"), flocons (1), NIL);

  setvar (cintern ("TRUE"), flocons (1), NIL);
  setvar (cintern ("FALSE"), flocons (0), NIL);

  /*  Script-fu types  */
  setvar (cintern ("SF-IMAGE"), flocons (SF_IMAGE), NIL);
  setvar (cintern ("SF-DRAWABLE"), flocons (SF_DRAWABLE), NIL);
  setvar (cintern ("SF-LAYER"), flocons (SF_LAYER), NIL);
  setvar (cintern ("SF-CHANNEL"), flocons (SF_CHANNEL), NIL);
  setvar (cintern ("SF-COLOR"), flocons (SF_COLOR), NIL);
  setvar (cintern ("SF-TOGGLE"), flocons (SF_TOGGLE), NIL);
  setvar (cintern ("SF-VALUE"), flocons (SF_VALUE), NIL);
}

static void
convert_string (str)
     char *str;
{
  while (*str)
    {
      if (*str == '_') *str = '-';
      str++;
    }
}

static int
sputs_fcn (char *st, void *dest)
{
  strcpy (*((char**)dest), st);
  *((char**)dest) += strlen(st);
  return (1);
}

static LISP
lprin1s (LISP exp, char *dest)
{
  struct gen_printio s;
  s.putc_fcn = NULL;
  s.puts_fcn = sputs_fcn;
  s.cb_argument = &dest;
  lprin1g (exp, &s);
  return (NIL);
}

static LISP
marshall_proc_db_call (LISP a)
{
  GParam *args;
  GParam *values;
  int nvalues;
  char *proc_name;
  char *proc_blurb;
  char *proc_help;
  char *proc_author;
  char *proc_copyright;
  char *proc_date;
  int proc_type;
  int nparams;
  int nreturn_vals;
  GParamDef *params;
  GParamDef *return_vals;
  char error_str[256];
  int i;
  int success = TRUE;
  LISP color_list;
  LISP intermediate_val;
  LISP return_val;
  char *string;
  int string_len;
  LISP a_saved;

  /* Save a in case it is needed for an error message. */
  a_saved = a;

  /*  Make sure there are arguments  */
  if (a == NIL)
    return err ("Procedure database argument marshaller was called with no arguments.  The procedure to be executed and the arguments it requires (possibly none) must be specified.", NIL);

  /*  Derive the pdb procedure name from the argument or first argument of a list  */
  if (TYPEP (a, tc_cons))
    proc_name = get_c_string (car (a));
  else
    proc_name = get_c_string (a);

  /*  report the current command  */
  script_fu_report_cc (proc_name);

  /*  Attempt to fetch the procedure from the database  */
  if (gimp_query_procedure (proc_name, &proc_blurb, &proc_help, &proc_author,
			    &proc_copyright, &proc_date, &proc_type, &nparams, &nreturn_vals,
			    &params, &return_vals) == FALSE)
    return err ("Invalid procedure name specified.", NIL);


  /*  Check the supplied number of arguments  */
  if ((nlength (a) - 1) != nparams)
    {
      sprintf (error_str, "Invalid arguments supplied to %s--(# args: %ld, expecting: %d)",
	       proc_name, (nlength (a) - 1), nparams);
      return err (error_str, NIL);
    }

  /*  Marshall the supplied arguments  */
  if (nparams)
    args = (GParam *) g_new (GParam, nparams);
  else
    args = NULL;

  a = cdr (a);
  for (i = 0; i < nparams; i++)
    {
      switch (params[i].type)
	{
	case PARAM_INT32:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_INT32;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;
	case PARAM_INT16:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_INT16;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;
	case PARAM_INT8:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_INT8;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;
	case PARAM_FLOAT:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_FLOAT;
	      args[i].data.d_float = get_c_double (car (a));
	    }
	  break;
	case PARAM_STRING:
	  if (!TYPEP (car (a), tc_string))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_STRING;
	      args[i].data.d_string = get_c_string (car (a));
	    }
	  break;
	case PARAM_INT32ARRAY:
	  if (!TYPEP (car (a), tc_long_array))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_INT32ARRAY;
	      args[i].data.d_int32array = (gint32*) (car (a))->storage_as.long_array.data;
	    }
	  break;
	case PARAM_INT16ARRAY:
	  if (!TYPEP (car (a), tc_long_array))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_INT16ARRAY;
	      args[i].data.d_int16array = (short *) (car (a))->storage_as.long_array.data;
	    }
	  break;
	case PARAM_INT8ARRAY:
	  if (!TYPEP (car (a), tc_byte_array))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_INT8ARRAY;
	      args[i].data.d_int8array = (gint8 *) (car (a))->storage_as.string.data;
	    }
	  break;
	case PARAM_FLOATARRAY:
	  if (!TYPEP (car (a), tc_double_array))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_FLOATARRAY;
	      args[i].data.d_floatarray = (car (a))->storage_as.double_array.data;
	    }
	  break;
	case PARAM_STRINGARRAY:
	  if (!TYPEP (car (a), tc_cons))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_STRINGARRAY;

	      /*  Set the array  */
	      {
		gint j;
		gint num_strings;
		gchar **array;
		LISP list;

		list = car (a);
		num_strings = args[i - 1].data.d_int32;
		if (nlength (list) != num_strings)
		  return err ("String array argument has incorrectly specified length", NIL);
		array = args[i].data.d_stringarray = g_new (char *, num_strings);

		for (j = 0; j < num_strings; j++)
		  {
		    array[j] = get_c_string (car (list));
		    list = cdr (list);
		  }
	      }
	    }
	  break;
	case PARAM_COLOR:
	  if (!TYPEP (car (a), tc_cons))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_COLOR;
	      color_list = car (a);
	      args[i].data.d_color.red = get_c_long (car (color_list));
	      color_list = cdr (color_list);
	      args[i].data.d_color.green = get_c_long (car (color_list));
	      color_list = cdr (color_list);
	      args[i].data.d_color.blue = get_c_long (car (color_list));
	    }
	  break;
	case PARAM_REGION:
	  return err ("Regions are currently unsupported as arguments", car (a));
	  break;
	case PARAM_DISPLAY:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_DISPLAY;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;
	case PARAM_IMAGE:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_IMAGE;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;
	case PARAM_LAYER:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_LAYER;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;
	case PARAM_CHANNEL:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_CHANNEL;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;
	case PARAM_DRAWABLE:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_DRAWABLE;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;
	case PARAM_SELECTION:
	  if (!TYPEP (car (a), tc_flonum))
	    success = FALSE;
	  if (success)
	    {
	      args[i].type = PARAM_SELECTION;
	      args[i].data.d_int32 = get_c_long (car (a));
	    }
	  break;
	case PARAM_BOUNDARY:
	  return err ("Boundaries are currently unsupported as arguments", car (a));
	  break;
	case PARAM_PATH:
	  return err ("Paths are currently unsupported as arguments", car (a));
	  break;
	case PARAM_STATUS:
	  return err ("Status is for return types, not arguments", car (a));
	  break;
	default:
	  return err ("Unknown argument type", NIL);
	}

      a = cdr (a);
    }

  if (success)
    values = gimp_run_procedure2 (proc_name, &nvalues, nparams, args);
  else
    return err ("Invalid types specified for arguments", NIL);

  /*  Check the return status  */
  if (! values)
	{
	  strcpy (error_str, "Procedural database execution did not return a status:\n    ");
	  lprin1s (a_saved, error_str + strlen(error_str));
      return err (error_str, NIL);
	}
  switch (values[0].data.d_status)
    {
    case STATUS_EXECUTION_ERROR:
	  strcpy (error_str, "Procedural database execution failed:\n    ");
	  lprin1s (a_saved, error_str + strlen(error_str));
      return err (error_str, NIL);
      break;
    case STATUS_CALLING_ERROR:
	  strcpy (error_str, "Procedural database execution failed on invalid input arguments:\n    ");
	  lprin1s (a_saved, error_str + strlen(error_str));
      return err (error_str, NIL);
      break;
    case STATUS_SUCCESS:
      return_val = NIL;

      for (i = 0; i < nvalues - 1; i++)
	{
	  switch (return_vals[i].type)
	    {
	    case PARAM_INT32:
	      return_val = cons (flocons (values[i + 1].data.d_int32), return_val);
	      break;
	    case PARAM_INT16:
	      return_val = cons (flocons (values[i + 1].data.d_int32), return_val);
	      break;
	    case PARAM_INT8:
	      return_val = cons (flocons (values[i + 1].data.d_int32), return_val);
	      break;
	    case PARAM_FLOAT:
	      return_val = cons (flocons (values[i + 1].data.d_float), return_val);
	      break;
	    case PARAM_STRING:
	      string = g_strdup ((gchar *) values[i + 1].data.d_string);
	      string_len = strlen (string);
	      return_val = cons (strcons (string_len, string), return_val);
	      break;
	    case PARAM_INT32ARRAY:
	      {
		LISP array;
		int j;

		array = arcons (tc_long_array, values[i].data.d_int32, 0);
		for (j = 0; j < values[i].data.d_int32; j++)
		  array->storage_as.long_array.data[j] = values[i + 1].data.d_int32array[j];

		return_val = cons (array, return_val);
	      }
	      break;
	    case PARAM_INT16ARRAY:
	      return err ("Arrays are currently unsupported as return values", NIL);
	      break;
	    case PARAM_INT8ARRAY:
	      {
		LISP array;
		int j;

		array = arcons (tc_byte_array, values[i].data.d_int32, 0);
		for (j = 0; j < values[i].data.d_int32; j++)
		  array->storage_as.string.data[j] = values[i + 1].data.d_int8array[j];

		return_val = cons (array, return_val);
	      }
	      break;
	    case PARAM_FLOATARRAY:
	      {
		LISP array;
		int j;

		array = arcons (tc_double_array, values[i].data.d_int32, 0);
		for (j = 0; j < values[i].data.d_int32; j++)
		  array->storage_as.double_array.data[j] = values[i + 1].data.d_floatarray[j];

		return_val = cons (array, return_val);
	      }
	      break;
	    case PARAM_STRINGARRAY:
	      /*  string arrays are always implemented such that the previous
	       *  return value contains the number of strings in the array
	       */
	      {
		int j;
		int num_strings = values[i].data.d_int32;
		LISP string_array = NIL;
		char **array = (char **) values[i + 1].data.d_stringarray;

		for (j = 0; j < num_strings; j++)
		  {
		    string_len = strlen (array[j]);
		    string_array = cons (strcons (string_len, g_strdup (array[j])), string_array);
		  }

		return_val = cons (nreverse (string_array), return_val);
	      }
	      break;
	    case PARAM_COLOR:
	      intermediate_val = cons (flocons ((int) values[i + 1].data.d_color.red),
				       cons (flocons ((int) values[i + 1].data.d_color.green),
					     cons (flocons ((int) values[i + 1].data.d_color.blue),
						   NIL)));
	      return_val = cons (intermediate_val, return_val);
	      break;
	    case PARAM_REGION:
	      return err ("Regions are currently unsupported as return values", NIL);
	      break;
	    case PARAM_DISPLAY:
	      return_val = cons (flocons (values[i + 1].data.d_int32), return_val);
	      break;
	    case PARAM_IMAGE:
	      return_val = cons (flocons (values[i + 1].data.d_int32), return_val);
	      break;
	    case PARAM_LAYER:
	      return_val = cons (flocons (values[i + 1].data.d_int32), return_val);
	      break;
	    case PARAM_CHANNEL:
	      return_val = cons (flocons (values[i + 1].data.d_int32), return_val);
	      break;
	    case PARAM_DRAWABLE:
	      return_val = cons (flocons (values[i + 1].data.d_int32), return_val);
	      break;
	    case PARAM_SELECTION:
	      return_val = cons (flocons (values[i + 1].data.d_int32), return_val);
	      break;
	    case PARAM_BOUNDARY:
	      return err ("Boundaries are currently unsupported as return values", NIL);
	      break;
	    case PARAM_PATH:
	      return err ("Paths are currently unsupported as return values", NIL);
	      break;
	    case PARAM_STATUS:
	      return err ("Procedural database execution returned multiple status values", NIL);
	      break;
	    default:
	      return err ("Unknown return type", NIL);
	    }
	}
      break;
    }

  /*  free up the executed procedure return values  */
  gimp_destroy_params (values, nvalues);

  /*  free up arguments and values  */
  if (args)
    g_free (args);

  /*  free the query information  */
  g_free (proc_blurb);
  g_free (proc_help);
  g_free (proc_author);
  g_free (proc_copyright);
  g_free (proc_date);
  g_free (params);
  g_free (return_vals);

  /*  reverse the return values  */
  return_val = nreverse (return_val);

  /*  if we're in server mode, listen for additional commands for 10 ms  */
  if (server_mode)
    script_fu_server_listen (10);

  return return_val;
}

static LISP
script_fu_register_call (LISP a)
{
  if (script_fu_base)
    return script_fu_add_script (a);
  else
    return NIL;
}

static LISP
script_fu_quit_call (LISP a)
{
  script_fu_done = TRUE;

  return NIL;
}

static void
script_fu_auxillary_init ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "[Interactive], non-interactive" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_temp_proc ("script_fu_refresh",
			  "Re-read all available scripts",
			  "Re-read all available scripts",
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1997",
			  "<Toolbox>/Xtns/Script-Fu/Refresh",
			  "",
			  PROC_TEMPORARY,
			  nargs, 0,
			  args, NULL,
			  script_fu_refresh_proc);
}

static void
script_fu_refresh_proc (char     *name,
			int       nparams,
			GParam   *params,
			int      *nreturn_vals,
			GParam  **return_vals)
{
  static GParam values[1];
  GStatusType status = STATUS_SUCCESS;

  /*  Reload all of the available scripts  */
  script_fu_find_scripts ();

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}
