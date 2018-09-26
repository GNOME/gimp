/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#if 0
#define DEBUG_MARSHALL 0  /* No need to define this until you need it */
#define DEBUG_SCRIPTS  0
#endif

#include "config.h"

#include <string.h>

#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include "libgimp/gimp.h"

#include "tinyscheme/scheme-private.h"
#if USE_DL
#include "tinyscheme/dynload.h"
#endif
#include "ftx/ftx.h"

#include "script-fu-types.h"

#include "script-fu-console.h"
#include "script-fu-interface.h"
#include "script-fu-regex.h"
#include "script-fu-scripts.h"
#include "script-fu-server.h"

#include "scheme-wrapper.h"


#undef cons

static void     ts_init_constants                           (scheme    *sc);
static void     ts_init_enum                                (scheme    *sc,
                                                             GType      enum_type);
static void     ts_init_procedures                          (scheme    *sc,
                                                             gboolean   register_scipts);
static void     convert_string                              (gchar     *str);
static pointer  script_fu_marshal_procedure_call            (scheme    *sc,
                                                             pointer    a,
                                                             gboolean   permissive);
static pointer  script_fu_marshal_procedure_call_strict     (scheme    *sc,
                                                             pointer    a);
static pointer  script_fu_marshal_procedure_call_permissive (scheme    *sc,
                                                             pointer    a);
static void     script_fu_marshal_destroy_args              (GimpParam *params,
                                                             gint       n_params);

static pointer  script_fu_register_call                     (scheme    *sc,
                                                             pointer    a);
static pointer  script_fu_menu_register_call                (scheme    *sc,
                                                             pointer    a);
static pointer  script_fu_quit_call                         (scheme    *sc,
                                                             pointer    a);
static pointer  script_fu_nil_call                          (scheme    *sc,
                                                             pointer    a);

static gboolean ts_load_file                                (const gchar *dirname,
                                                             const gchar *basename);

typedef struct
{
  const gchar *name;
  gint         value;
} NamedConstant;

static const NamedConstant script_constants[] =
{
  /* Useful values from libgimpbase/gimplimits.h */
  { "MIN-IMAGE-SIZE", GIMP_MIN_IMAGE_SIZE },
  { "MAX-IMAGE-SIZE", GIMP_MAX_IMAGE_SIZE },
  { "MIN-RESOLUTION", GIMP_MIN_RESOLUTION },
  { "MAX-RESOLUTION", GIMP_MAX_RESOLUTION },

  /* Useful misc stuff */
  { "TRUE",           TRUE  },
  { "FALSE",          FALSE },

  /* Builtin units */
  { "UNIT-PIXEL",     GIMP_UNIT_PIXEL },
  { "UNIT-INCH",      GIMP_UNIT_INCH  },
  { "UNIT-MM",        GIMP_UNIT_MM    },
  { "UNIT-POINT",     GIMP_UNIT_POINT },
  { "UNIT-PICA",      GIMP_UNIT_PICA  },

  /* Script-Fu types */
  { "SF-IMAGE",       SF_IMAGE      },
  { "SF-DRAWABLE",    SF_DRAWABLE   },
  { "SF-LAYER",       SF_LAYER      },
  { "SF-CHANNEL",     SF_CHANNEL    },
  { "SF-VECTORS",     SF_VECTORS    },
  { "SF-COLOR",       SF_COLOR      },
  { "SF-TOGGLE",      SF_TOGGLE     },
  { "SF-VALUE",       SF_VALUE      },
  { "SF-STRING",      SF_STRING     },
  { "SF-FILENAME",    SF_FILENAME   },
  { "SF-DIRNAME",     SF_DIRNAME    },
  { "SF-ADJUSTMENT",  SF_ADJUSTMENT },
  { "SF-FONT",        SF_FONT       },
  { "SF-PATTERN",     SF_PATTERN    },
  { "SF-BRUSH",       SF_BRUSH      },
  { "SF-GRADIENT",    SF_GRADIENT   },
  { "SF-OPTION",      SF_OPTION     },
  { "SF-PALETTE",     SF_PALETTE    },
  { "SF-TEXT",        SF_TEXT       },
  { "SF-ENUM",        SF_ENUM       },
  { "SF-DISPLAY",     SF_DISPLAY    },

  /* For SF-ADJUSTMENT */
  { "SF-SLIDER",      SF_SLIDER     },
  { "SF-SPINNER",     SF_SPINNER    },

  { NULL, 0 }
};


static scheme sc;


void
tinyscheme_init (GList    *path,
                 gboolean  register_scripts)
{
  /* init the interpreter */
  if (! scheme_init (&sc))
    {
      g_message ("Could not initialize TinyScheme!");
      return;
    }

  scheme_set_input_port_file (&sc, stdin);
  scheme_set_output_port_file (&sc, stdout);
  ts_register_output_func (ts_stdout_output_func, NULL);

  /* Initialize the TinyScheme extensions */
  init_ftx (&sc);
  script_fu_regex_init (&sc);

  /* register in the interpreter the gimp functions and types. */
  ts_init_constants (&sc);
  ts_init_procedures (&sc, register_scripts);

  if (path)
    {
      GList *list;

      for (list = path; list; list = g_list_next (list))
        {
          gchar *dir = g_file_get_path (list->data);

          if (ts_load_file (dir, "script-fu.init"))
            {
              /*  To improve compatibility with older Script-Fu scripts,
               *  load script-fu-compat.init from the same directory.
               */
              ts_load_file (dir, "script-fu-compat.init");

              /*  To improve compatibility with older GIMP version,
               *  load plug-in-compat.init from the same directory.
               */
              ts_load_file (dir, "plug-in-compat.init");

              g_free (dir);

              break;
            }

          g_free (dir);
        }

      if (list == NULL)
        g_printerr ("Unable to read initialization file script-fu.init\n");
    }
}

/* Create an SF-RUN-MODE constant for use in scripts.
 * It is set to the run mode state determined by GIMP.
 */
void
ts_set_run_mode (GimpRunMode run_mode)
{
  pointer symbol;

  symbol = sc.vptr->mk_symbol (&sc, "SF-RUN-MODE");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                          sc.vptr->mk_integer (&sc, run_mode));
  sc.vptr->setimmutable (symbol);
}

void
ts_set_print_flag (gint print_flag)
{
  sc.print_output = print_flag;
}

void
ts_print_welcome (void)
{
  ts_output_string (TS_OUTPUT_NORMAL,
                    "Welcome to TinyScheme, Version 1.40\n", -1);
  ts_output_string (TS_OUTPUT_NORMAL,
                    "Copyright (c) Dimitrios Souflis\n", -1);
}

void
ts_interpret_stdin (void)
{
  scheme_load_file (&sc, stdin);
}

gint
ts_interpret_string (const gchar *expr)
{
#if DEBUG_SCRIPTS
  sc.print_output = 1;
  sc.tracing = 1;
#endif

  sc.vptr->load_string (&sc, (char *) expr);

  return sc.retcode;
}

const gchar *
ts_get_success_msg (void)
{
  if (sc.vptr->is_string (sc.value))
    return sc.vptr->string_value (sc.value);

  return "Success";
}

void
ts_stdout_output_func (TsOutputType  type,
                       const char   *string,
                       int           len,
                       gpointer      user_data)
{
  if (len < 0)
    len = strlen (string);
  fprintf (stdout, "%.*s", len, string);
  fflush (stdout);
}

void
ts_gstring_output_func (TsOutputType  type,
                        const char   *string,
                        int           len,
                        gpointer      user_data)
{
  GString *gstr = (GString *) user_data;

  g_string_append_len (gstr, string, len);
}


/*  private functions  */

/*
 * Below can be found the functions responsible for registering the
 * gimp functions and types against the scheme interpreter.
 */
static void
ts_init_constants (scheme *sc)
{
  const gchar **enum_type_names;
  gint          n_enum_type_names;
  gint          i;
  pointer       symbol;
  GQuark        quark;

  symbol = sc->vptr->mk_symbol (sc, "gimp-directory");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_string (sc, gimp_directory ()));
  sc->vptr->setimmutable (symbol);

  symbol = sc->vptr->mk_symbol (sc, "gimp-data-directory");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_string (sc, gimp_data_directory ()));
  sc->vptr->setimmutable (symbol);

  symbol = sc->vptr->mk_symbol (sc, "gimp-plug-in-directory");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_string (sc, gimp_plug_in_directory ()));
  sc->vptr->setimmutable (symbol);

  symbol = sc->vptr->mk_symbol (sc, "gimp-locale-directory");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_string (sc, gimp_locale_directory ()));
  sc->vptr->setimmutable (symbol);

  symbol = sc->vptr->mk_symbol (sc, "gimp-sysconf-directory");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_string (sc, gimp_sysconf_directory ()));
  sc->vptr->setimmutable (symbol);

  enum_type_names = gimp_enums_get_type_names (&n_enum_type_names);
  quark           = g_quark_from_static_string ("gimp-compat-enum");

  for (i = 0; i < n_enum_type_names; i++)
    {
      const gchar *enum_name  = enum_type_names[i];
      GType        enum_type  = g_type_from_name (enum_name);

      ts_init_enum (sc, enum_type);

      enum_type = (GType) g_type_get_qdata (enum_type, quark);

      if (enum_type)
        ts_init_enum (sc, enum_type);
    }

  /* Constants used in the register block of scripts */
  for (i = 0; script_constants[i].name != NULL; ++i)
    {
      symbol = sc->vptr->mk_symbol (sc, script_constants[i].name);
      sc->vptr->scheme_define (sc, sc->global_env, symbol,
                               sc->vptr->mk_integer (sc,
                                                     script_constants[i].value));
      sc->vptr->setimmutable (symbol);
    }

  /* Define string constant for use in building paths to files/directories */
  symbol = sc->vptr->mk_symbol (sc, "DIR-SEPARATOR");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_string (sc, G_DIR_SEPARATOR_S));
  sc->vptr->setimmutable (symbol);

  /* Define string constant for use in building search paths */
  symbol = sc->vptr->mk_symbol (sc, "SEARCHPATH-SEPARATOR");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_string (sc, G_SEARCHPATH_SEPARATOR_S));
  sc->vptr->setimmutable (symbol);

  /* These constants are deprecated and will be removed at a later date. */
  symbol = sc->vptr->mk_symbol (sc, "gimp-dir");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_string (sc, gimp_directory ()));
  sc->vptr->setimmutable (symbol);

  symbol = sc->vptr->mk_symbol (sc, "gimp-data-dir");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_string (sc, gimp_data_directory ()));
  sc->vptr->setimmutable (symbol);

  symbol = sc->vptr->mk_symbol (sc, "gimp-plugin-dir");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_string (sc, gimp_plug_in_directory ()));
  sc->vptr->setimmutable (symbol);
}

static void
ts_init_enum (scheme *sc,
              GType   enum_type)
{
  GEnumClass  *enum_class = g_type_class_ref (enum_type);
  GEnumValue  *value;

  for (value = enum_class->values; value->value_name; value++)
    {
      if (g_str_has_prefix (value->value_name, "GIMP_"))
        {
          gchar   *scheme_name;
          pointer  symbol;

          scheme_name = g_strdup (value->value_name + strlen ("GIMP_"));
          convert_string (scheme_name);

          symbol = sc->vptr->mk_symbol (sc, scheme_name);
          sc->vptr->scheme_define (sc, sc->global_env, symbol,
                                   sc->vptr->mk_integer (sc, value->value));
          sc->vptr->setimmutable (symbol);

          g_free (scheme_name);
        }
    }

  g_type_class_unref (enum_class);
}

static void
ts_init_procedures (scheme   *sc,
                    gboolean  register_scripts)
{
  gchar   **proc_list;
  gint      num_procs;
  gint      i;
  pointer   symbol;

#if USE_DL
  symbol = sc->vptr->mk_symbol (sc,"load-extension");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_foreign_func (sc, scm_load_ext));
  sc->vptr->setimmutable (symbol);
#endif

  symbol = sc->vptr->mk_symbol (sc, "script-fu-register");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_foreign_func (sc,
                                                      register_scripts ?
                                                      script_fu_register_call :
                                                      script_fu_nil_call));
  sc->vptr->setimmutable (symbol);

  symbol = sc->vptr->mk_symbol (sc, "script-fu-menu-register");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_foreign_func (sc,
                                                      register_scripts ?
                                                      script_fu_menu_register_call :
                                                      script_fu_nil_call));
  sc->vptr->setimmutable (symbol);

  symbol = sc->vptr->mk_symbol (sc, "script-fu-quit");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_foreign_func (sc, script_fu_quit_call));
  sc->vptr->setimmutable (symbol);

  /*  register the database execution procedure  */
  symbol = sc->vptr->mk_symbol (sc, "gimp-proc-db-call");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_foreign_func (sc,
                                                      script_fu_marshal_procedure_call_strict));
  sc->vptr->setimmutable (symbol);

  /*  register the internal database execution procedure; see comment below  */
  symbol = sc->vptr->mk_symbol (sc, "-gimp-proc-db-call");
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_foreign_func (sc,
                                                      script_fu_marshal_procedure_call_permissive));
  sc->vptr->setimmutable (symbol);

  gimp_procedural_db_query (".*", ".*", ".*", ".*", ".*", ".*", ".*",
                            &num_procs, &proc_list);

  /*  Register each procedure as a scheme func  */
  for (i = 0; i < num_procs; i++)
    {
      gchar *buff;

      /* Build a define that will call the foreign function.
       * The Scheme statement was suggested by Simon Budig.
       *
       * We call the procedure through -gimp-proc-db-call, which is a more
       * permissive version of gimp-proc-db-call, that accepts (and ignores)
       * any number of arguments for nullary procedures, for backward
       * compatibility.
       */
      buff = g_strdup_printf (" (define (%s . args)"
                              " (apply -gimp-proc-db-call \"%s\" args))",
                              proc_list[i], proc_list[i]);

      /*  Execute the 'define'  */
      sc->vptr->load_string (sc, buff);

      g_free (buff);
    }

  g_strfreev (proc_list);
}

static gboolean
ts_load_file (const gchar *dirname,
              const gchar *basename)
{
  gchar *filename;
  FILE  *fin;

  filename = g_build_filename (dirname, basename, NULL);

  fin = g_fopen (filename, "rb");

  g_free (filename);

  if (fin)
    {
      scheme_load_file (&sc, fin);
      fclose (fin);

      return TRUE;
    }

  return FALSE;
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

/* This is called by the Scheme interpreter to allow calls to GIMP functions */
static pointer
script_fu_marshal_procedure_call (scheme   *sc,
                                  pointer   a,
                                  gboolean  permissive)
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
  gchar            error_str[1024];
  gint             i;
  gint             success = TRUE;
  pointer          return_val = sc->NIL;

#if DEBUG_MARSHALL
/* These three #defines are from Tinyscheme (tinyscheme/scheme.c) */
#define T_MASKTYPE  31
#define typeflag(p) ((p)->_flag)
#define type(p)     (typeflag(p)&T_MASKTYPE)

  static const char *ts_types[] =
  {
    "T_NONE",
    "T_STRING",    "T_NUMBER",     "T_SYMBOL",       "T_PROC",
    "T_PAIR",      "T_CLOSURE",    "T_CONTINUATION", "T_FOREIGN",
    "T_CHARACTER", "T_PORT",       "T_VECTOR",       "T_MACRO",
    "T_PROMISE",   "T_ENVIRONMENT","T_ARRAY"
  };

  g_printerr ("\nIn %s()\n", G_STRFUNC);
#endif

  /*  Make sure there are arguments  */
  if (a == sc->NIL)
    return foreign_error (sc,
                          "Procedure argument marshaller was called with no arguments. "
                          "The procedure to be executed and the arguments it requires "
                          "(possibly none) must be specified.", 0);

  /*  The PDB procedure name is the argument or first argument of the list  */
  if (sc->vptr->is_pair (a))
    proc_name = g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
  else
    proc_name = g_strdup (sc->vptr->string_value (a));

#ifdef DEBUG_MARSHALL
  g_printerr ("  proc name: %s\n", proc_name);
  g_printerr ("  parms rcvd: %d\n", sc->vptr->list_length (sc, a)-1);
#endif

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
#ifdef DEBUG_MARSHALL
      g_printerr ("  Invalid procedure name\n");
#endif
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid procedure name %s specified", proc_name);
      return foreign_error (sc, error_str, 0);
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
  if ((nparams > 0 || ! permissive) &&
      (sc->vptr->list_length (sc, a) - 1) != nparams)
    {
#if DEBUG_MARSHALL
      g_printerr ("  Invalid number of arguments (expected %d but received %d)",
                  nparams, (sc->vptr->list_length (sc, a) - 1));
#endif
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid number of arguments for %s (expected %d but received %d)",
                  proc_name, nparams, (sc->vptr->list_length (sc, a) - 1));
      return foreign_error (sc, error_str, 0);
    }

  /*  Marshall the supplied arguments  */
  if (nparams)
    args = g_new (GimpParam, nparams);
  else
    args = NULL;

  for (i = 0; i < nparams; i++)
    {
      gint32  n_elements;
      pointer vector;
      gint    j;

      a = sc->vptr->pair_cdr (a);

#if DEBUG_MARSHALL
      {
        const gchar *type_name;

        gimp_enum_get_value (GIMP_TYPE_PDB_ARG_TYPE,
                             params[i].type,
                             &type_name, NULL, NULL, NULL);

        g_printerr ("    param %d - expecting type %s (%d)\n",
                    i + 1, type_name, params[i].type);
        g_printerr ("      passed arg is type %s (%d)\n",
                    ts_types[ type(sc->vptr->pair_car (a)) ],
                    type(sc->vptr->pair_car (a)));
      }
#endif

      args[i].type = params[i].type;

      switch (params[i].type)
        {
        case GIMP_PDB_INT32:
        case GIMP_PDB_DISPLAY:
        case GIMP_PDB_IMAGE:
        case GIMP_PDB_ITEM:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_VECTORS:
          if (!sc->vptr->is_number (sc->vptr->pair_car (a)))
            success = FALSE;
          if (success)
            {
              args[i].data.d_int32 = sc->vptr->ivalue (sc->vptr->pair_car (a));
#if DEBUG_MARSHALL
              g_printerr ("      int32 arg is '%d'\n", args[i].data.d_int32);
#endif
            }
          break;

        case GIMP_PDB_INT16:
          if (!sc->vptr->is_number (sc->vptr->pair_car (a)))
            success = FALSE;
          if (success)
            {
              args[i].data.d_int16 = (gint16) sc->vptr->ivalue (sc->vptr->pair_car (a));
#if DEBUG_MARSHALL
              g_printerr ("      int16 arg is '%d'\n", args[i].data.d_int16);
#endif
            }
          break;

        case GIMP_PDB_INT8:
          if (!sc->vptr->is_number (sc->vptr->pair_car (a)))
            success = FALSE;
          if (success)
            {
              args[i].data.d_int8 = (guint8) sc->vptr->ivalue (sc->vptr->pair_car (a));
#if DEBUG_MARSHALL
              g_printerr ("      int8 arg is '%u'\n", args[i].data.d_int8);
#endif
            }
          break;

        case GIMP_PDB_FLOAT:
          if (!sc->vptr->is_number (sc->vptr->pair_car (a)))
            success = FALSE;
          if (success)
            {
              args[i].data.d_float = sc->vptr->rvalue (sc->vptr->pair_car (a));
#if DEBUG_MARSHALL
              g_printerr ("      float arg is '%f'\n", args[i].data.d_float);
#endif
            }
          break;

        case GIMP_PDB_STRING:
          if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
            success = FALSE;
          if (success)
            {
              args[i].data.d_string = sc->vptr->string_value (sc->vptr->pair_car (a));
#if DEBUG_MARSHALL
              g_printerr ("      string arg is '%s'\n", args[i].data.d_string);
#endif
            }
          break;

        case GIMP_PDB_INT32ARRAY:
          vector = sc->vptr->pair_car (a);
          if (!sc->vptr->is_vector (vector))
            success = FALSE;
          if (success)
            {
              n_elements = args[i-1].data.d_int32;
              if (n_elements < 0 ||
                  n_elements > sc->vptr->vector_length (vector))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "INT32 vector (argument %d) for function %s has "
                              "size of %ld but expected size of %d",
                              i+1, proc_name,
                              sc->vptr->vector_length (vector), n_elements);
                  return foreign_error (sc, error_str, 0);
                }

              args[i].data.d_int32array = g_new (gint32, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);

                  /* FIXME: Check values in vector stay within range for each type. */
                  if (!sc->vptr->is_number (v_element))
                    {
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not a number (argument %d for function %s)",
                                  j+1, i+1, proc_name);
                      return foreign_error (sc, error_str, vector);
                    }

                  args[i].data.d_int32array[j] =
                      (gint32) sc->vptr->ivalue (v_element);
                }

#if DEBUG_MARSHALL
              {
                glong count = sc->vptr->vector_length (vector);
                g_printerr ("      int32 vector has %ld elements\n", count);
                if (count > 0)
                  {
                    g_printerr ("     ");
                    for (j = 0; j < count; ++j)
                      g_printerr (" %ld",
                                  sc->vptr->ivalue ( sc->vptr->vector_elem (vector, j) ));
                    g_printerr ("\n");
                  }
              }
#endif
            }
          break;

        case GIMP_PDB_INT16ARRAY:
          vector = sc->vptr->pair_car (a);
          if (!sc->vptr->is_vector (vector))
            success = FALSE;
          if (success)
            {
              n_elements = args[i-1].data.d_int32;
              if (n_elements < 0 || n_elements > sc->vptr->vector_length (vector))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "INT16 vector (argument %d) for function %s has "
                              "size of %ld but expected size of %d",
                              i+1, proc_name, sc->vptr->vector_length (vector), n_elements);
                  return foreign_error (sc, error_str, 0);
                }

              args[i].data.d_int16array = g_new (gint16, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);

                  if (!sc->vptr->is_number (v_element))
                    {
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not a number (argument %d for function %s)",
                                  j+1, i+1, proc_name);
                      return foreign_error (sc, error_str, vector);
                    }

                  args[i].data.d_int16array[j] =
                      (gint16) sc->vptr->ivalue (v_element);
                }

#if DEBUG_MARSHALL
              {
                glong count = sc->vptr->vector_length (vector);
                g_printerr ("      int16 vector has %ld elements\n", count);
                if (count > 0)
                  {
                    g_printerr ("     ");
                    for (j = 0; j < count; ++j)
                      g_printerr (" %ld",
                                  sc->vptr->ivalue ( sc->vptr->vector_elem (vector, j) ));
                    g_printerr ("\n");
                  }
              }
#endif
            }
          break;

        case GIMP_PDB_INT8ARRAY:
          vector = sc->vptr->pair_car (a);
          if (!sc->vptr->is_vector (vector))
            success = FALSE;
          if (success)
            {
              n_elements = args[i-1].data.d_int32;
              if (n_elements < 0 ||
                  n_elements > sc->vptr->vector_length (vector))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "INT8 vector (argument %d) for function %s has "
                              "size of %ld but expected size of %d",
                              i+1, proc_name,
                              sc->vptr->vector_length (vector), n_elements);
                  return foreign_error (sc, error_str, 0);
                }

              args[i].data.d_int8array = g_new (guint8, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);

                  if (!sc->vptr->is_number (v_element))
                    {
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not a number (argument %d for function %s)",
                                  j+1, i+1, proc_name);
                      return foreign_error (sc, error_str, vector);
                    }

                  args[i].data.d_int8array[j] =
                      (guint8) sc->vptr->ivalue (v_element);
                }

#if DEBUG_MARSHALL
              {
                glong count = sc->vptr->vector_length (vector);
                g_printerr ("      int8 vector has %ld elements\n", count);
                if (count > 0)
                  {
                    g_printerr ("     ");
                    for (j = 0; j < count; ++j)
                      g_printerr (" %ld",
                                  sc->vptr->ivalue ( sc->vptr->vector_elem (vector, j) ));
                    g_printerr ("\n");
                  }
              }
#endif
            }
          break;

        case GIMP_PDB_FLOATARRAY:
          vector = sc->vptr->pair_car (a);
          if (!sc->vptr->is_vector (vector))
            success = FALSE;
          if (success)
            {
              n_elements = args[i-1].data.d_int32;
              if (n_elements < 0 ||
                  n_elements > sc->vptr->vector_length (vector))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "FLOAT vector (argument %d) for function %s has "
                              "size of %ld but expected size of %d",
                              i+1, proc_name,
                              sc->vptr->vector_length (vector), n_elements);
                  return foreign_error (sc, error_str, 0);
                }

              args[i].data.d_floatarray = g_new (gdouble, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);

                  if (!sc->vptr->is_number (v_element))
                    {
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not a number (argument %d for function %s)",
                                  j+1, i+1, proc_name);
                      return foreign_error (sc, error_str, vector);
                    }

                  args[i].data.d_floatarray[j] =
                      (gfloat) sc->vptr->rvalue (v_element);
                }

#if DEBUG_MARSHALL
              {
                glong count = sc->vptr->vector_length (vector);
                g_printerr ("      float vector has %ld elements\n", count);
                if (count > 0)
                  {
                    g_printerr ("     ");
                    for (j = 0; j < count; ++j)
                      g_printerr (" %f",
                                  sc->vptr->rvalue ( sc->vptr->vector_elem (vector, j) ));
                    g_printerr ("\n");
                  }
              }
#endif
            }
          break;

        case GIMP_PDB_STRINGARRAY:
          vector = sc->vptr->pair_car (a);  /* vector is pointing to a list */
          if (!sc->vptr->is_list (sc, vector))
            success = FALSE;
          if (success)
            {
              n_elements = args[i - 1].data.d_int32;
              if (n_elements < 0 ||
                  n_elements > sc->vptr->list_length (sc, vector))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "STRING vector (argument %d) for function %s has "
                              "length of %d but expected length of %d",
                              i+1, proc_name,
                              sc->vptr->list_length (sc, vector), n_elements);
                  return foreign_error (sc, error_str, 0);
                }

              args[i].data.d_stringarray = g_new (gchar *, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->pair_car (vector);

                  if (!sc->vptr->is_string (v_element))
                    {
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not a string (argument %d for function %s)",
                                  j+1, i+1, proc_name);
                      return foreign_error (sc, error_str, vector);
                    }

                  args[i].data.d_stringarray[j] =
                      (gchar *) sc->vptr->string_value (v_element);

                  vector = sc->vptr->pair_cdr (vector);
                }

#if DEBUG_MARSHALL
              {
                glong count = sc->vptr->list_length ( sc, sc->vptr->pair_car (a) );
                g_printerr ("      string vector has %ld elements\n", count);
                if (count > 0)
                  {
                    g_printerr ("     ");
                    for (j = 0; j < count; ++j)
                      g_printerr (" \"%s\"",
                                  args[i].data.d_stringarray[j]);
                    g_printerr ("\n");
                  }
              }
#endif
            }
          break;

        case GIMP_PDB_COLOR:
          if (sc->vptr->is_string (sc->vptr->pair_car (a)))
            {
              if (! gimp_rgb_parse_css (&args[i].data.d_color,
                                        sc->vptr->string_value (sc->vptr->pair_car (a)),
                                        -1))
                success = FALSE;

              gimp_rgb_set_alpha (&args[i].data.d_color, 1.0);
#if DEBUG_MARSHALL
              g_printerr ("      (%s)\n",
                          sc->vptr->string_value (sc->vptr->pair_car (a)));
#endif
            }
          else if (sc->vptr->is_list (sc, sc->vptr->pair_car (a)) &&
                   sc->vptr->list_length (sc, sc->vptr->pair_car (a)) == 3)
            {
              pointer color_list;
              guchar  r = 0, g = 0, b = 0;

              color_list = sc->vptr->pair_car (a);
              if (sc->vptr->is_number (sc->vptr->pair_car (color_list)))
                r = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)),
                           0, 255);
              else
                success = FALSE;

              color_list = sc->vptr->pair_cdr (color_list);
              if (sc->vptr->is_number (sc->vptr->pair_car (color_list)))
                g = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)),
                           0, 255);
              else
                success = FALSE;

              color_list = sc->vptr->pair_cdr (color_list);
              if (sc->vptr->is_number (sc->vptr->pair_car (color_list)))
                b = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)),
                           0, 255);
              else
                success = FALSE;

              if (success)
                gimp_rgba_set_uchar (&args[i].data.d_color, r, g, b, 255);
#if DEBUG_MARSHALL
              if (success)
                g_printerr ("      (%d %d %d)\n", r, g, b);
              else
                g_printerr ("      COLOR list contains non-numbers\n");
#endif
            }
          else
            {
              success = FALSE;
            }
          break;

        case GIMP_PDB_COLORARRAY:
          vector = sc->vptr->pair_car (a);
          if (!sc->vptr->is_vector (vector))
            success = FALSE;
          if (success)
            {
              n_elements = args[i-1].data.d_int32;
              if (n_elements < 0 ||
                  n_elements > sc->vptr->vector_length (vector))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "COLOR vector (argument %d) for function %s has "
                              "size of %ld but expected size of %d",
                              i+1, proc_name,
                              sc->vptr->vector_length (vector), n_elements);
                  return foreign_error (sc, error_str, 0);
                }

              args[i].data.d_colorarray = g_new (GimpRGB, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);
                  pointer color_list;
                  guchar  r, g, b;

                  if (! (sc->vptr->is_list (sc,
                                            sc->vptr->pair_car (v_element)) &&
                         sc->vptr->list_length (sc,
                                                sc->vptr->pair_car (v_element)) == 3))
                    {
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not a color "
                                  "(argument %d for function %s)",
                                  j+1, i+1, proc_name);
                      return foreign_error (sc, error_str, vector);
                    }

                  color_list = sc->vptr->pair_car (v_element);
                  r = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)),
                             0, 255);
                  color_list = sc->vptr->pair_cdr (color_list);
                  g = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)),
                             0, 255);
                  color_list = sc->vptr->pair_cdr (color_list);
                  b = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)),
                             0, 255);

                  gimp_rgba_set_uchar (&args[i].data.d_colorarray[j],
                                       r, g, b, 255);
                }
#if DEBUG_MARSHALL
              {
                glong count = sc->vptr->vector_length (vector);
                g_printerr ("      color vector has %ld elements\n", count);
              }
#endif
            }
          break;

        case GIMP_PDB_PARASITE:
          if (!sc->vptr->is_list (sc, sc->vptr->pair_car (a)) ||
              sc->vptr->list_length (sc, sc->vptr->pair_car (a)) != 3)
            success = FALSE;
          if (success)
            {
              pointer temp_val;

              /* parasite->name */
              temp_val = sc->vptr->pair_car (a);

              if (!sc->vptr->is_string (sc->vptr->pair_car (temp_val)))
                {
                  success = FALSE;
                  break;
                }

              args[i].data.d_parasite.name =
                sc->vptr->string_value (sc->vptr->pair_car (temp_val));
#if DEBUG_MARSHALL
              g_printerr ("      name '%s'\n", args[i].data.d_parasite.name);
#endif

              /* parasite->flags */
              temp_val = sc->vptr->pair_cdr (temp_val);

              if (!sc->vptr->is_number (sc->vptr->pair_car (temp_val)))
                {
                  success = FALSE;
                  break;
                }

              args[i].data.d_parasite.flags =
                sc->vptr->ivalue (sc->vptr->pair_car (temp_val));
#if DEBUG_MARSHALL
              g_printerr ("      flags %d", args[i].data.d_parasite.flags);
#endif

              /* parasite->data */
              temp_val = sc->vptr->pair_cdr (temp_val);

              if (!sc->vptr->is_string (sc->vptr->pair_car (temp_val)))
                {
                  success = FALSE;
                  break;
                }

              args[i].data.d_parasite.data =
                sc->vptr->string_value (sc->vptr->pair_car (temp_val));
              args[i].data.d_parasite.size = strlen (args[i].data.d_parasite.data);

#if DEBUG_MARSHALL
              g_printerr (", size %d\n", args[i].data.d_parasite.size);
              g_printerr ("      data '%s'\n", (char *)args[i].data.d_parasite.data);
#endif
            }
          break;

        case GIMP_PDB_STATUS:
          return foreign_error (sc,
                                "Status is for return types, not arguments",
                                sc->vptr->pair_car (a));
          break;

        default:
          g_snprintf (error_str, sizeof (error_str),
                      "Argument %d for %s is an unknown type",
                      i+1, proc_name);
          return foreign_error (sc, error_str, 0);
        }

      /* Break out of loop before i gets updated when error was detected */
      if (! success)
        break;
    }

  if (success)
    {
      /* refuse to refresh scripts from a script, better than crashing
       * see bug #575830
       */
      if (strcmp (proc_name, "script-fu-refresh-scripts"))
        {
#if DEBUG_MARSHALL
          g_printerr ("    calling %s...", proc_name);
#endif
          values = gimp_run_procedure2 (proc_name, &nvalues, nparams, args);
#if DEBUG_MARSHALL
          g_printerr ("  done.\n");
#endif
        }
    }
  else
    {
#if DEBUG_MARSHALL
      g_printerr ("  Invalid type for argument %d\n", i+1);
#endif
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid type for argument %d to %s",
                  i+1, proc_name);
      return foreign_error (sc, error_str, 0);
    }

  /*  Check the return status  */
  if (! values)
    {
#if DEBUG_MARSHALL
      g_printerr ("  Did not return status\n");
#endif
      g_snprintf (error_str, sizeof(error_str),
                  "Procedure execution of %s did not return a status",
                  proc_name);

      return foreign_error (sc, error_str, 0);
    }

#if DEBUG_MARSHALL
  {
    const gchar *status_name;

    gimp_enum_get_value (GIMP_TYPE_PDB_STATUS_TYPE,
                         values[0].data.d_status,
                         &status_name, NULL, NULL, NULL);
    g_printerr ("    return value is %s\n", status_name);
  }
#endif

  switch (values[0].data.d_status)
    {
    case GIMP_PDB_EXECUTION_ERROR:
      if (nvalues > 1 && values[1].type == GIMP_PDB_STRING)
        {
          g_snprintf (error_str, sizeof (error_str),
                      "Procedure execution of %s failed: %s",
                      proc_name, values[1].data.d_string);
        }
      else
        {
          g_snprintf (error_str, sizeof (error_str),
                      "Procedure execution of %s failed",
                      proc_name);
        }
      return foreign_error (sc, error_str, 0);
      break;

    case GIMP_PDB_CALLING_ERROR:
      if (nvalues > 1 && values[1].type == GIMP_PDB_STRING)
        {
          g_snprintf (error_str, sizeof (error_str),
                      "Procedure execution of %s failed on invalid input arguments: %s",
                      proc_name, values[1].data.d_string);
        }
      else
        {
          g_snprintf (error_str, sizeof (error_str),
                      "Procedure execution of %s failed on invalid input arguments",
                      proc_name);
        }
      return foreign_error (sc, error_str, 0);
      break;

    case GIMP_PDB_SUCCESS:
#if DEBUG_MARSHALL
      g_printerr ("    values returned: %d\n", nvalues-1);
#endif
      for (i = nvalues - 2; i >= 0; --i)
        {
          const gchar *string;
          gint         j;

#if DEBUG_MARSHALL
          {
            const gchar *type_name;

            gimp_enum_get_value (GIMP_TYPE_PDB_ARG_TYPE,
                                 return_vals[i].type,
                                 &type_name, NULL, NULL, NULL);

            g_printerr ("      value %d is type %s (%d)\n",
                        i, type_name, return_vals[i].type);
          }
#endif
          switch (return_vals[i].type)
            {
            case GIMP_PDB_INT32:
            case GIMP_PDB_DISPLAY:
            case GIMP_PDB_IMAGE:
            case GIMP_PDB_ITEM:
            case GIMP_PDB_LAYER:
            case GIMP_PDB_CHANNEL:
            case GIMP_PDB_DRAWABLE:
            case GIMP_PDB_SELECTION:
            case GIMP_PDB_VECTORS:
              return_val = sc->vptr->cons (sc,
                             sc->vptr->mk_integer (sc,
                                                   values[i + 1].data.d_int32),
                             return_val);
              break;

            case GIMP_PDB_INT16:
              return_val = sc->vptr->cons (sc,
                             sc->vptr->mk_integer (sc,
                                                   values[i + 1].data.d_int16),
                             return_val);
              break;

            case GIMP_PDB_INT8:
              return_val = sc->vptr->cons (sc,
                             sc->vptr->mk_integer (sc,
                                                   values[i + 1].data.d_int8),
                             return_val);
              break;

            case GIMP_PDB_FLOAT:
              return_val = sc->vptr->cons (sc,
                             sc->vptr->mk_real (sc,
                                                values[i + 1].data.d_float),
                             return_val);
              break;

            case GIMP_PDB_STRING:
              string = values[i + 1].data.d_string;
              if (! string)
                string = "";
              return_val = sc->vptr->cons (sc,
                             sc->vptr->mk_string (sc, string),
                             return_val);
              break;

            case GIMP_PDB_INT32ARRAY:
              {
                gint32  num_int32s = values[i].data.d_int32;
                gint32 *array      = (gint32 *) values[i + 1].data.d_int32array;
                pointer vector     = sc->vptr->mk_vector (sc, num_int32s);

                for (j = 0; j < num_int32s; j++)
                  {
                    sc->vptr->set_vector_elem (vector, j,
                                               sc->vptr->mk_integer (sc,
                                                                     array[j]));
                  }

                return_val = sc->vptr->cons (sc, vector, return_val);
              }
              break;

            case GIMP_PDB_INT16ARRAY:
              {
                gint32  num_int16s = values[i].data.d_int32;
                gint16 *array      = (gint16 *) values[i + 1].data.d_int16array;
                pointer vector     = sc->vptr->mk_vector (sc, num_int16s);

                for (j = 0; j < num_int16s; j++)
                  {
                    sc->vptr->set_vector_elem (vector, j,
                                               sc->vptr->mk_integer (sc,
                                                                     array[j]));
                  }

                return_val = sc->vptr->cons (sc, vector, return_val);
              }
              break;

            case GIMP_PDB_INT8ARRAY:
              {
                gint32  num_int8s = values[i].data.d_int32;
                guint8 *array     = (guint8 *) values[i + 1].data.d_int8array;
                pointer vector    = sc->vptr->mk_vector (sc, num_int8s);

                for (j = 0; j < num_int8s; j++)
                  {
                    sc->vptr->set_vector_elem (vector, j,
                                               sc->vptr->mk_integer (sc,
                                                                     array[j]));
                  }

                return_val = sc->vptr->cons (sc, vector, return_val);
              }
              break;

            case GIMP_PDB_FLOATARRAY:
              {
                gint32   num_floats = values[i].data.d_int32;
                gdouble *array      = (gdouble *) values[i + 1].data.d_floatarray;
                pointer  vector     = sc->vptr->mk_vector (sc, num_floats);

                for (j = 0; j < num_floats; j++)
                  {
                    sc->vptr->set_vector_elem (vector, j,
                                               sc->vptr->mk_real (sc,
                                                                  array[j]));
                  }

                return_val = sc->vptr->cons (sc, vector, return_val);
              }
              break;

            case GIMP_PDB_STRINGARRAY:
              {
                gint    num_strings = values[i].data.d_int32;
                gchar **array       = (gchar **) values[i + 1].data.d_stringarray;
                pointer list        = sc->NIL;

                for (j = num_strings - 1; j >= 0; j--)
                  {
                    list = sc->vptr->cons (sc,
                                           sc->vptr->mk_string (sc,
                                                                array[j] ?
                                                                array[j] : ""),
                                           list);
                  }

                return_val = sc->vptr->cons (sc, list, return_val);
              }
              break;

            case GIMP_PDB_COLOR:
              {
                guchar   r, g, b;
                gpointer temp_val;

                gimp_rgb_get_uchar (&values[i + 1].data.d_color, &r, &g, &b);

                temp_val = sc->vptr->cons (sc,
                             sc->vptr->mk_integer (sc, r),
                             sc->vptr->cons (sc,
                               sc->vptr->mk_integer (sc, g),
                               sc->vptr->cons (sc,
                                 sc->vptr->mk_integer (sc, b),
                                 sc->NIL)));
                return_val = sc->vptr->cons (sc,
                                             temp_val,
                                             return_val);
                break;
              }

            case GIMP_PDB_COLORARRAY:
              {
                gint32   num_colors = values[i].data.d_int32;
                GimpRGB *array      = (GimpRGB *) values[i + 1].data.d_colorarray;
                pointer  vector     = sc->vptr->mk_vector (sc, num_colors);

                for (j = 0; j < num_colors; j++)
                  {
                    guchar  r, g, b;
                    pointer temp_val;

                    gimp_rgb_get_uchar (&array[j], &r, &g, &b);

                    temp_val = sc->vptr->cons (sc,
                                 sc->vptr->mk_integer (sc, r),
                                 sc->vptr->cons (sc,
                                   sc->vptr->mk_integer (sc, g),
                                   sc->vptr->cons (sc,
                                     sc->vptr->mk_integer (sc, b),
                                     sc->NIL)));
                    sc->vptr->set_vector_elem (vector, j, temp_val);
                  }

                return_val = sc->vptr->cons (sc, vector, return_val);
              }
              break;

            case GIMP_PDB_PARASITE:
              {
                if (values[i + 1].data.d_parasite.name == NULL)
                  {
                    return_val = foreign_error (sc, "Error: null parasite", 0);
                  }
                else
                  {
                    GimpParasite *p = &values[i + 1].data.d_parasite;
                    gchar        *data = g_strndup (p->data, p->size);
                    gint          char_cnt = g_utf8_strlen (data, p->size);
                    pointer       temp_val;

                    /* don't move the mk_foo() calls outside this function call,
                     * otherwise they might be garbage collected away!
                     */
                    temp_val = sc->vptr->cons (sc,
                                 sc->vptr->mk_string (sc, p->name),
                                 sc->vptr->cons (sc,
                                   sc->vptr->mk_integer (sc, p->flags),
                                   sc->vptr->cons (sc,
                                     sc->vptr->mk_counted_string (sc,
                                                                  data,
                                                                  char_cnt),
                                     sc->NIL)));
                    return_val = sc->vptr->cons (sc,
                                                 temp_val,
                                                 return_val);
                    g_free (data);

#if DEBUG_MARSHALL
                    g_printerr ("      name '%s'\n", p->name);
                    g_printerr ("      flags %d", p->flags);
                    g_printerr (", size %d\n", p->size);
                    g_printerr ("      data '%.*s'\n",
                                p->size, (gchar *) p->data);
#endif
                  }
              }
              break;

            case GIMP_PDB_STATUS:
              return foreign_error (sc, "Procedure execution returned multiple status values", 0);
              break;

            default:
              return foreign_error (sc, "Unknown return type", 0);
            }
        }

    case GIMP_PDB_PASS_THROUGH:
    case GIMP_PDB_CANCEL:   /*  should we do something here?  */
      break;
    }

  /* If we have no return value(s) from PDB call, return
   * either TRUE or FALSE to indicate if call succeeded.
   */
  if (return_val == sc->NIL)
    {
      if (values[0].data.d_status == GIMP_PDB_SUCCESS)
        return_val = sc->vptr->cons (sc, sc->T, sc->NIL);
      else
        return_val = sc->vptr->cons (sc, sc->F, sc->NIL);
    }

  /*  free the proc name  */
  g_free (proc_name);

  /*  free up the executed procedure return values  */
  gimp_destroy_params (values, nvalues);

  /*  free up arguments and values  */
  script_fu_marshal_destroy_args (args, nparams);

  /*  free the query information  */
  g_free (proc_blurb);
  g_free (proc_help);
  g_free (proc_author);
  g_free (proc_copyright);
  g_free (proc_date);
  g_free (params);
  g_free (return_vals);

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

static pointer
script_fu_marshal_procedure_call_strict (scheme  *sc,
                                         pointer  a)
{
  return script_fu_marshal_procedure_call (sc, a, FALSE);
}

static pointer
script_fu_marshal_procedure_call_permissive (scheme  *sc,
                                             pointer  a)
{
  return script_fu_marshal_procedure_call (sc, a, TRUE);
}

static void
script_fu_marshal_destroy_args (GimpParam *params,
                                gint       n_params)
{
  gint i;

  for (i = 0; i < n_params; i++)
    {
      switch (params[i].type)
        {
        case GIMP_PDB_INT32:
        case GIMP_PDB_INT16:
        case GIMP_PDB_INT8:
        case GIMP_PDB_FLOAT:
        case GIMP_PDB_STRING:
          break;

        case GIMP_PDB_INT32ARRAY:
          g_free (params[i].data.d_int32array);
          break;

        case GIMP_PDB_INT16ARRAY:
          g_free (params[i].data.d_int16array);
          break;

        case GIMP_PDB_INT8ARRAY:
          g_free (params[i].data.d_int8array);
          break;

        case GIMP_PDB_FLOATARRAY:
          g_free (params[i].data.d_floatarray);
          break;

        case GIMP_PDB_STRINGARRAY:
          g_free (params[i].data.d_stringarray);
          break;

        case GIMP_PDB_COLORARRAY:
          g_free (params[i].data.d_colorarray);
          break;

        case GIMP_PDB_COLOR:
        case GIMP_PDB_DISPLAY:
        case GIMP_PDB_IMAGE:
        case GIMP_PDB_ITEM:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_VECTORS:
        case GIMP_PDB_PARASITE:
        case GIMP_PDB_STATUS:
        case GIMP_PDB_END:
          break;
        }
    }

  g_free (params);
}

static pointer
script_fu_register_call (scheme  *sc,
                         pointer  a)
{
  return script_fu_add_script (sc, a);
}

static pointer
script_fu_menu_register_call (scheme  *sc,
                              pointer  a)
{
  return script_fu_add_menu (sc, a);
}

static pointer
script_fu_quit_call (scheme  *sc,
                     pointer  a)
{
  script_fu_server_quit ();

  scheme_deinit (sc);

  return sc->NIL;
}

static pointer
script_fu_nil_call (scheme  *sc,
                    pointer  a)
{
  return sc->NIL;
}
