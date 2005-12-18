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

#include <string.h> /* memcpy, strcpy, strlen */

#include <gtk/gtk.h>

#include "libgimp/gimp.h"

#include "siod/siod.h"

#include "script-fu-types.h"

#include "script-fu-console.h"
#include "script-fu-interface.h"
#include "script-fu-scripts.h"
#include "script-fu-server.h"

#include "siod-wrapper.h"

static int    siod_console_mode;

/* global variables declared by the scheme interpreter */
extern FILE * siod_output;
extern int    siod_verbose_level;
extern gchar  siod_err_msg[];
extern void * siod_output_routine;
extern LISP   repl_return_val;


/* defined in regex.c. not exported by regex.h */
extern void  init_regex   (void);

/* defined in siodp.h but this file cannot be imported... */
extern long  nlength (LISP obj);
extern LISP  leval_define (LISP args, LISP env);


/* wrapper functions */
FILE *
siod_get_output_file (void)
{
  return siod_output;
}

void
siod_set_output_file (FILE *file)
{
  siod_output = file;
}

void
siod_set_console_mode (int flag)
{
  siod_console_mode = flag;
}

int
siod_get_verbose_level (void)
{
  return siod_verbose_level;
}


void
siod_set_verbose_level (gint verbose_level)
{
  siod_verbose_level = verbose_level;
}

void
siod_print_welcome (void)
{
  print_welcome ();
}

gint
siod_interpret_string (const gchar *expr)
{
  return repl_c_string ((char *)expr, 0, 0, 1);
}

const char *
siod_get_error_msg (void)
{
  return siod_err_msg;
}

const gchar *
siod_get_success_msg (void)
{
  if (TYPEP (repl_return_val, tc_string))
    return get_c_string (repl_return_val);
  else
    return "Success";
}

void
siod_output_string (FILE        *fp,
                    const gchar *format,
                    ...)
{
  gchar   *buf;
  va_list  args;

  va_start (args, format);
  buf = g_strdup_vprintf (format, args);
  va_end (args);

  if (siod_console_mode && fp == stdout)
    {
      script_fu_output_to_console (buf);
    }
  else
    {
      fprintf (fp, buf);
      fflush (fp);
    }

  g_free (buf);
}


static void  init_constants   (void);
static void  init_procedures  (void);

static gboolean register_scripts = FALSE;

void
siod_init (gboolean local_register_scripts)
{
  char *siod_argv[] =
  {
    "siod",
    "-h100000:10",
    "-g0",
    "-o1000",
    "-s200000",
    "-n2048",
    "-v0",
  };

  register_scripts = local_register_scripts;
  siod_output_routine = siod_output_string;

  /* init the interpreter */
  process_cla (G_N_ELEMENTS (siod_argv), siod_argv, 1);
  init_storage ();
  init_subrs ();
  init_trace ();
  init_regex ();

  /* register in the interpreter the gimp functions and types. */
  init_procedures ();
  init_constants ();

}

static void  convert_string               (gchar    *str);
static gint  sputs_fcn                    (gchar    *st,
                                           gpointer  dest);
static LISP  lprin1s                      (LISP      exp,
                                           gchar    *dest);
static LISP  marshall_proc_db_call        (LISP      a);
static LISP  script_fu_register_call      (LISP      a);
static LISP  script_fu_menu_register_call (LISP      a);
static LISP  script_fu_quit_call          (LISP      a);


/*
 * Below can be found the functions responsible for registering the
 * gimp functions and types against the scheme interpreter.
 */


static void
init_procedures (void)
{
  gchar          **proc_list;
  gchar           *proc_blurb;
  gchar           *proc_help;
  gchar           *proc_author;
  gchar           *proc_copyright;
  gchar           *proc_date;
  GimpPDBProcType  proc_type;
  gint             nparams;
  gint             nreturn_vals;
  GimpParamDef    *params;
  GimpParamDef    *return_vals;
  gint             num_procs;
  gint             i;

  /*  register the database execution procedure  */
  init_lsubr ("gimp-proc-db-call",       marshall_proc_db_call);
  init_lsubr ("script-fu-register",      script_fu_register_call);
  init_lsubr ("script-fu-menu-register", script_fu_menu_register_call);
  init_lsubr ("script-fu-quit",          script_fu_quit_call);

  gimp_procedural_db_query (".*", ".*", ".*", ".*", ".*", ".*", ".*",
                            &num_procs, &proc_list);

  /*  Register each procedure as a scheme func  */
  for (i = 0; i < num_procs; i++)
    {
      /*  lookup the procedure  */
      if (gimp_procedural_db_proc_info (proc_list[i],
                                        &proc_blurb,
                                        &proc_help,
                                        &proc_author,
                                        &proc_copyright,
                                        &proc_date,
                                        &proc_type,
                                        &nparams, &nreturn_vals,
                                        &params, &return_vals))
        {
          LISP   args = NIL;
          LISP   code = NIL;
          gint   j;

          /*  create a new scheme func that calls gimp-proc-db-call  */
          for (j = 0; j < nparams; j++)
            {
              args = cons (rintern (params[j].name), args);
              code = cons (rintern (params[j].name), code);
            }

          /*  reverse the list  */
          args = nreverse (args);
          code = nreverse (code);

          /*  set the procedure name  */
          args = cons (rintern (proc_list[i]), args);

          /*  set the actual pdb procedure name  */
          code = cons (cons (cintern ("quote"),
                             cons (rintern (proc_list[i]), NIL)), code);
          code = cons (cintern ("gimp-proc-db-call"), code);

          leval_define (cons (args, cons (code, NIL)), NIL);

          /*  free the queried information  */
          g_free (proc_blurb);
          g_free (proc_help);
          g_free (proc_author);
          g_free (proc_copyright);
          g_free (proc_date);

          gimp_destroy_paramdefs (params, nparams);
          gimp_destroy_paramdefs (return_vals, nreturn_vals);
        }

      g_free (proc_list[i]);
    }

  g_free (proc_list);
}

static void
init_constants (void)
{
  const gchar **enum_type_names;
  gint          n_enum_type_names;
  gint          i;
  GimpUnit      unit;

  setvar (cintern ("gimp-directory"),
          strcons (-1, (gchar *) gimp_directory ()), NIL);

  setvar (cintern ("gimp-data-directory"),
          strcons (-1, (gchar *) gimp_data_directory ()), NIL);

  setvar (cintern ("gimp-plug-in-directory"),
          strcons (-1, (gchar *) gimp_plug_in_directory ()), NIL);

  setvar (cintern ("gimp-locale-directory"),
          strcons (-1, (gchar *) gimp_locale_directory ()), NIL);

  setvar (cintern ("gimp-sysconf-directory"),
          strcons (-1, (gchar *) gimp_sysconf_directory ()), NIL);

  enum_type_names = gimp_enums_get_type_names (&n_enum_type_names);

  for (i = 0; i < n_enum_type_names; i++)
    {
      const gchar *enum_name  = enum_type_names[i];
      GType        enum_type  = g_type_from_name (enum_name);
      GEnumClass  *enum_class = g_type_class_ref (enum_type);
      GEnumValue  *value;

      for (value = enum_class->values; value->value_name; value++)
        {
          if (! strncmp ("GIMP_", value->value_name, 5))
            {
              gchar *scheme_name;

              scheme_name = g_strdup (value->value_name + 5);
              convert_string (scheme_name);

              setvar (rintern (scheme_name), flocons (value->value), NIL);

              g_free (scheme_name);
            }
        }

      g_type_class_unref (enum_class);
    }

  for (unit = GIMP_UNIT_PIXEL;
       unit < gimp_unit_get_number_of_built_in_units ();
       unit++)
    {
      gchar *tmp;
      gchar *scheme_name;

      tmp = g_ascii_strup (gimp_unit_get_singular (unit), -1);
      scheme_name = g_strconcat ("UNIT-", tmp, NULL);
      g_free (tmp);

      setvar (rintern (scheme_name), flocons (unit), NIL);

      g_free (scheme_name);
    }

  /* These are for backwards compatibility; they should be removed sometime */
  setvar (cintern ("gimp-dir"),
          strcons (-1, (gchar *) gimp_directory ()), NIL);

  setvar (cintern ("gimp-data-dir"),
          strcons (-1, (gchar *) gimp_data_directory ()), NIL);

  setvar (cintern ("gimp-plugin-dir"),
          strcons (-1, (gchar *) gimp_plug_in_directory ()), NIL);

  setvar (cintern ("NORMAL"),         flocons (GIMP_NORMAL_MODE),       NIL);
  setvar (cintern ("DISSOLVE"),       flocons (GIMP_DISSOLVE_MODE),     NIL);
  setvar (cintern ("BEHIND"),         flocons (GIMP_BEHIND_MODE),       NIL);
  setvar (cintern ("MULTIPLY"),       flocons (GIMP_MULTIPLY_MODE),     NIL);
  setvar (cintern ("SCREEN"),         flocons (GIMP_SCREEN_MODE),       NIL);
  setvar (cintern ("OVERLAY"),        flocons (GIMP_OVERLAY_MODE),      NIL);
  setvar (cintern ("DIFFERENCE"),     flocons (GIMP_DIFFERENCE_MODE),   NIL);
  setvar (cintern ("ADDITION"),       flocons (GIMP_ADDITION_MODE),     NIL);
  setvar (cintern ("SUBTRACT"),       flocons (GIMP_SUBTRACT_MODE),     NIL);
  setvar (cintern ("DARKEN-ONLY"),    flocons (GIMP_DARKEN_ONLY_MODE),  NIL);
  setvar (cintern ("LIGHTEN-ONLY"),   flocons (GIMP_LIGHTEN_ONLY_MODE), NIL);
  setvar (cintern ("HUE"),            flocons (GIMP_HUE_MODE),          NIL);
  setvar (cintern ("SATURATION"),     flocons (GIMP_SATURATION_MODE),   NIL);
  setvar (cintern ("COLOR"),          flocons (GIMP_COLOR_MODE),        NIL);
  setvar (cintern ("VALUE"),          flocons (GIMP_VALUE_MODE),        NIL);
  setvar (cintern ("DIVIDE"),         flocons (GIMP_DIVIDE_MODE),       NIL);

  setvar (cintern ("BLUR"),           flocons (GIMP_BLUR_CONVOLVE),     NIL);
  setvar (cintern ("SHARPEN"),        flocons (GIMP_SHARPEN_CONVOLVE),  NIL);

  setvar (cintern ("WHITE-MASK"),     flocons (GIMP_ADD_WHITE_MASK),    NIL);
  setvar (cintern ("BLACK-MASK"),     flocons (GIMP_ADD_BLACK_MASK),    NIL);
  setvar (cintern ("ALPHA-MASK"),     flocons (GIMP_ADD_ALPHA_MASK),    NIL);

  setvar (cintern ("ADD"),         flocons (GIMP_CHANNEL_OP_ADD),       NIL);
  setvar (cintern ("SUB"),         flocons (GIMP_CHANNEL_OP_SUBTRACT),  NIL);
  setvar (cintern ("REPLACE"),     flocons (GIMP_CHANNEL_OP_REPLACE),   NIL);
  setvar (cintern ("INTERSECT"),   flocons (GIMP_CHANNEL_OP_INTERSECT), NIL);

  setvar (cintern ("FG-BG-RGB"),   flocons (GIMP_FG_BG_RGB_MODE),       NIL);
  setvar (cintern ("FG-BG-HSV"),   flocons (GIMP_FG_BG_HSV_MODE),       NIL);
  setvar (cintern ("FG-TRANS"),    flocons (GIMP_FG_TRANSPARENT_MODE),  NIL);
  setvar (cintern ("CUSTOM"),      flocons (GIMP_CUSTOM_MODE),          NIL);

  setvar (cintern ("FG-IMAGE-FILL"),    flocons (GIMP_FOREGROUND_FILL),  NIL);
  setvar (cintern ("BG-IMAGE-FILL"),    flocons (GIMP_BACKGROUND_FILL),  NIL);
  setvar (cintern ("WHITE-IMAGE-FILL"), flocons (GIMP_WHITE_FILL),       NIL);
  setvar (cintern ("TRANS-IMAGE-FILL"), flocons (GIMP_TRANSPARENT_FILL), NIL);

  setvar (cintern ("APPLY"),     flocons (GIMP_MASK_APPLY),   NIL);
  setvar (cintern ("DISCARD"),   flocons (GIMP_MASK_DISCARD), NIL);

  setvar (cintern ("HARD"),      flocons (GIMP_BRUSH_HARD), NIL);
  setvar (cintern ("SOFT"),      flocons (GIMP_BRUSH_SOFT), NIL);

  setvar (cintern ("CONTINUOUS"),  flocons (GIMP_PAINT_CONSTANT),    NIL);
  setvar (cintern ("INCREMENTAL"), flocons (GIMP_PAINT_INCREMENTAL), NIL);

  setvar (cintern ("HORIZONTAL"), flocons (GIMP_ORIENTATION_HORIZONTAL), NIL);
  setvar (cintern ("VERTICAL"),   flocons (GIMP_ORIENTATION_VERTICAL),   NIL);
  setvar (cintern ("UNKNOWN"),    flocons (GIMP_ORIENTATION_UNKNOWN),    NIL);

  setvar (cintern ("LINEAR"),               flocons (GIMP_GRADIENT_LINEAR),               NIL);
  setvar (cintern ("BILINEAR"),             flocons (GIMP_GRADIENT_BILINEAR),             NIL);
  setvar (cintern ("RADIAL"),               flocons (GIMP_GRADIENT_RADIAL),               NIL);
  setvar (cintern ("SQUARE"),               flocons (GIMP_GRADIENT_SQUARE),               NIL);
  setvar (cintern ("CONICAL-SYMMETRIC"),    flocons (GIMP_GRADIENT_CONICAL_SYMMETRIC),    NIL);
  setvar (cintern ("CONICAL-ASYMMETRIC"),   flocons (GIMP_GRADIENT_CONICAL_ASYMMETRIC),   NIL);
  setvar (cintern ("SHAPEBURST-ANGULAR"),   flocons (GIMP_GRADIENT_SHAPEBURST_ANGULAR),   NIL);
  setvar (cintern ("SHAPEBURST-SPHERICAL"), flocons (GIMP_GRADIENT_SHAPEBURST_SPHERICAL), NIL);
  setvar (cintern ("SHAPEBURST-DIMPLED"),   flocons (GIMP_GRADIENT_SHAPEBURST_DIMPLED),   NIL);
  setvar (cintern ("SPIRAL-CLOCKWISE"),     flocons (GIMP_GRADIENT_SPIRAL_CLOCKWISE),     NIL);
  setvar (cintern ("SPIRAL-ANTICLOCKWISE"), flocons (GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE), NIL);

  setvar (cintern ("VALUE-LUT"), flocons (GIMP_HISTOGRAM_VALUE), NIL);
  setvar (cintern ("RED-LUT"),   flocons (GIMP_HISTOGRAM_RED),   NIL);
  setvar (cintern ("GREEN-LUT"), flocons (GIMP_HISTOGRAM_GREEN), NIL);
  setvar (cintern ("BLUE-LUT"),  flocons (GIMP_HISTOGRAM_BLUE),  NIL);
  setvar (cintern ("ALPHA-LUT"), flocons (GIMP_HISTOGRAM_ALPHA), NIL);

  /* Useful values from libgimpbase/gimplimits.h */
  setvar (cintern ("MIN-IMAGE-SIZE"), flocons (GIMP_MIN_IMAGE_SIZE), NIL);
  setvar (cintern ("MAX-IMAGE-SIZE"), flocons (GIMP_MAX_IMAGE_SIZE), NIL);
  setvar (cintern ("MIN-RESOLUTION"), flocons (GIMP_MIN_RESOLUTION), NIL);
  setvar (cintern ("MAX-RESOLUTION"), flocons (GIMP_MAX_RESOLUTION), NIL);

  /* Useful misc stuff */
  setvar (cintern ("TRUE"),           flocons (TRUE),  NIL);
  setvar (cintern ("FALSE"),          flocons (FALSE), NIL);

  /*  Script-fu types  */
  setvar (cintern ("SF-IMAGE"),       flocons (SF_IMAGE),      NIL);
  setvar (cintern ("SF-DRAWABLE"),    flocons (SF_DRAWABLE),   NIL);
  setvar (cintern ("SF-LAYER"),       flocons (SF_LAYER),      NIL);
  setvar (cintern ("SF-CHANNEL"),     flocons (SF_CHANNEL),    NIL);
  setvar (cintern ("SF-COLOR"),       flocons (SF_COLOR),      NIL);
  setvar (cintern ("SF-TOGGLE"),      flocons (SF_TOGGLE),     NIL);
  setvar (cintern ("SF-VALUE"),       flocons (SF_VALUE),      NIL);
  setvar (cintern ("SF-STRING"),      flocons (SF_STRING),     NIL);
  setvar (cintern ("SF-FILENAME"),    flocons (SF_FILENAME),   NIL);
  setvar (cintern ("SF-DIRNAME"),     flocons (SF_DIRNAME),    NIL);
  setvar (cintern ("SF-ADJUSTMENT"),  flocons (SF_ADJUSTMENT), NIL);
  setvar (cintern ("SF-FONT"),        flocons (SF_FONT),       NIL);
  setvar (cintern ("SF-PATTERN"),     flocons (SF_PATTERN),    NIL);
  setvar (cintern ("SF-BRUSH"),       flocons (SF_BRUSH),      NIL);
  setvar (cintern ("SF-GRADIENT"),    flocons (SF_GRADIENT),   NIL);
  setvar (cintern ("SF-OPTION"),      flocons (SF_OPTION),     NIL);
  setvar (cintern ("SF-PALETTE"),     flocons (SF_PALETTE),    NIL);
  setvar (cintern ("SF-TEXT"),        flocons (SF_TEXT),       NIL);
  setvar (cintern ("SF-ENUM"),        flocons (SF_ENUM),       NIL);

  /* for SF_ADJUSTMENT */
  setvar (cintern ("SF-SLIDER"),      flocons (SF_SLIDER),     NIL);
  setvar (cintern ("SF-SPINNER"),     flocons (SF_SPINNER),    NIL);
}

static void
convert_string (gchar *str)
{
  while (*str)
    {
      if (*str == '_') *str = '-';
      str++;
    }
}

static gboolean
sputs_fcn (gchar    *st,
           gpointer  dest)
{
  strcpy (*((gchar**)dest), st);
  *((gchar**)dest) += strlen (st);

  return TRUE;
}

static LISP
lprin1s (LISP   exp,
         gchar *dest)
{
  struct gen_printio s;

  s.putc_fcn    = NULL;
  s.puts_fcn    = sputs_fcn;
  s.cb_argument = &dest;

  lprin1g (exp, &s);

  return (NIL);
}


static LISP
marshall_proc_db_call (LISP a)
{
  GimpParam       *args;
  GimpParam       *values = NULL;
  gint             nvalues;
  gchar           *proc_name;
  gchar           *proc_blurb;
  gchar           *proc_help;
  gchar           *proc_author;
  gchar           *proc_copyright;
  gchar           *proc_date;
  GimpPDBProcType  proc_type;
  gint             nparams;
  gint             nreturn_vals;
  GimpParamDef    *params;
  GimpParamDef    *return_vals;
  gchar            error_str[256];
  gint             i;
  gint             success = TRUE;
  LISP             intermediate_val;
  LISP             return_val = NIL;
  gchar           *string;
  gint             string_len;
  LISP             a_saved;

  /* Save a in case it is needed for an error message. */
  a_saved = a;

  /*  Make sure there are arguments  */
  if (a == NIL)
    return my_err ("Procedure database argument marshaller was called with no arguments. "
                   "The procedure to be executed and the arguments it requires "
                   "(possibly none) must be specified.", NIL);

  /*  Derive the pdb procedure name from the argument
      or first argument of a list  */
  if (TYPEP (a, tc_cons))
    proc_name = g_strdup (get_c_string (car (a)));
  else
    proc_name = g_strdup (get_c_string (a));

  /*  report the current command  */
  script_fu_interface_report_cc (proc_name);

  /*  Attempt to fetch the procedure from the database  */
  if (! gimp_procedural_db_proc_info (proc_name,
                                      &proc_blurb,
                                      &proc_help,
                                      &proc_author,
                                      &proc_copyright,
                                      &proc_date,
                                      &proc_type,
                                      &nparams, &nreturn_vals,
                                      &params, &return_vals))
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid procedure name %s specified", proc_name);
      return my_err (error_str, NIL);
    }

  /* Free the name and the description which are of no use here.  */
  for (i = 0; i < nparams; i++)
    {
      g_free (params[i].name);
      g_free (params[i].description);
    }
  for (i = 0; i < nreturn_vals; i++)
    {
      g_free (return_vals[i].name);
      g_free (return_vals[i].description);
    }

  /*  Check the supplied number of arguments  */
  if ((nlength (a) - 1) != nparams)
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid arguments supplied to %s -- "
                  "(# args: %ld, expecting: %d)",
                  proc_name, (nlength (a) - 1), nparams);
      return my_err (error_str, NIL);
    }

  /*  Marshall the supplied arguments  */
  if (nparams)
    args = g_new (GimpParam, nparams);
  else
    args = NULL;

  a = cdr (a);
  for (i = 0; i < nparams; i++)
    {
      switch (params[i].type)
        {
        case GIMP_PDB_INT32:
          if (!TYPEP (car (a), tc_flonum))
            success = FALSE;
          if (success)
            {
              args[i].type         = GIMP_PDB_INT32;
              args[i].data.d_int32 = get_c_long (car (a));
            }
          break;

        case GIMP_PDB_INT16:
          if (!TYPEP (car (a), tc_flonum))
            success = FALSE;
          if (success)
            {
              args[i].type         = GIMP_PDB_INT16;
              args[i].data.d_int16 = (gint16) get_c_long (car (a));
            }
          break;

        case GIMP_PDB_INT8:
          if (!TYPEP (car (a), tc_flonum))
            success = FALSE;
          if (success)
            {
              args[i].type        = GIMP_PDB_INT8;
              args[i].data.d_int8 = (gint8) get_c_long (car (a));
            }
          break;

        case GIMP_PDB_FLOAT:
          if (!TYPEP (car (a), tc_flonum))
            success = FALSE;
          if (success)
            {
              args[i].type         = GIMP_PDB_FLOAT;
              args[i].data.d_float = get_c_double (car (a));
            }
          break;

        case GIMP_PDB_STRING:
          if (!TYPEP (car (a), tc_string))
            success = FALSE;
          if (success)
            {
              args[i].type          = GIMP_PDB_STRING;
              args[i].data.d_string = get_c_string (car (a));
            }
          break;

        case GIMP_PDB_INT32ARRAY:
          if (!TYPEP (car (a), tc_long_array))
            success = FALSE;
          if (success)
            {
              gint n_elements = args[i - 1].data.d_int32;
              LISP list       = car (a);

              if ((n_elements < 0) || (n_elements > nlength (list)))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "INT32 array (argument %d) for function %s has "
                              "incorrect length (got %ld, expected %d)",
                              i + 1, proc_name, nlength (list), n_elements);
                  return my_err (error_str, NIL);
                }

              args[i].type              = GIMP_PDB_INT32ARRAY;
              args[i].data.d_int32array = (gint32 *)
                list->storage_as.long_array.data;
            }
          break;

        case GIMP_PDB_INT16ARRAY:
          if (!TYPEP (car (a), tc_long_array))
            success = FALSE;
          if (success)
            {
              gint n_elements = args[i - 1].data.d_int32;
              LISP list       = car (a);

              if ((n_elements < 0) || (n_elements > nlength (list)))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "INT16 array (argument %d) for function %s has "
                              "incorrect length (got %ld, expected %d)",
                              i + 1, proc_name, nlength (list), n_elements);
                  return my_err (error_str, NIL);
                }

              args[i].type              = GIMP_PDB_INT16ARRAY;
              args[i].data.d_int16array = (gint16 *)
                list->storage_as.long_array.data;
            }
          break;

        case GIMP_PDB_INT8ARRAY:
          if (!TYPEP (car (a), tc_byte_array))
            success = FALSE;
          if (success)
            {
              gint n_elements = args[i - 1].data.d_int32;
              LISP list       = car (a);

              if ((n_elements < 0) || (n_elements > nlength (list)))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "INT8 array (argument %d) for function %s has "
                              "incorrect length (got %ld, expected %d)",
                              i + 1, proc_name, nlength (list), n_elements);
                  return my_err (error_str, NIL);
                }

              args[i].type             = GIMP_PDB_INT8ARRAY;
              args[i].data.d_int8array = (gint8 *) list->storage_as.string.data;
            }
          break;

        case GIMP_PDB_FLOATARRAY:
          if (!TYPEP (car (a), tc_double_array))
            success = FALSE;
          if (success)
            {
              gint n_elements = args[i - 1].data.d_int32;
              LISP list       = car (a);

              if ((n_elements < 0) || (n_elements > nlength (list)))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "FLOAT array (argument %d) for function %s has "
                              "incorrect length (got %ld, expected %d)",
                              i + 1, proc_name, nlength (list), n_elements);
                  return my_err (error_str, NIL);
                }

              args[i].type              = GIMP_PDB_FLOATARRAY;
              args[i].data.d_floatarray = list->storage_as.double_array.data;
            }
          break;

        case GIMP_PDB_STRINGARRAY:
          if (!TYPEP (car (a), tc_cons))
            success = FALSE;
          if (success)
            {
              gint n_elements = args[i - 1].data.d_int32;
              LISP list       = car (a);
              gint j;

              if ((n_elements < 0) || (n_elements > nlength (list)))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "String array (argument %d) for function %s has "
                              "incorrect length (got %ld, expected %d)",
                              i + 1, proc_name, nlength (list), n_elements);
                  return my_err (error_str, NIL);
                }

              args[i].type               = GIMP_PDB_STRINGARRAY;
              args[i].data.d_stringarray = g_new0 (gchar *, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  args[i].data.d_stringarray[j] = get_c_string (car (list));
                  list = cdr (list);
                }
            }
          break;

        case GIMP_PDB_COLOR:
          if (!TYPEP (car (a), tc_cons))
            success = FALSE;
          if (success)
            {
              LISP   color_list;
              guchar r, g, b;

              args[i].type = GIMP_PDB_COLOR;
              color_list = car (a);
              r = CLAMP (get_c_long (car (color_list)), 0, 255);
              color_list = cdr (color_list);
              g = CLAMP (get_c_long (car (color_list)), 0, 255);
              color_list = cdr (color_list);
              b = CLAMP (get_c_long (car (color_list)), 0, 255);

              gimp_rgba_set_uchar (&args[i].data.d_color, r, g, b, 255);
            }
          break;

        case GIMP_PDB_REGION:
          return my_err ("Regions are currently unsupported as arguments",
                         car (a));
          break;

        case GIMP_PDB_DISPLAY:
          if (!TYPEP (car (a), tc_flonum))
            success = FALSE;
          if (success)
            {
              args[i].type = GIMP_PDB_DISPLAY;
              args[i].data.d_int32 = get_c_long (car (a));
            }
          break;

        case GIMP_PDB_IMAGE:
          if (!TYPEP (car (a), tc_flonum))
            success = FALSE;
          if (success)
            {
              args[i].type = GIMP_PDB_IMAGE;
              args[i].data.d_int32 = get_c_long (car (a));
            }
          break;

        case GIMP_PDB_LAYER:
          if (!TYPEP (car (a), tc_flonum))
            success = FALSE;
          if (success)
            {
              args[i].type = GIMP_PDB_LAYER;
              args[i].data.d_int32 = get_c_long (car (a));
            }
          break;

        case GIMP_PDB_CHANNEL:
          if (!TYPEP (car (a), tc_flonum))
            success = FALSE;
          if (success)
            {
              args[i].type = GIMP_PDB_CHANNEL;
              args[i].data.d_int32 = get_c_long (car (a));
            }
          break;

        case GIMP_PDB_DRAWABLE:
          if (!TYPEP (car (a), tc_flonum))
            success = FALSE;
          if (success)
            {
              args[i].type = GIMP_PDB_DRAWABLE;
              args[i].data.d_int32 = get_c_long (car (a));
            }
          break;

        case GIMP_PDB_SELECTION:
          if (!TYPEP (car (a), tc_flonum))
            success = FALSE;
          if (success)
            {
              args[i].type = GIMP_PDB_SELECTION;
              args[i].data.d_int32 = get_c_long (car (a));
            }
          break;

        case GIMP_PDB_BOUNDARY:
          return my_err ("Boundaries are currently unsupported as arguments",
                         car (a));
          break;

        case GIMP_PDB_PATH:
          if (!TYPEP (car (a), tc_flonum))
            success = FALSE;
          if (success)
            {
              args[i].type = GIMP_PDB_PATH;
              args[i].data.d_int32 = get_c_long (car (a));
            }
          break;

        case GIMP_PDB_PARASITE:
          if (!TYPEP (car (a), tc_cons))
            success = FALSE;
          if (success)
            {
              args[i].type = GIMP_PDB_PARASITE;

              /* parasite->name */
              intermediate_val = car (a);

              if (!TYPEP (car (intermediate_val), tc_string))
                {
                  success = FALSE;
                  break;
                }

              args[i].data.d_parasite.name =
                get_c_string (car (intermediate_val));

              /* parasite->flags */
              intermediate_val = cdr (intermediate_val);

              if (!TYPEP (car (intermediate_val), tc_flonum))
                {
                  success = FALSE;
                  break;
                }

              args[i].data.d_parasite.flags =
                get_c_long (car (intermediate_val));

              /* parasite->size */
              intermediate_val = cdr (intermediate_val);

              if (!TYPEP (car (intermediate_val), tc_string) &&
                  !TYPEP (car (intermediate_val), tc_byte_array))
                {
                  success = FALSE;
                  break;
                }

              args[i].data.d_parasite.size =
                (car (intermediate_val))->storage_as.string.dim;

              /* parasite->data */
              args[i].data.d_parasite.data =
                (car (intermediate_val))->storage_as.string.data;
            }
          break;

        case GIMP_PDB_STATUS:
          return my_err ("Status is for return types, not arguments", car (a));
          break;

        default:
          g_snprintf (error_str, sizeof (error_str),
                      "Argument %d for %s is an unknown type",
                      i + 1, proc_name);
          return my_err (error_str, NIL);
        }

      if (!success)
        break;

      a = cdr (a);
    }

  if (success)
    {
      values = gimp_run_procedure2 (proc_name, &nvalues, nparams, args);
    }
  else
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid type for argument %d to %s", i + 1, proc_name);
      return my_err (error_str, NIL);
    }

  /*  Check the return status  */
  if (! values)
    {
      strcpy (error_str,
              "Procedural database execution did not return a status:\n    ");
      lprin1s (a_saved, error_str + strlen (error_str));

      return my_err (error_str, NIL);
    }

  switch (values[0].data.d_status)
    {
    case GIMP_PDB_EXECUTION_ERROR:
      strcpy (error_str,
              "Procedural database execution failed:\n    ");
      lprin1s (a_saved, error_str + strlen (error_str));
      return my_err (error_str, NIL);
      break;

    case GIMP_PDB_CALLING_ERROR:
      strcpy (error_str,
              "Procedural database execution failed on invalid input arguments:\n    ");
      lprin1s (a_saved, error_str + strlen (error_str));
      return my_err (error_str, NIL);
      break;

    case GIMP_PDB_SUCCESS:
      return_val = NIL;

      for (i = 0; i < nvalues - 1; i++)
        {
          switch (return_vals[i].type)
            {
            case GIMP_PDB_INT32:
              return_val = cons (flocons (values[i + 1].data.d_int32),
                                 return_val);
              break;

            case GIMP_PDB_INT16:
              return_val = cons (flocons (values[i + 1].data.d_int32),
                                 return_val);
              break;

            case GIMP_PDB_INT8:
              return_val = cons (flocons (values[i + 1].data.d_int32),
                                 return_val);
              break;

            case GIMP_PDB_FLOAT:
              return_val = cons (flocons (values[i + 1].data.d_float),
                                 return_val);
              break;

            case GIMP_PDB_STRING:
              string = values[i + 1].data.d_string;
              if (! string)
                string = "";
              string_len = strlen (string);
              return_val = cons (strcons (string_len, string), return_val);
              break;

            case GIMP_PDB_INT32ARRAY:
              {
                LISP array;
                gint j;

                array = arcons (tc_long_array, values[i].data.d_int32, 0);
                for (j = 0; j < values[i].data.d_int32; j++)
                  {
                    array->storage_as.long_array.data[j] =
                      values[i + 1].data.d_int32array[j];
                  }
                return_val = cons (array, return_val);
              }
              break;

            case GIMP_PDB_INT16ARRAY:
              {
                LISP array;
                gint j;

                array = arcons (tc_long_array, values[i].data.d_int32, 0);
                for (j = 0; j < values[i].data.d_int32; j++)
                  {
                    array->storage_as.long_array.data[j] =
                      values[i + 1].data.d_int16array[j];
                  }
                return_val = cons (array, return_val);
              }
              break;

            case GIMP_PDB_INT8ARRAY:
              {
                LISP array;
                gint j;

                array = arcons (tc_byte_array, values[i].data.d_int32, 0);
                for (j = 0; j < values[i].data.d_int32; j++)
                  {
                    array->storage_as.string.data[j] =
                      values[i + 1].data.d_int8array[j];
                  }
                return_val = cons (array, return_val);
              }
              break;

            case GIMP_PDB_FLOATARRAY:
              {
                LISP array;
                gint j;

                array = arcons (tc_double_array, values[i].data.d_int32, 0);
                for (j = 0; j < values[i].data.d_int32; j++)
                  {
                    array->storage_as.double_array.data[j] =
                      values[i + 1].data.d_floatarray[j];
                  }
                return_val = cons (array, return_val);
              }
              break;

            case GIMP_PDB_STRINGARRAY:
              {
                LISP array = NIL;
                gint j;

                for (j = 0; j < values[i].data.d_int32; j++)
                  {
                    string = (values[i + 1].data.d_stringarray)[j];

                    if (string)
                      {
                        string_len = strlen (string);
                        array = cons (strcons (string_len, string), array);
                      }
                    else
                      {
                        array = cons (strcons (0, ""), array);
                      }
                  }

                return_val = cons (nreverse (array), return_val);
              }
              break;

            case GIMP_PDB_COLOR:
              {
                guchar r, g, b;

                gimp_rgb_get_uchar (&values[i + 1].data.d_color, &r, &g, &b);

                intermediate_val = cons (flocons (r),
                                         cons (flocons (g),
                                               cons (flocons (b),
                                                     NIL)));
                return_val = cons (intermediate_val, return_val);
                break;
              }

            case GIMP_PDB_REGION:
              return my_err ("Regions are currently unsupported as return values", NIL);
              break;

            case GIMP_PDB_DISPLAY:
              return_val = cons (flocons (values[i + 1].data.d_int32),
                                 return_val);
              break;

            case GIMP_PDB_IMAGE:
              return_val = cons (flocons (values[i + 1].data.d_int32),
                                 return_val);
              break;

            case GIMP_PDB_LAYER:
              return_val = cons (flocons (values[i + 1].data.d_int32),
                                 return_val);
              break;

            case GIMP_PDB_CHANNEL:
              return_val = cons (flocons (values[i + 1].data.d_int32),
                                 return_val);
              break;

            case GIMP_PDB_DRAWABLE:
              return_val = cons (flocons (values[i + 1].data.d_int32),
                                 return_val);
              break;

            case GIMP_PDB_SELECTION:
              return_val = cons (flocons (values[i + 1].data.d_int32),
                                 return_val);
              break;

            case GIMP_PDB_BOUNDARY:
              return my_err ("Boundaries are currently unsupported as return values", NIL);
              break;

            case GIMP_PDB_PATH:
              return_val = cons (flocons (values[i + 1].data.d_int32),
                                 return_val);
              break;

            case GIMP_PDB_PARASITE:
              {
                LISP name, flags, data;

                if (values[i + 1].data.d_parasite.name == NULL)
                  {
                    return_val = my_err("Error: null parasite", NIL);
                  }
                else
                  {
                    string_len = strlen (values[i + 1].data.d_parasite.name);
                    name    = strcons (string_len,
                                       values[i + 1].data.d_parasite.name);

                    flags   = flocons (values[i + 1].data.d_parasite.flags);
                    data    = arcons (tc_byte_array,
                                      values[i + 1].data.d_parasite.size, 0);
                    memcpy(data->storage_as.string.data,
                           values[i + 1].data.d_parasite.data,
                           values[i + 1].data.d_parasite.size);

                    intermediate_val = cons (name,
                                             cons(flags, cons(data, NIL)));
                    return_val = cons (intermediate_val, return_val);
                  }
              }
              break;

            case GIMP_PDB_STATUS:
              return my_err ("Procedural database execution returned multiple status values", NIL);
              break;

            default:
              return my_err ("Unknown return type", NIL);
            }
        }
      break;

    case GIMP_PDB_PASS_THROUGH:
    case GIMP_PDB_CANCEL:   /*  should we do something here?  */
      break;
    }

  /*  free the proc name  */
  g_free (proc_name);

  /*  free up the executed procedure return values  */
  gimp_destroy_params (values, nvalues);

  /*  free up arguments and values  */
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
  if (script_fu_server_get_mode ())
    script_fu_server_listen (10);

#ifdef GDK_WINDOWING_WIN32
  /* This seems to help a lot on Windoze. */
  while (gtk_events_pending ())
    gtk_main_iteration ();
#endif

  return return_val;
}

static LISP
script_fu_register_call (LISP a)
{
  if (register_scripts)
    return script_fu_add_script (a);
  else
    return NIL;
}

static LISP
script_fu_menu_register_call (LISP a)
{
  if (register_scripts)
    return script_fu_add_menu (a);
  else
    return NIL;
}

static LISP
script_fu_quit_call (LISP a)
{
  script_fu_server_quit ();

  return NIL;
}
