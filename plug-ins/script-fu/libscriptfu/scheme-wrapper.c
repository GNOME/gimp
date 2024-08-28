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

#define DEBUG_SCRIPTS  0


#include "config.h"

#include <string.h>

#include <glib/gstdio.h>

#include <girepository.h>

#include <gtk/gtk.h>

#include "libgimp/gimp.h"

#include "tinyscheme/scheme-private.h"
#if USE_DL
#include "tinyscheme/dynload.h"
#endif
#include "ftx/ftx.h"

#include "script-fu-types.h"

#include "script-fu-regex.h"
#include "script-fu-scripts.h"
#include "script-fu-errors.h"
#include "script-fu-compat.h"
#include "script-fu-version.h"
#include "script-fu-progress.h"

#include "scheme-wrapper.h"
#include "scheme-marshal.h"
#include "scheme-marshal-return.h"


#undef cons

static void     ts_init_constants                           (scheme       *sc,
                                                             GIRepository *repo);
static void     ts_init_enums                               (scheme       *sc,
                                                             GIRepository *repo,
                                                             const char   *namespace);
static void     ts_define_procedure                         (scheme       *sc,
                                                             const gchar  *symbol_name,
                                                             TsWrapperFunc func);
static void     ts_init_procedures                          (scheme    *sc,
                                                             gboolean   register_scipts);
static pointer  script_fu_marshal_procedure_call            (scheme    *sc,
                                                             pointer    a,
                                                             gboolean   permissive,
                                                             gboolean   deprecated);
static pointer  script_fu_marshal_procedure_call_strict     (scheme    *sc,
                                                             pointer    a);
static pointer  script_fu_marshal_procedure_call_permissive (scheme    *sc,
                                                             pointer    a);
static pointer  script_fu_marshal_procedure_call_deprecated (scheme    *sc,
                                                             pointer    a);

static pointer  script_fu_register_call                     (scheme    *sc,
                                                             pointer    a);
static pointer  script_fu_register_call_filter              (scheme    *sc,
                                                             pointer    a);
static pointer  script_fu_menu_register_call                (scheme    *sc,
                                                             pointer    a);
static pointer  script_fu_use_v3_call                       (scheme    *sc,
                                                             pointer    a);
static pointer  script_fu_use_v2_call                       (scheme    *sc,
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

/* LHS is text in a script, RHS is constant defined in C. */
static const NamedConstant script_constants[] =
{
  /* Useful values from libgimpbase/gimplimits.h */
  { "MIN-IMAGE-SIZE", GIMP_MIN_IMAGE_SIZE },
  { "MAX-IMAGE-SIZE", GIMP_MAX_IMAGE_SIZE },
  /* Note conversion to int, loss of precision: 0.005=>0 */
  { "MIN-RESOLUTION", (gint) GIMP_MIN_RESOLUTION },
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

  /* Arg types. */
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

  /* PDB procedure drawable_arity, i.e. sensitivity.
   * Used with script-fu-register-filter.
   *
   * This declares the arity of the algorithm,
   * and not the signature of the PDB procedure.
   * Since v3, PDB procedures that are image procedures,
   * take a container of drawables.
   * This only describes how many drawables the container *should* hold.
   *
   * For calls invoked by a user, this describes
   * how many drawables the user is expected to select,
   * which disables/enables the menu item for the procedure.
   *
   * Procedures are also called from other procedures.
   * A call from another procedure may in fact
   * pass more drawables than declared for drawable_arity.
   * That is a programming error on behalf of the caller.
   * A well-written callee that is passed more drawables than declared
   * should return an error instead of processing any of the drawables.
   *
   * Similarly for fewer than declared.
   */

  /* Requires two drawables. Often an operation between them, yielding a new drawable */
  { "SF-TWO-OR-MORE-DRAWABLE",   SF_TWO_OR_MORE_DRAWABLE  },
  /* Often processed independently, sequentially, with side effects on the drawables. */
  { "SF-ONE-OR-MORE-DRAWABLE",   SF_ONE_OR_MORE_DRAWABLE  },
  /* Requires exactly one drawable. */
  { "SF-ONE-DRAWABLE",     SF_ONE_DRAWABLE    },

  /* For SF-ADJUSTMENT */
  { "SF-SLIDER",      SF_SLIDER     },
  { "SF-SPINNER",     SF_SPINNER    },

  { NULL, 0 }
};


static scheme sc;

/*
 * These callbacks break the backwards compile-time dependence
 * of inner scheme-wrapper on the outer script-fu-server.
 * Only script-fu-server registers, when it runs.
 */
static TsCallbackFunc post_command_callback = NULL;
static TsCallbackFunc quit_callback         = NULL;


void
tinyscheme_init (GList    *path,
                 gboolean  register_scripts)
{
  GIRepository *repo;
  GITypelib    *typelib;
  GError       *error = NULL;

  /* init the interpreter */
  if (! scheme_init (&sc))
    {
      g_warning ("Could not initialize TinyScheme!");
      return;
    }

  scheme_set_input_port_file (&sc, stdin);
  scheme_set_output_port_file (&sc, stdout);
  ts_register_output_func (ts_stdout_output_func, NULL);

  /* Initialize the TinyScheme extensions */
  init_ftx (&sc);
  script_fu_regex_init (&sc);

  /* Fetch the typelib */
  repo = g_irepository_get_default ();
  typelib = g_irepository_require (repo, "Gimp", NULL, 0, &error);
  if (!typelib)
    {
      g_warning ("%s", error->message);
      g_clear_error (&error);
      return;
    }

  /* register in the interpreter the gimp functions and types. */
  ts_init_constants (&sc, repo);
  ts_init_procedures (&sc, register_scripts);

  if (path)
    {
      GList *list;

      g_debug ("Loading init and compat scripts.");

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
        g_warning ("Unable to read initialization file script-fu.init\n");
    }
  else
    g_warning ("Not loading initialization or compatibility scripts.");
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
  gint result;

#if DEBUG_SCRIPTS
  sc.print_output = 1;
  sc.tracing = 1;
#endif

  sc.vptr->load_string (&sc, (char *) expr);

  result = sc.retcode;

  g_debug ("ts_interpret_string returns: %i", result);
  return result;
}

const gchar *
ts_get_success_msg (void)
{
  if (sc.vptr->is_string (sc.value))
    return sc.vptr->string_value (sc.value);

  return "Success";
}

/* Delegate. The caller doesn't know the scheme instance,
 * and here we don't know TS internals.
 */
const gchar*
ts_get_error_msg (void)
{
  return ts_get_error_string (&sc);
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

void
ts_register_quit_callback (TsCallbackFunc callback)
{
  quit_callback = callback;
}

void
ts_register_post_command_callback (TsCallbackFunc callback)
{
  post_command_callback = callback;
}


/*  private functions  */

/*
 * Below can be found the functions responsible for registering the
 * gimp functions and types against the scheme interpreter.
 */
static void
ts_init_constants (scheme       *sc,
                   GIRepository *repo)
{
  int     i;
  pointer symbol;

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

  ts_init_enums (sc, repo, "Gimp");
  ts_init_enums (sc, repo, "Gegl");

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
ts_init_enum (scheme     *sc,
              GIEnumInfo *info,
              const char *namespace)
{
  int n_values;

  n_values = g_enum_info_get_n_values ((GIEnumInfo *) info);
  for (int j = 0; j < n_values; j++)
    {
      GIValueInfo *value_info;
      const char  *c_identifier;
      char        *scheme_name;
      int         int_value;
      pointer     symbol;

      value_info = g_enum_info_get_value (info, j);

      /* Get name & value. Normally, we would use the actual name of the
       * GIBaseInfo here, but that would break bw-compatibility */
      c_identifier = g_base_info_get_attribute ((GIBaseInfo *) value_info, "c:identifier");
      if (c_identifier == NULL)
        {
          g_warning ("Problem in the GIR file: enum value without \"c:identifier\"!");
          g_base_info_unref ((GIBaseInfo *) value_info);
          continue;
        }

      /* Scheme-ify the name */
      if (g_strcmp0 (namespace, "Gimp") == 0)
        {
          /* Skip the GIMP prefix for GIMP enums */
          if (g_str_has_prefix (c_identifier, "GIMP_"))
            c_identifier += strlen ("GIMP_");
        }
      else
        {
          /* Other namespaces: skip non-prefixed symbols, to prevent clashes */
          if (g_ascii_strncasecmp (c_identifier, namespace, strlen (namespace)) != 0)
            {
              g_base_info_unref ((GIBaseInfo *) value_info);
              continue;
            }
        }
      scheme_name = g_strdelimit (g_strdup (c_identifier), "_", '-');
      int_value = g_value_info_get_value (value_info);

      /* Register the symbol */
      symbol = sc->vptr->mk_symbol (sc, scheme_name);
      sc->vptr->scheme_define (sc, sc->global_env, symbol,
                               sc->vptr->mk_integer (sc, int_value));
      sc->vptr->setimmutable (symbol);

      g_free (scheme_name);
      g_base_info_unref ((GIBaseInfo *) value_info);
    }
}

static void
ts_init_enums (scheme       *sc,
               GIRepository *repo,
               const char   *namespace)
{
  int i, n_infos;

  n_infos = g_irepository_get_n_infos (repo, namespace);
  for (i = 0; i < n_infos; i++)
    {
      GIBaseInfo *info;

      info = g_irepository_get_info (repo, namespace, i);
      if (GI_IS_ENUM_INFO (info))
        {
          ts_init_enum (sc, (GIEnumInfo *) info, namespace);
        }

      g_base_info_unref (info);
    }
}

/* Define a symbol into interpreter state,
 * bound to a foreign function, language C, defined here in ScriptFu source.
 */
static void
ts_define_procedure (scheme       *sc,
                     const gchar  *symbol_name,
                     TsWrapperFunc func)
{
  pointer   symbol;

  symbol = sc->vptr->mk_symbol (sc, symbol_name);
  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_foreign_func (sc, func));
  sc->vptr->setimmutable (symbol);
}


/* Define, into interpreter state,
 * 1) Scheme functions that call wrapper functions in C here in ScriptFu.
 * 2) Scheme functions wrapping every procedure in the PDB.
 */
static void
ts_init_procedures (scheme   *sc,
                    gboolean  register_scripts)
{
  gchar   **proc_list;
  gint      num_procs;
  gint      i;

#if USE_DL
/* scm_load_ext not same as other wrappers, defined in tinyscheme/dynload */
ts_define_procedure (sc, "load-extension", scm_load_ext);
#endif

  /* Define special functions used in scripts. */
  if (register_scripts)
    {
      ts_define_procedure (sc, "script-fu-register",        script_fu_register_call);
      ts_define_procedure (sc, "script-fu-register-filter", script_fu_register_call_filter);
      ts_define_procedure (sc, "script-fu-menu-register",   script_fu_menu_register_call);
    }
  else
    {
      ts_define_procedure (sc, "script-fu-register",        script_fu_nil_call);
      ts_define_procedure (sc, "script-fu-register-filter", script_fu_nil_call);
      ts_define_procedure (sc, "script-fu-menu-register",   script_fu_nil_call);
    }

  ts_define_procedure (sc, "script-fu-use-v3",    script_fu_use_v3_call);
  ts_define_procedure (sc, "script-fu-use-v2",    script_fu_use_v2_call);
  ts_define_procedure (sc, "script-fu-quit",      script_fu_quit_call);

  /* Define wrapper functions, not used in scripts.
   * FUTURE: eliminate all but one, deprecated and permissive is obsolete.
   */
  ts_define_procedure (sc, "gimp-proc-db-call",   script_fu_marshal_procedure_call_strict);
  ts_define_procedure (sc, "-gimp-proc-db-call",  script_fu_marshal_procedure_call_permissive);
  ts_define_procedure (sc, "--gimp-proc-db-call", script_fu_marshal_procedure_call_deprecated);

  /* Define each PDB procedure as a scheme func.
   * Each call passes through one of the wrapper funcs.
   */
  proc_list = gimp_pdb_query_procedures (gimp_get_pdb (),
                                         ".*", ".*", ".*", ".*",
                                         ".*", ".*", ".*", ".*");
  num_procs = proc_list ? g_strv_length (proc_list) : 0;

  for (i = 0; i < num_procs; i++)
    {
      gchar *buff;

      /* Build a define that will call the foreign function.
       * The Scheme statement was suggested by Simon Budig.
       *
       * Call the procedure through -gimp-proc-db-call, which is a more
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

  /* Register more scheme funcs that call PDB procedures, for compatibility
   * This can overwrite earlier scheme func definitions.
   */
  define_compat_procs (sc);
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

/* Called by the Scheme interpreter on calls to GIMP PDB procedures */
static pointer
script_fu_marshal_procedure_call (scheme   *sc,
                                  pointer   a,
                                  gboolean  permissive,
                                  gboolean  deprecated)
{
  GimpProcedure        *procedure;
  GimpProcedureConfig  *config;
  GimpValueArray       *values = NULL;
  gchar                *proc_name;
  GParamSpec          **arg_specs;
  gint                  n_arg_specs;
  gint                  actual_arg_count;
  gint                  consumed_arg_count = 0;
  gchar                 error_str[1024];
  gint                  i;
  pointer               return_val = sc->NIL;

  g_debug ("In %s()", G_STRFUNC);

  if (a == sc->NIL)
    /* Some ScriptFu function is calling this incorrectly. */
    return implementation_error (sc,
                                 "Procedure argument marshaller was called with no arguments. "
                                 "The procedure to be executed and the arguments it requires "
                                 "(possibly none) must be specified.",
                                 0);

  /*  The PDB procedure name is the argument or first argument of the list  */
  if (sc->vptr->is_pair (a))
    proc_name = g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
  else
    proc_name = g_strdup (sc->vptr->string_value (a));

  g_debug ("proc name: %s", proc_name);
  g_debug ("parms rcvd: %d", sc->vptr->list_length (sc, a)-1);

  if (deprecated )
    g_warning ("PDB procedure name %s is deprecated, please use %s.",
               deprecated_name_for (proc_name),
               proc_name);

  script_fu_progress_report (proc_name);

  /*  Attempt to fetch the procedure from the database  */
  procedure = gimp_pdb_lookup_procedure (gimp_get_pdb (), proc_name);

  if (! procedure)
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid procedure name: %s", proc_name);
      return script_error (sc, error_str, 0);
    }

  config    = gimp_procedure_create_config (procedure);
  arg_specs = gimp_procedure_get_arguments (procedure, &n_arg_specs);
  actual_arg_count = sc->vptr->list_length (sc, a) - 1;

  /* Check the supplied number of arguments.
   * This only gives warnings to the console.
   * It does not ensure that the count of supplied args equals the count of formal args.
   * Subsequent code must not assume that.
   *
   * When too few supplied args, when permissive, scriptfu or downstream machinery
   * can try to provide missing args e.g. defaults.
   *
   * Extra supplied args can be discarded.
   * Formerly, this was a deprecated behavior depending on "permissive".
   */
  {
    if (actual_arg_count > n_arg_specs)
      {
        /* Warn, but permit extra args. Will discard args from script.*/
        g_warning ("in script, permitting too many args to %s", proc_name);
      }
    else if (actual_arg_count < n_arg_specs)
      {
        /* Warn, but permit too few args.
         * Scriptfu or downstream might provide missing args.
         * It is author friendly to continue to parse the script for type errors.
         */
        g_warning ("in script, permitting too few args to %s", proc_name);
      }
    /* else equal counts of args. */
  }

  /*  Marshall the supplied arguments  */
  for (i = 0; i < n_arg_specs; i++)
    {
      GParamSpec *arg_spec = arg_specs[i];
      GValue      value    = G_VALUE_INIT;
      guint       n_elements; /* !!! unsigned length */
      pointer     vector;   /* !!! list or vector */
      gint        j;

      consumed_arg_count++;

      if (consumed_arg_count > actual_arg_count)
        {
          /* Exhausted supplied arguments before formal specs. */

          /* Say formal type of first missing arg. */
          g_warning ("Missing arg type: %s", g_type_name (G_PARAM_SPEC_VALUE_TYPE (arg_spec)));

          /* Break loop over formal specs. Continuation is to call PDB with partial args. */
          break;
        }
      else
        a = sc->vptr->pair_cdr (a);  /* advance pointer to next arg in list. */

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (arg_spec));

      debug_in_arg (sc, a, i, g_type_name (G_VALUE_TYPE (&value)));

      if (G_VALUE_HOLDS_INT (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            {
              return script_type_error (sc, "numeric", i, proc_name);
            }
          else
            {
              GParamSpecInt *ispec = G_PARAM_SPEC_INT (arg_spec);
              gint           v     = sc->vptr->ivalue (sc->vptr->pair_car (a));

              if (v < ispec->minimum || v > ispec->maximum)
                {
                  gchar error_message[1024];

                  g_snprintf (error_message, sizeof (error_message),
                              "Invalid value %d for argument %d: expected value between %d and %d",
                              v, i, ispec->minimum, ispec->maximum);
                  return script_error (sc, error_message, 0);
                }

              g_value_set_int (&value, v);
            }
        }
      else if (G_VALUE_HOLDS_UINT (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            {
              return script_type_error (sc, "numeric", i, proc_name);
            }
          else
            {
              GParamSpecUInt *ispec = G_PARAM_SPEC_UINT (arg_spec);
              gint            v     = sc->vptr->ivalue (sc->vptr->pair_car (a));

              if (v < ispec->minimum || v > ispec->maximum)
                {
                  gchar error_message[1024];

                  g_snprintf (error_message, sizeof (error_message),
                              "Invalid value %d for argument %d: expected value between %d and %d",
                              v, i, ispec->minimum, ispec->maximum);
                  return script_error (sc, error_message, 0);
                }

              g_value_set_uint (&value, v);
            }
        }
      else if (G_VALUE_HOLDS_UCHAR (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            {
              return script_type_error (sc, "numeric", i, proc_name);
            }
          else
            {
              GParamSpecUChar *cspec = G_PARAM_SPEC_UCHAR (arg_spec);
              gint             c     = sc->vptr->ivalue (sc->vptr->pair_car (a));

              if (c < cspec->minimum || c > cspec->maximum)
                {
                  gchar error_message[1024];

                  g_snprintf (error_message, sizeof (error_message),
                              "Invalid value %d for argument %d: expected value between %d and %d",
                              c, i, cspec->minimum, cspec->maximum);
                  return script_error (sc, error_message, 0);
                }

              g_value_set_uchar (&value, c);
            }
        }
      else if (G_VALUE_HOLDS_DOUBLE (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            {
              return script_type_error (sc, "numeric", i, proc_name);
            }
          else
            {
              GParamSpecDouble *dspec = G_PARAM_SPEC_DOUBLE (arg_spec);
              gdouble           d     = sc->vptr->rvalue (sc->vptr->pair_car (a));

              if (d < dspec->minimum || d > dspec->maximum)
                {
                  gchar error_message[1024];

                  g_snprintf (error_message, sizeof (error_message),
                              "Invalid value %f for argument %d: expected value between %f and %f",
                              d, i, dspec->minimum, dspec->maximum);
                  return script_error (sc, error_message, 0);
                }

              g_value_set_double (&value, d);
            }
        }
      else if (G_VALUE_HOLDS_ENUM (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            return script_type_error (sc, "numeric", i, proc_name);
          else
            g_value_set_enum (&value,
                              sc->vptr->ivalue (sc->vptr->pair_car (a)));
        }
      else if (G_VALUE_HOLDS_BOOLEAN (&value))
        {
          if (sc->vptr->is_number (sc->vptr->pair_car (a)))
            {
              /* Bind according to C idiom: 0 is false, other numeric values true.
               * This is not strict Scheme: 0 is truthy in Scheme.
               * This lets FALSE still work, where FALSE is a deprecated symbol for 0.
               */
              g_value_set_boolean (&value,
                                   sc->vptr->ivalue (sc->vptr->pair_car (a)));
            }
          else
            {
              if (is_interpret_v3_dialect ())
                {
                  /* Use Scheme semantics: anything but #f is true.
                   * This allows Scheme expressions yielding any Scheme type.
                   */
                  /* is_false is not exported from scheme.c (but should be.)
                   * This is the same code: compare Scheme pointers.
                   */
                  gboolean truth_value = ! (sc->vptr->pair_car (a) == sc->F);
                  g_value_set_boolean (&value, truth_value);
                }
              else
                {
                  /* v2 */
                  return script_type_error (sc, "numeric", i, proc_name);
                }
            }
        }
      else if (G_VALUE_HOLDS_STRING (&value))
        {
          if (! sc->vptr->is_string (sc->vptr->pair_car (a)))
            return script_type_error (sc, "string", i, proc_name);
          else
              g_value_set_string (&value,
                                  sc->vptr->string_value (sc->vptr->pair_car (a)));
        }
      else if (G_VALUE_HOLDS (&value, G_TYPE_STRV))
        {
          vector = sc->vptr->pair_car (a);  /* vector is pointing to a list */
          if (! sc->vptr->is_list (sc, vector))
            return script_type_error (sc, "vector", i, proc_name);
          else
            {
              gchar **array;

              n_elements = sc->vptr->list_length (sc, vector);

              array = g_new0 (gchar *, n_elements + 1);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->pair_car (vector);

                  if (!sc->vptr->is_string (v_element))
                    {
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not a string (argument %d for function %s)",
                                  j+1, i+1, proc_name);
                      g_strfreev (array);
                      return foreign_error (sc, error_str, vector);
                    }

                  array[j] = g_strdup (sc->vptr->string_value (v_element));

                  vector = sc->vptr->pair_cdr (vector);
                }

              g_value_take_boxed (&value, array);

#if DEBUG_MARSHALL
              {
                glong count = sc->vptr->list_length ( sc, sc->vptr->pair_car (a) );
                g_printerr ("      string vector has %ld elements\n", count);
                if (count > 0)
                  {
                    g_printerr ("     ");
                    for (j = 0; j < count; ++j)
                      g_printerr (" \"%s\"", array[j]);
                    g_printerr ("\n");
                  }
              }
#endif
            }
        }
      else if (GIMP_VALUE_HOLDS_DISPLAY (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            return script_type_error (sc, "numeric", i, proc_name);
          else
            {
              GimpDisplay *display =
                gimp_display_get_by_id (sc->vptr->ivalue (sc->vptr->pair_car (a)));

              g_value_set_object (&value, display);
            }
        }
      else if (GIMP_VALUE_HOLDS_IMAGE (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            return script_type_error (sc, "numeric", i, proc_name);
          else
            {
              GimpImage *image =
                gimp_image_get_by_id (sc->vptr->ivalue (sc->vptr->pair_car (a)));

              g_value_set_object (&value, image);
            }
        }
      else if (GIMP_VALUE_HOLDS_LAYER (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            return script_type_error (sc, "numeric", i, proc_name);
          else
            {
              GimpLayer *layer =
                gimp_layer_get_by_id (sc->vptr->ivalue (sc->vptr->pair_car (a)));

              g_value_set_object (&value, layer);
            }
        }
      else if (GIMP_VALUE_HOLDS_LAYER_MASK (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            return script_type_error (sc, "numeric", i, proc_name);
          else
            {
              GimpLayerMask *layer_mask =
                gimp_layer_mask_get_by_id (sc->vptr->ivalue (sc->vptr->pair_car (a)));

              g_value_set_object (&value, layer_mask);
            }
        }
      else if (GIMP_VALUE_HOLDS_CHANNEL (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            return script_type_error (sc, "numeric", i, proc_name);
          else
            {
              GimpChannel *channel =
                gimp_channel_get_by_id (sc->vptr->ivalue (sc->vptr->pair_car (a)));

              g_value_set_object (&value, channel);
            }
        }
      else if (GIMP_VALUE_HOLDS_DRAWABLE (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            return script_type_error (sc, "numeric", i, proc_name);
          else
            {
              gint id = sc->vptr->ivalue (sc->vptr->pair_car (a));

              pointer error = marshal_ID_to_item (sc, a, id, &value);
              if (error)
                return error;
            }
        }
      else if (GIMP_VALUE_HOLDS_PATH (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            return script_type_error (sc, "numeric", i, proc_name);
          else
            {
              GimpPath *path =
                gimp_path_get_by_id (sc->vptr->ivalue (sc->vptr->pair_car (a)));

              g_value_set_object (&value, path);
            }
        }
      else if (GIMP_VALUE_HOLDS_ITEM (&value))
        {
          if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
            return script_type_error (sc, "numeric", i, proc_name);
          else
            {
              gint item_ID;
              item_ID = sc->vptr->ivalue (sc->vptr->pair_car (a));

              /* Avoid failed assertion in libgimp.*/
              if (gimp_item_id_is_valid (item_ID))
                {
                  GimpItem *item = gimp_item_get_by_id (item_ID);
                  g_value_set_object (&value, item);
                }
              else
                {
                  return script_error (sc, "runtime: invalid item ID", a);
                }
            }
        }
      else if (GIMP_VALUE_HOLDS_INT32_ARRAY (&value))
        {
          vector = sc->vptr->pair_car (a);
          if (! sc->vptr->is_vector (vector))
            return script_type_error (sc, "vector", i, proc_name);
          else
            {
              /* !!! Comments applying to all array args.
               * n_elements is expected list length, from previous argument.
               * A PDB procedure takes args paired: ...length, array...
               * and a script passes the same paired args (..., 1, '("foo"))
               * (FUTURE: a more object oriented design for the PDB API
               * would obviate need for this discussion.)
               *
               * When a script passes a shorter or empty list,
               * ensure we don't segfault on cdr past end of list.
               *
               * n_elements is unsigned, we don't need to check >= 0
               *
               * Since we are not checking for equality of passed length
               * to actual container length, we adapt an array
               * that is shorter than specified by the length arg.
               * Ignoring a discrepancy by the script author.
               * FUTURE: List must be *exactly* n_elements long.
               * n_elements != sc->vptr->list_length (sc, vector))
               */
              gint32 *array;

              if (i == 0)
                return script_error (sc, "The first argument cannot be an array", a);
              else if (! g_type_is_a (arg_specs[i - 1]->value_type, G_TYPE_INT))
                return script_error (sc, "Array arguments must be preceded by an int argument (number of items)", a);

              g_object_get (config, arg_specs[i - 1]->name, &n_elements, NULL);

              if (n_elements > sc->vptr->vector_length (vector))
                return script_length_error_in_vector (sc, i, proc_name, n_elements, vector);

              array = g_new0 (gint32, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);

                  /* FIXME: Check values in vector stay within range for each type. */
                  if (! sc->vptr->is_number (v_element))
                    {
                      g_free (array);
                      return script_type_error_in_container (sc, "numeric", i, j, proc_name, vector);
                    }

                  array[j] = (gint32) sc->vptr->ivalue (v_element);
                }

              gimp_value_take_int32_array (&value, array, n_elements);

              debug_vector (sc, vector, "%ld");
            }
        }
      else if (G_VALUE_HOLDS (&value, G_TYPE_BYTES))
        {
          vector = sc->vptr->pair_car (a);
          if (! sc->vptr->is_vector (vector))
            return script_type_error (sc, "vector", i, proc_name);
          else
            {
              guint8 *array;

              n_elements = sc->vptr->vector_length (vector);

              array = g_new0 (guint8, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);

                  if (!sc->vptr->is_number (v_element))
                    {
                      g_free (array);
                      return script_type_error_in_container (sc, "numeric", i, j, proc_name, vector);
                    }

                  array[j] = (guint8) sc->vptr->ivalue (v_element);
                }

              g_value_take_boxed (&value, g_bytes_new_take (array, n_elements));

              debug_vector (sc, vector, "%ld");
            }
        }
      else if (GIMP_VALUE_HOLDS_FLOAT_ARRAY (&value))
        {
          vector = sc->vptr->pair_car (a);
          if (! sc->vptr->is_vector (vector))
            return script_type_error (sc, "vector", i, proc_name);
          else
            {
              gdouble *array;

              if (i == 0)
                return script_error (sc, "The first argument cannot be an array", a);
              else if (! g_type_is_a (arg_specs[i - 1]->value_type, G_TYPE_INT))
                return script_error (sc, "Array arguments must be preceded by an int argument (number of items)", a);

              g_object_get (config, arg_specs[i - 1]->name, &n_elements, NULL);

              if (n_elements > sc->vptr->vector_length (vector))
                return script_length_error_in_vector (sc, i, proc_name, n_elements, vector);

              array = g_new0 (gdouble, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);

                  if (!sc->vptr->is_number (v_element))
                    {
                      g_free (array);
                      return script_type_error_in_container (sc, "numeric", i, j, proc_name, vector);
                    }

                  array[j] = (gfloat) sc->vptr->rvalue (v_element);
                }

              gimp_value_take_float_array (&value, array, n_elements);

              debug_vector (sc, vector, "%f");
            }
        }
      else if (GIMP_VALUE_HOLDS_COLOR (&value))
        {
          GeglColor *color = NULL;

          if (sc->vptr->is_string (sc->vptr->pair_car (a)))
            {
              gchar *color_string = sc->vptr->string_value (sc->vptr->pair_car (a));

              if (! (color = sf_color_get_color_from_name (color_string)))
                return script_type_error (sc, "color string", i, proc_name);
            }
          else if (sc->vptr->is_list (sc, sc->vptr->pair_car (a)))
            {
              pointer color_list = sc->vptr->pair_car (a);

              if (! (color = marshal_component_list_to_color (sc, color_list)))
                return script_type_error (sc, "color list of numeric components", i, proc_name);
            }
          else
            {
              return script_type_error (sc, "color string or list", i, proc_name);
            }

          /* Transfer ownership. */
          g_value_take_object (&value, color);
        }
      else if (GIMP_VALUE_HOLDS_COLOR_ARRAY (&value))
        {
          vector = sc->vptr->pair_car (a);
          if (! sc->vptr->is_vector (vector))
            {
              return script_type_error (sc, "vector", i, proc_name);
            }
          else
            {
              GeglColor **colors;

              if (i == 0)
                return script_error (sc, "The first argument cannot be an array", a);
              else if (! g_type_is_a (arg_specs[i - 1]->value_type, G_TYPE_INT))
                return script_error (sc, "Array arguments must be preceded by an int argument (number of items)", a);

              g_object_get (config, arg_specs[i - 1]->name, &n_elements, NULL);

              if (n_elements > sc->vptr->vector_length (vector))
                return script_length_error_in_vector (sc, i, proc_name, n_elements, vector);

              colors = g_new0 (GeglColor *, n_elements + 1);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);
                  pointer color_list;
                  guchar  rgb[3];

                  if (! (sc->vptr->is_list (sc,
                                            sc->vptr->pair_car (v_element)) &&
                         sc->vptr->list_length (sc,
                                                sc->vptr->pair_car (v_element)) == 3))
                    {
                      gimp_color_array_free (colors);
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not a color "
                                  "(argument %d for function %s)",
                                  j+1, i+1, proc_name);
                      return script_error (sc, error_str, 0);
                    }

                  color_list = sc->vptr->pair_car (v_element);
                  rgb[0] = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)),
                                  0, 255);
                  color_list = sc->vptr->pair_cdr (color_list);
                  rgb[1] = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)),
                                  0, 255);
                  color_list = sc->vptr->pair_cdr (color_list);
                  rgb[2] = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)),
                                  0, 255);

                  colors[j] = gegl_color_new (NULL);
                  gegl_color_set_pixel (colors[j], babl_format ("R'G'B' u8"), rgb);
                }

              g_value_take_boxed (&value, colors);

              g_debug ("color vector has %ld elements", sc->vptr->vector_length (vector));
            }
        }
      else if (GIMP_VALUE_HOLDS_PARASITE (&value))
        {
          if (! sc->vptr->is_list (sc, sc->vptr->pair_car (a)) ||
              sc->vptr->list_length (sc, sc->vptr->pair_car (a)) != 3)
            return script_type_error (sc, "list", i, proc_name);
          else
            {
              GimpParasite parasite;
              pointer      temp_val;

              /* parasite->name */
              temp_val = sc->vptr->pair_car (a);

              if (! sc->vptr->is_string (sc->vptr->pair_car (temp_val)))
                return script_type_error_in_container (sc, "string", i, 0, proc_name, 0);

              parasite.name =
                sc->vptr->string_value (sc->vptr->pair_car (temp_val));
              g_debug ("name '%s'", parasite.name);

              /* parasite->flags */
              temp_val = sc->vptr->pair_cdr (temp_val);

              if (! sc->vptr->is_number (sc->vptr->pair_car (temp_val)))
                return script_type_error_in_container (sc, "numeric", i, 1, proc_name, 0);

              parasite.flags =
                sc->vptr->ivalue (sc->vptr->pair_car (temp_val));
                g_debug ("flags %d", parasite.flags);

              /* parasite->data */
              temp_val = sc->vptr->pair_cdr (temp_val);

              if (!sc->vptr->is_string (sc->vptr->pair_car (temp_val)))
                return script_type_error_in_container (
                  sc, "string", i, 2, proc_name, 0);

              parasite.data =
                sc->vptr->string_value (sc->vptr->pair_car (temp_val));
              parasite.size = strlen (parasite.data);

              g_debug ("size %d", parasite.size);
              g_debug ("data '%s'", (char *)parasite.data);

              g_value_set_boxed (&value, &parasite);
            }
        }
      else if (GIMP_VALUE_HOLDS_OBJECT_ARRAY (&value))
        {
          /* Now PDB procedures take arrays of Item (Drawable, Vectors, etc.).
           * When future PDB procedures take arrays of Image, Display, Resource, etc.
           * this will need changes.
           */
          vector = sc->vptr->pair_car (a);

          if (sc->vptr->is_vector (vector))
            {
              pointer error = marshal_vector_to_item_array (sc, vector, &value);
              if (error)
                return error;
            }
          else
              return script_type_error (sc, "vector", i, proc_name);
        }
      else if (G_VALUE_TYPE (&value) == G_TYPE_FILE)
        {
          if (! sc->vptr->is_string (sc->vptr->pair_car (a)))
            return script_type_error (sc, "string for path", i, proc_name);
          marshal_path_string_to_gfile (sc, a, &value);
        }
      else if (G_VALUE_TYPE (&value) == GIMP_TYPE_PDB_STATUS_TYPE)
        {
          /* A PDB procedure signature wrongly requires a status. */
          return implementation_error (sc,
                                       "Status is for return types, not arguments",
                                       sc->vptr->pair_car (a));
        }
      else if (GIMP_VALUE_HOLDS_RESOURCE (&value))
        {
          if (! sc->vptr->is_integer (sc->vptr->pair_car (a)))
            return script_type_error (sc, "integer", i, proc_name);
          else
            {
              /* Create new instance of a resource object. */
              GimpResource *resource;

              gint resource_id = sc->vptr->ivalue (sc->vptr->pair_car (a));
              /* Resource is abstract superclass. Concrete subclass is e.g. Brush.
               * The gvalue holds i.e. requires an instance of concrete subclass.
               * ID's are unique across all instances of Resource.
               */

              if (! gimp_resource_id_is_valid (resource_id))
               {
                  /* Not the ID of any instance of Resource. */
                  return script_error (sc, "runtime: invalid resource ID", a);
                }
              resource = gimp_resource_get_by_id (resource_id);
              if (! g_value_type_compatible (G_OBJECT_TYPE (resource), G_VALUE_TYPE (&value)))
                {
                  /* not the required subclass held by the gvalue */
                  return script_error (sc, "runtime: resource ID of improper subclass.", a);
                }
              g_value_set_object (&value, resource);
            }
        }
      else if (GIMP_VALUE_HOLDS_EXPORT_OPTIONS (&value))
        {
          /* ExportOptions is work in progress.
           * For now, you can't instantiate (no gimp_export_options_new())
           * and a script must always pass the equivalent of NULL
           * when calling PDB functions having formal arg type ExportOptions.
           *
           * TEMPORARY: eat the actual Scheme arg but bind to C NULL.
           * FUTURE: (unlikely) ScriptFu plugins can use non-NULL export options.
           * check Scheme type of actual arg, must be int ID
           * create a GimpExportOptions (actually a proxy?)
           */
            g_value_set_object (&value, NULL);
        }
      else
        {
          g_snprintf (error_str, sizeof (error_str),
                      "Argument %d for %s is unhandled type %s",
                      i+1, proc_name, g_type_name (G_VALUE_TYPE (&value)));
          return implementation_error (sc, error_str, 0);
        }
      debug_gvalue (&value);
      if (g_param_value_validate (arg_spec, &value))
        {
          gchar error_message[1024];

          g_snprintf (error_message, sizeof (error_message),
                      "Invalid value for argument %d",
                      i);
          g_value_unset (&value);

          return script_error (sc, error_message, 0);
        }
      g_object_set_property (G_OBJECT (config), arg_specs[i]->name, &value);
      g_value_unset (&value);
    }

  /* Omit refresh scripts from a script, better than crashing, see #575830. */
  if (strcmp (proc_name, "script-fu-refresh") == 0)
      return script_error (sc, "A script cannot refresh scripts", 0);

  g_debug ("calling %s", proc_name);
  values = gimp_procedure_run_config (procedure, config);
  g_debug ("done.");
  g_clear_object (&config);

  /*  Check the return status  */
  if (! values)
    {
      /* Usually a plugin that crashed, wire error */
      g_snprintf (error_str, sizeof(error_str),
                  "in script, called procedure %s failed to return a status",
                  proc_name);
      return script_error (sc, error_str, 0);
    }

  {
    pointer calling_error;
    return_val = marshal_PDB_return (sc, values, proc_name, &calling_error);

    /* Now returns error immediately.
     * Which leaks memory normally freed below.
     * Most plugins, except extension script-fu, will exit soon anyway.
     * FUTURE: don't leak.
     */
    if (calling_error != NULL)
      /* calling error is foreign_error or similar. */
      return calling_error;
  }

  g_free (proc_name);

  /*  free executed procedure return values  */
  gimp_value_array_unref (values);

  /*  free arguments and values  */

  /* The callback is NULL except for script-fu-server.  See explanation there. */
  if (post_command_callback != NULL)
    post_command_callback ();

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
  return script_fu_marshal_procedure_call (sc, a, FALSE, FALSE);
}

static pointer
script_fu_marshal_procedure_call_permissive (scheme  *sc,
                                             pointer  a)
{
  return script_fu_marshal_procedure_call (sc, a, TRUE, FALSE);
}

static pointer
script_fu_marshal_procedure_call_deprecated (scheme  *sc,
                                             pointer  a)
{
  return script_fu_marshal_procedure_call (sc, a, TRUE, TRUE);
}

static pointer
script_fu_register_call (scheme  *sc,
                         pointer  a)
{
  return script_fu_add_script (sc, a);
}

static pointer
script_fu_register_call_filter (scheme  *sc,
                                pointer  a)
{
  return script_fu_add_script_filter (sc, a);
}

static pointer
script_fu_menu_register_call (scheme  *sc,
                              pointer  a)
{
  return script_fu_add_menu (sc, a);
}

static pointer
script_fu_use_v3_call (scheme  *sc,
                       pointer  a)
{
  begin_interpret_v3_dialect ();
  return sc->NIL;
}

static pointer
script_fu_use_v2_call (scheme  *sc,
                       pointer  a)
{
  begin_interpret_v2_dialect ();
  return sc->NIL;
}

static pointer
script_fu_quit_call (scheme  *sc,
                     pointer  a)
{
  /* If script-fu-server running, tell it. */
  if (quit_callback != NULL)
    quit_callback ();

  scheme_deinit (sc);

  return sc->NIL;
}

static pointer
script_fu_nil_call (scheme  *sc,
                    pointer  a)
{
  return sc->NIL;
}
