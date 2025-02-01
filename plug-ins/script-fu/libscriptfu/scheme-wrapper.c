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

#include "script-fu-lib.h"
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

static void     ts_init_constants                                 (scheme              *sc,
                                                                   GIRepository        *repo);
static void     ts_init_enums                                     (scheme              *sc,
                                                                   GIRepository        *repo,
                                                                   const char          *namespace);
static void     ts_define_procedure                               (scheme              *sc,
                                                                   const gchar         *symbol_name,
                                                                   TsWrapperFunc        func);
static void     ts_init_procedures                                (scheme              *sc,
                                                                   gboolean             register_scipts);
static void     ts_load_init_and_compatibility_scripts            (GList               *paths);

static pointer  script_fu_marshal_arg_to_value                    (scheme              *sc,
                                                                   pointer              a,
                                                                   const gchar         *proc_name,
                                                                   gint                 arg_index,
                                                                   GParamSpec          *arg_spec,
                                                                   GValue              *value,
                                                                   gchar              **strvalue);

static pointer  script_fu_marshal_procedure_call                  (scheme              *sc,
                                                                   pointer              a,
                                                                   gboolean             permissive,
                                                                   gboolean             deprecated);
static pointer  script_fu_marshal_procedure_call_strict           (scheme              *sc,
                                                                   pointer              a);
static pointer  script_fu_marshal_procedure_call_permissive       (scheme              *sc,
                                                                   pointer              a);
static pointer  script_fu_marshal_procedure_call_deprecated       (scheme              *sc,
                                                                   pointer              a);
static pointer  script_fu_marshal_procedure_exists                (scheme              *sc,
                                                                   pointer              a);

static pointer  script_fu_marshal_drawable_create_filter          (scheme              *sc,
                                                                   pointer              a,
                                                                   const gchar         *proc_name,
                                                                   GimpDrawable       **drawable,
                                                                   GimpDrawableFilter **filter);
static pointer  script_fu_marshal_drawable_filter_configure_call  (scheme              *sc,
                                                                   pointer              a);
static pointer  script_fu_marshal_drawable_filter_set_aux_call    (scheme              *sc,
                                                                   pointer              a);
static pointer  script_fu_marshal_drawable_merge_filter_call      (scheme              *sc,
                                                                   pointer              a);
static pointer  script_fu_marshal_drawable_append_filter_call     (scheme              *sc,
                                                                   pointer              a);
static pointer  script_fu_marshal_drawable_merge_new_filter_call  (scheme              *sc,
                                                                   pointer               a);
static pointer  script_fu_marshal_drawable_append_new_filter_call (scheme              *sc,
                                                                   pointer              a);

static pointer  script_fu_register_call                           (scheme               *sc,
                                                                   pointer               a);
static pointer  script_fu_register_call_filter                    (scheme               *sc,
                                                                   pointer               a);
static pointer  script_fu_register_call_procedure                 (scheme               *sc,
                                                                   pointer               a);
static pointer  script_fu_menu_register_call                      (scheme               *sc,
                                                                   pointer               a);
static pointer  script_fu_register_i18n_call                      (scheme               *sc,
                                                                   pointer               a);
static pointer  script_fu_use_v3_call                             (scheme               *sc,
                                                                   pointer               a);
static pointer  script_fu_use_v2_call                             (scheme               *sc,
                                                                   pointer               a);
static pointer  script_fu_quit_call                               (scheme               *sc,
                                                                   pointer               a);
static pointer  script_fu_nil_call                                (scheme               *sc,
                                                                  pointer               a);

static gboolean ts_load_file                                      (const gchar         *dirname,
                                                                   const gchar         *basename);

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

  ts_load_init_and_compatibility_scripts (path);
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

/* Define a symbol into interpreter state, bound to a string, immutable. */
static void
ts_define_constant_string (scheme       *sc,
                           const gchar  *symbol_name,
                           const gchar  *symbol_value)
{
  pointer symbol = sc->vptr->mk_symbol (sc, symbol_name);

  sc->vptr->scheme_define (sc, sc->global_env, symbol,
                           sc->vptr->mk_string (sc, symbol_value));
  sc->vptr->setimmutable (symbol);
}

static void
ts_init_constants (scheme       *sc,
                   GIRepository *repo)
{
  ts_define_constant_string (sc, "gimp-directory",         gimp_directory ());
  ts_define_constant_string (sc, "gimp-data-directory",    gimp_data_directory ());
  ts_define_constant_string (sc, "gimp-plug-in-directory", gimp_plug_in_directory ());
  ts_define_constant_string (sc, "gimp-locale-directory",  gimp_locale_directory ());
  ts_define_constant_string (sc, "gimp-sysconf-directory", gimp_sysconf_directory ());

  {
    gchar *path;

    path = script_fu_sys_init_directory ();
    ts_define_constant_string (sc, "script-fu-sys-init-directory", path);
    g_free (path);
    path = script_fu_user_init_directory ();
    ts_define_constant_string (sc, "script-fu-user-init-directory", path);
    g_free (path);
  }

  ts_init_enums (sc, repo, "Gimp");
  ts_init_enums (sc, repo, "Gegl");

  /* Constants used in the register block of scripts e.g. SF-ADJUSTMENT */
  for (int i = 0; script_constants[i].name != NULL; ++i)
    {
      pointer symbol;

      symbol = sc->vptr->mk_symbol (sc, script_constants[i].name);
      sc->vptr->scheme_define (sc, sc->global_env, symbol,
                               sc->vptr->mk_integer (sc,
                                                     script_constants[i].value));
      sc->vptr->setimmutable (symbol);
    }

  /* use to build paths to files/directories */
  ts_define_constant_string (sc, "DIR-SEPARATOR",        G_DIR_SEPARATOR_S);
  /* use to build search paths */
  ts_define_constant_string (sc, "SEARCHPATH-SEPARATOR", G_SEARCHPATH_SEPARATOR_S);
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
      ts_define_procedure (sc, "script-fu-register-procedure", script_fu_register_call_procedure);
      ts_define_procedure (sc, "script-fu-menu-register",   script_fu_menu_register_call);
      ts_define_procedure (sc, "script-fu-register-i18n",   script_fu_register_i18n_call);
    }
  else
    {
      ts_define_procedure (sc, "script-fu-register",        script_fu_nil_call);
      ts_define_procedure (sc, "script-fu-register-filter", script_fu_nil_call);
      ts_define_procedure (sc, "script-fu-register-procedure", script_fu_nil_call);
      ts_define_procedure (sc, "script-fu-menu-register",   script_fu_nil_call);
      ts_define_procedure (sc, "script-fu-register-i18n",   script_fu_nil_call);
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

  ts_define_procedure (sc, "gimp-pdb-procedure-exists", script_fu_marshal_procedure_exists);

  ts_define_procedure (sc, "gimp-drawable-filter-configure", script_fu_marshal_drawable_filter_configure_call);
  ts_define_procedure (sc, "gimp-drawable-filter-set-aux-input", script_fu_marshal_drawable_filter_set_aux_call);
  ts_define_procedure (sc, "gimp-drawable-merge-filter", script_fu_marshal_drawable_merge_filter_call);
  ts_define_procedure (sc, "gimp-drawable-append-filter", script_fu_marshal_drawable_append_filter_call);
  ts_define_procedure (sc, "gimp-drawable-merge-new-filter", script_fu_marshal_drawable_merge_new_filter_call);
  ts_define_procedure (sc, "gimp-drawable-append-new-filter", script_fu_marshal_drawable_append_new_filter_call);

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

/* Load script defining much of Scheme in Scheme language.
 * This hides the actual name.

 * By convention in Scheme, the name is init.scm.
 * Other Schemes use a same named file for the same purpose.
 * The contents are more or less same as upstream TinyScheme.
 * The contents only define a Lisp dialect,
 * and not ScriptFu's additional bindings to the PDB.
 *
 * Returns TRUE on successful load.
 */
static gboolean
ts_load_main_init_script (gchar *dir)
{
  return ts_load_file (dir, "init.scm");
}

/* Load certain Scheme init scripts from certain directories.
 * Loads from two directories, user and sys, in that order.
 * Only loads from directories named "scriptfu-init.""
 * Only loads a small set of named files, not all .scm files in the directory.
 * Only loads the first set of init scripts found,
 * from the first directory where the main init script is found.
 * Does not recursively descend into the directories.
 *
 * We recommend a user not shadow the sys init scripts,
 * especially the main one: init.scm.
 * Should not shadow in the user init script directory,
 * or in any other script directory.
 */
static void
ts_load_init_and_compatibility_scripts (GList *paths)
{
  gboolean did_find_main_init_script = FALSE;

  g_debug ("%s", G_STRFUNC);

  if (paths == NULL)
    {
      g_warning ("%s Missing paths.", G_STRFUNC);
      return;
    }

  /* paths is a list of dirs known by ScriptFu, user specific and system wide.
   * The order is important, and this first searches user specific directories.
   */
  for (GList *list = paths; list; list = g_list_next (list))
    {
      /* Load from a designated init subdirectory.
       * Subsequent loading of ordinary scripts skips this subdir.
       */
      gchar *dir = script_fu_get_init_subdirectory (list->data);

      if (ts_load_main_init_script (dir))
        {
          did_find_main_init_script = TRUE;

          /* Load small set of named other init scripts.
           * Only from same dir as the main init script!
           *
           * We don't warn when they are missing.
           */

          /* Improve compatibility with older Script-Fu scripts,
           * load definitions for old dialects of Lisp (SIOD) or older ScriptFu.
           */
          (void) ts_load_file (dir, "script-fu-compat.scm");

          /* Improve compatibility with older GIMP version,
           * load definitions that alias/adapt older PDB procedures or plugins.
           */
          (void) ts_load_file (dir, "plug-in-compat.scm");

          g_free (dir);

          /* !!! Only load init scripts from the first dir found. */
          break;
        }

      g_free (dir);
    }

  if (! did_find_main_init_script)
    {
      /* Continue, but the interpreter will be crippled. */
      g_warning ("Failed to load main initialization file");
    }
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

/* Returns pointer to sc->NIL (normal) or pointer to error. */
static pointer
script_fu_marshal_arg_to_value (scheme       *sc,
                                pointer       a,
                                const gchar  *proc_name,
                                gint          arg_index,
                                GParamSpec   *arg_spec,
                                GValue       *value,
                                gchar       **strvalue)
{
  pointer arg_val;
  pointer vector;
  guint   n_elements;
  gint    j;
  gchar   error_str[1024];

  if (sc->vptr->is_pair (a))
    arg_val = sc->vptr->pair_car (a);
  else
    arg_val = a;

  if (G_VALUE_HOLDS_INT (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          GParamSpecInt *ispec = G_PARAM_SPEC_INT (arg_spec);
          gint           v     = sc->vptr->ivalue (sc->vptr->pair_car (a));

          if (! (arg_spec->flags & GIMP_PARAM_NO_VALIDATE) &&
              (v < ispec->minimum || v > ispec->maximum))
            return script_int_range_error (sc, arg_index, proc_name,
                                           ispec->minimum, ispec->maximum, v);

          g_value_set_int (value, v);
          if (strvalue)
            *strvalue = g_strdup_printf ("%d", v);
        }
    }
  else if (G_VALUE_HOLDS_UINT (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          GParamSpecUInt *ispec = G_PARAM_SPEC_UINT (arg_spec);
          gint            v     = sc->vptr->ivalue (arg_val);

          if (! (arg_spec->flags & GIMP_PARAM_NO_VALIDATE) &&
              (v < ispec->minimum || v > ispec->maximum))
            return script_int_range_error (sc, arg_index, proc_name,
                                           ispec->minimum, ispec->maximum, v);

          g_value_set_uint (value, v);
          if (strvalue)
            *strvalue = g_strdup_printf ("%u", v);
        }
    }
  else if (G_VALUE_HOLDS_UCHAR (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          GParamSpecUChar *cspec = G_PARAM_SPEC_UCHAR (arg_spec);
          gint             c     = sc->vptr->ivalue (arg_val);

          if (! (arg_spec->flags & GIMP_PARAM_NO_VALIDATE) &&
              (c < cspec->minimum || c > cspec->maximum))
            return script_int_range_error (sc, arg_index, proc_name,
                                           cspec->minimum, cspec->maximum, c);

          g_value_set_uchar (value, c);
          if (strvalue)
            *strvalue = g_strdup_printf ("%d", c);
        }
    }
  else if (G_VALUE_HOLDS_DOUBLE (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          GParamSpecDouble *dspec = G_PARAM_SPEC_DOUBLE (arg_spec);
          gdouble           d     = sc->vptr->rvalue (arg_val);

          if (! (arg_spec->flags & GIMP_PARAM_NO_VALIDATE) &&
              (d < dspec->minimum || d > dspec->maximum))
            return script_float_range_error (sc, arg_index, proc_name, dspec->minimum, dspec->maximum, d);

          g_value_set_double (value, d);
          if (strvalue)
            *strvalue = g_strdup_printf ("%f", d);
        }
    }
  else if (G_VALUE_HOLDS_ENUM (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          gint e = sc->vptr->ivalue (arg_val);

          g_value_set_enum (value, e);
          if (strvalue)
            *strvalue = g_strdup_printf ("%d", e);
        }
    }
  else if (G_VALUE_HOLDS_BOOLEAN (value))
    {
      if (sc->vptr->is_number (arg_val))
        {
          gboolean b = sc->vptr->ivalue (arg_val);

          /* Bind according to C idiom: 0 is false, other numeric values true.
           * This is not strict Scheme: 0 is truthy in Scheme.
           * This lets FALSE still work, where FALSE is a deprecated symbol for 0.
           */
          g_value_set_boolean (value, b);
          if (strvalue)
            *strvalue = g_strdup_printf ("%s", b ? "TRUE" : "FALSE");
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
              gboolean truth_value = ! (arg_val == sc->F);
              g_value_set_boolean (value, truth_value);
              if (strvalue)
                *strvalue = g_strdup_printf ("%s", truth_value ? "TRUE" : "FALSE");
            }
          else
            {
              /* v2 */
              return script_type_error (sc, "numeric", arg_index, proc_name);
            }
        }
    }
  else if (G_VALUE_HOLDS_STRING (value))
    {
      if (! sc->vptr->is_string (arg_val))
        {
          return script_type_error (sc, "string", arg_index, proc_name);
        }
      else
        {
          const gchar *s = sc->vptr->string_value (arg_val);

          g_value_set_string (value, s);
          if (strvalue)
            {
              gchar *escaped = g_strescape (s, NULL);
              *strvalue = g_strdup_printf ("\"%s\"", escaped);
              g_free (escaped);
            }
        }
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_STRV))
    {
      vector = arg_val;  /* vector is pointing to a list */
      if (! sc->vptr->is_list (sc, vector))
        {
          return script_type_error (sc, "vector", arg_index, proc_name);
        }
      else
        {
          gchar   **array;
          GString  *v = NULL;

          n_elements = sc->vptr->list_length (sc, vector);

          array = g_new0 (gchar *, n_elements + 1);
          if (strvalue)
            v = g_string_new ("");

          for (j = 0; j < n_elements; j++)
            {
              pointer v_element = sc->vptr->pair_car (vector);

              if (!sc->vptr->is_string (v_element))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "Item %d in vector is not a string (argument %d for function %s)",
                              j+1, arg_index+1, proc_name);
                  g_strfreev (array);
                  if (v)
                    g_string_free (v, TRUE);
                  return foreign_error (sc, error_str, vector);
                }

              array[j] = g_strdup (sc->vptr->string_value (v_element));
              if (v)
                {
                  gchar *escaped = g_strescape (array[j], NULL);
                  g_string_append_printf (v, "%s\"%s\"", j == 0 ? "" : " ", escaped);
                  g_free (escaped);
                }

              vector = sc->vptr->pair_cdr (vector);
            }

          g_value_take_boxed (value, array);

#if DEBUG_MARSHALL
            {
              glong count = sc->vptr->list_length ( sc, arg_val );
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

          if (v)
            {
              *strvalue = g_strdup_printf ("#(%s)", v->str);
              g_string_free (v, TRUE);
            }
        }
    }
  else if (GIMP_VALUE_HOLDS_DISPLAY (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          GimpDisplay *display;
          gint         id = sc->vptr->ivalue (arg_val);

          display = gimp_display_get_by_id (id);

          g_value_set_object (value, display);

          if (strvalue)
            *strvalue = g_strdup_printf ("%d", id);
        }
    }
  else if (GIMP_VALUE_HOLDS_IMAGE (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          GimpImage *image;
          gint       id = sc->vptr->ivalue (arg_val);

          image = gimp_image_get_by_id (id);

          g_value_set_object (value, image);

          if (strvalue)
            *strvalue = g_strdup_printf ("%d", id);
        }
    }
  else if (GIMP_VALUE_HOLDS_LAYER (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          GimpLayer *layer;
          gint       id = sc->vptr->ivalue (arg_val);

          layer = gimp_layer_get_by_id (id);

          g_value_set_object (value, layer);

          if (strvalue)
            *strvalue = g_strdup_printf ("%d", id);
        }
    }
  else if (GIMP_VALUE_HOLDS_LAYER_MASK (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          GimpLayerMask *layer_mask;
          gint           id = sc->vptr->ivalue (arg_val);

          layer_mask = gimp_layer_mask_get_by_id (id);

          g_value_set_object (value, layer_mask);

          if (strvalue)
            *strvalue = g_strdup_printf ("%d", id);
        }
    }
  else if (GIMP_VALUE_HOLDS_CHANNEL (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          GimpChannel *channel;
          gint         id = sc->vptr->ivalue (arg_val);

          channel = gimp_channel_get_by_id (id);

          g_value_set_object (value, channel);

          if (strvalue)
            *strvalue = g_strdup_printf ("%d", id);
        }
    }
  else if (GIMP_VALUE_HOLDS_DRAWABLE (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          gint id = sc->vptr->ivalue (arg_val);

          pointer error = marshal_ID_to_item (sc, a, id, value);
          if (error)
            return error;

          if (strvalue)
            *strvalue = g_strdup_printf ("%d", id);
        }
    }
  else if (GIMP_VALUE_HOLDS_PATH (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          GimpPath *path;
          gint      id = sc->vptr->ivalue (arg_val);

          path = gimp_path_get_by_id (id);

          g_value_set_object (value, path);

          if (strvalue)
            *strvalue = g_strdup_printf ("%d", id);
        }
    }
  else if (GIMP_VALUE_HOLDS_ITEM (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          gint id = sc->vptr->ivalue (arg_val);

          if (gimp_item_id_is_valid (id))
            {
              GimpItem *item = gimp_item_get_by_id (id);
              g_value_set_object (value, item);
            }
          else
            {
              /* item ID is invalid.
               * Usually 0 or -1, passed for a nullable arg.
               */
              g_value_set_object (value, NULL);
            }

          if (strvalue)
            *strvalue = g_strdup_printf ("%d", id);
        }
    }
  else if (GIMP_VALUE_HOLDS_DRAWABLE_FILTER (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          gint id = sc->vptr->ivalue (arg_val);

          if (gimp_drawable_filter_id_is_valid (id))
            {
              GimpDrawableFilter *filter = gimp_drawable_filter_get_by_id (id);

              g_value_set_object (value, filter);
            }
          else
            {
              /* Filter ID is invalid.
               * Usually 0 or -1, passed for a nullable arg.
               */
              g_value_set_object (value, NULL);
            }

          if (strvalue)
            *strvalue = g_strdup_printf ("%d", id);
        }
    }
  else if (GIMP_VALUE_HOLDS_UNIT (value))
    {
      if (! sc->vptr->is_number (arg_val))
        {
          return script_type_error (sc, "numeric", arg_index, proc_name);
        }
      else
        {
          GimpUnit *unit;
          gint      id = sc->vptr->ivalue (arg_val);

          unit = gimp_unit_get_by_id (id);

          g_value_set_object (value, unit);

          if (strvalue)
            *strvalue = g_strdup_printf ("%d", id);
        }
    }
  else if (GIMP_VALUE_HOLDS_INT32_ARRAY (value))
    {
      vector = arg_val;
      if (! sc->vptr->is_vector (vector))
        {
          return script_type_error (sc, "vector", arg_index, proc_name);
        }
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
          gint32  *array;
          GString *v = NULL;

          n_elements = sc->vptr->vector_length (vector);
          array = g_new0 (gint32, n_elements);

          if (strvalue)
            v = g_string_new ("");

          for (j = 0; j < n_elements; j++)
            {
              pointer v_element = sc->vptr->vector_elem (vector, j);

              /* FIXME: Check values in vector stay within range for each type. */
              if (! sc->vptr->is_number (v_element))
                {
                  g_free (array);
                  if (v)
                    g_string_free (v, TRUE);
                  return script_type_error_in_container (sc, "numeric", arg_index, j, proc_name, vector);
                }

              array[j] = (gint32) sc->vptr->ivalue (v_element);
              if (v)
                g_string_append_printf (v, "%s%d", j == 0 ? "" : " ", array[j]);
            }

          gimp_value_take_int32_array (value, array, n_elements);
          if (v)
            {
              *strvalue = g_strdup_printf ("#(%s)", v->str);
              g_string_free (v, TRUE);
            }

          debug_vector (sc, vector, "%ld");
        }
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_BYTES))
    {
      vector = arg_val;
      if (! sc->vptr->is_vector (vector))
        {
          return script_type_error (sc, "vector", arg_index, proc_name);
        }
      else
        {
          guint8   *array;
          GString  *v = NULL;

          n_elements = sc->vptr->vector_length (vector);

          array = g_new0 (guint8, n_elements);
          if (strvalue)
            v = g_string_new ("");

          for (j = 0; j < n_elements; j++)
            {
              pointer v_element = sc->vptr->vector_elem (vector, j);

              if (!sc->vptr->is_number (v_element))
                {
                  g_free (array);
                  if (v)
                    g_string_free (v, TRUE);
                  return script_type_error_in_container (sc, "numeric", arg_index, j, proc_name, vector);
                }

              array[j] = (guint8) sc->vptr->ivalue (v_element);
              if (v)
                g_string_append_printf (v, "%s%d", j == 0 ? "" : " ", array[j]);
            }

          g_value_take_boxed (value, g_bytes_new_take (array, n_elements));
          if (v)
            {
              *strvalue = g_strdup_printf ("#(%s)", v->str);
              g_string_free (v, TRUE);
            }

          debug_vector (sc, vector, "%ld");
        }
    }
  else if (GIMP_VALUE_HOLDS_DOUBLE_ARRAY (value))
    {
      vector = arg_val;
      if (! sc->vptr->is_vector (vector))
        {
          return script_type_error (sc, "vector", arg_index, proc_name);
        }
      else
        {
          gdouble *array;
          GString *v = NULL;

          n_elements = sc->vptr->vector_length (vector);
          array = g_new0 (gdouble, n_elements);
          if (strvalue)
            v = g_string_new ("");

          for (j = 0; j < n_elements; j++)
            {
              pointer v_element = sc->vptr->vector_elem (vector, j);

              if (!sc->vptr->is_number (v_element))
                {
                  g_free (array);
                  if (v)
                    g_string_free (v, TRUE);
                  return script_type_error_in_container (sc, "numeric", arg_index, j, proc_name, vector);
                }

              array[j] = (gdouble) sc->vptr->rvalue (v_element);
              if (v)
                g_string_append_printf (v, "%s%f", j == 0 ? "" : " ", array[j]);
            }

          gimp_value_take_double_array (value, array, n_elements);
          if (v)
            {
              *strvalue = g_strdup_printf ("#(%s)", v->str);
              g_string_free (v, TRUE);
            }

          debug_vector (sc, vector, "%f");
        }
    }
  else if (GIMP_VALUE_HOLDS_COLOR (value))
    {
      GeglColor *color = NULL;

      if (sc->vptr->is_string (arg_val))
        {
          gchar *color_string = sc->vptr->string_value (arg_val);

          if (! (color = sf_color_get_color_from_name (color_string)))
            return script_type_error (sc, "color string", arg_index, proc_name);

          if (strvalue)
            *strvalue = g_strdup_printf ("\"%s\"", color_string);
        }
      else if (sc->vptr->is_list (sc, arg_val))
        {
          pointer color_list = arg_val;

          if (! (color = marshal_component_list_to_color (sc, color_list, strvalue)))
            return script_type_error (sc, "color list of numeric components", arg_index, proc_name);
        }
      else
        {
          return script_type_error (sc, "color string or list", arg_index, proc_name);
        }

      /* Transfer ownership. */
      g_value_take_object (value, color);
    }
  else if (GIMP_VALUE_HOLDS_COLOR_ARRAY (value))
    {
      vector = arg_val;
      if (! sc->vptr->is_vector (vector))
        {
          return script_type_error (sc, "vector", arg_index, proc_name);
        }
      else
        {
          GeglColor **colors;
          GString    *v = NULL;

          n_elements = sc->vptr->vector_length (vector);

          colors = g_new0 (GeglColor *, n_elements + 1);
          if (strvalue)
            v = g_string_new ("");

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
                              j+1, arg_index+1, proc_name);
                  if (v)
                    g_string_free (v, TRUE);
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
              if (v)
                g_string_append_printf (v, " '(%d %d %d)", rgb[0], rgb[1], rgb[2]);
            }

          g_value_take_boxed (value, colors);
          if (v)
            {
              *strvalue = g_strdup_printf ("#(%s)", v->str);
              g_string_free (v, TRUE);
            }

          g_debug ("color vector has %ld elements", sc->vptr->vector_length (vector));
        }
    }
  else if (GIMP_VALUE_HOLDS_PARASITE (value))
    {
      if (! sc->vptr->is_list (sc, arg_val) ||
          sc->vptr->list_length (sc, arg_val) != 3)
        {
          return script_type_error (sc, "list", arg_index, proc_name);
        }
      else
        {
          GimpParasite parasite;
          pointer      temp_val;

          /* parasite->name */
          temp_val = arg_val;

          if (! sc->vptr->is_string (sc->vptr->pair_car (temp_val)))
            return script_type_error_in_container (sc, "string", arg_index, 0, proc_name, 0);

          parasite.name =
            sc->vptr->string_value (sc->vptr->pair_car (temp_val));
          g_debug ("name '%s'", parasite.name);

          /* parasite->flags */
          temp_val = sc->vptr->pair_cdr (temp_val);

          if (! sc->vptr->is_number (sc->vptr->pair_car (temp_val)))
            return script_type_error_in_container (sc, "numeric", arg_index, 1, proc_name, 0);

          parasite.flags =
            sc->vptr->ivalue (sc->vptr->pair_car (temp_val));
          g_debug ("flags %d", parasite.flags);

          /* parasite->data */
          temp_val = sc->vptr->pair_cdr (temp_val);

          if (!sc->vptr->is_string (sc->vptr->pair_car (temp_val)))
            return script_type_error_in_container (sc, "string", arg_index, 2, proc_name, 0);

          parasite.data =
            sc->vptr->string_value (sc->vptr->pair_car (temp_val));
          parasite.size = strlen (parasite.data);

          g_debug ("size %d", parasite.size);
          g_debug ("data '%s'", (char *)parasite.data);

          g_value_set_boxed (value, &parasite);
          if (strvalue)
            {
              gchar *escaped_name = g_strescape (parasite.name, NULL);
              gchar *escaped_data = g_strescape (parasite.data, NULL);

              *strvalue = g_strdup_printf ("(\"%s\" %d \"%s\")",
                                           escaped_name, parasite.flags, escaped_data);
              g_free (escaped_name);
              g_free (escaped_data);
            }
        }
    }
  else if (GIMP_VALUE_HOLDS_CORE_OBJECT_ARRAY (value))
    {
      /* Now PDB procedures take arrays of Item (Drawable, Vectors, etc.).
       * When future PDB procedures take arrays of Image, Display, Resource, etc.
       * this will need changes.
       */
      vector = arg_val;

      if (sc->vptr->is_vector (vector))
        {
          pointer error = marshal_vector_to_item_array (sc, vector, value, strvalue);
          if (error)
            return error;
        }
      else
        {
          return script_type_error (sc, "vector", arg_index, proc_name);
        }
    }
  else if (G_VALUE_TYPE (value) == G_TYPE_FILE)
    {
      if (! sc->vptr->is_string (arg_val))
        return script_type_error (sc, "string for path", arg_index, proc_name);
      marshal_path_string_to_gfile (sc, a, value);

      if (strvalue)
        *strvalue = g_strdup_printf ("%s", sc->vptr->string_value (arg_val));
    }
  else if (G_VALUE_TYPE (value) == GIMP_TYPE_PDB_STATUS_TYPE)
    {
      /* A PDB procedure signature wrongly requires a status. */
      return implementation_error (sc,
                                   "Status is for return types, not arguments",
                                   arg_val);
    }
  else if (GIMP_VALUE_HOLDS_RESOURCE (value))
    {
      if (! sc->vptr->is_integer (arg_val))
        {
          return script_type_error (sc, "integer", arg_index, proc_name);
        }
      else
        {
          /* Create new instance of a resource object. */
          GimpResource *resource;

          gint resource_id = sc->vptr->ivalue (arg_val);
          /* Resource is abstract superclass. Concrete subclass is e.g. Brush.
           * The gvalue holds arg_index.e. requires an instance of concrete subclass.
           * ID's are unique across all instances of Resource.
           */

          if (! gimp_resource_id_is_valid (resource_id))
            {
              /* Not the ID of any instance of Resource. */
              return script_error (sc, "runtime: invalid resource ID", a);
            }
          resource = gimp_resource_get_by_id (resource_id);
          if (! g_value_type_compatible (G_OBJECT_TYPE (resource), G_VALUE_TYPE (value)))
            {
              /* not the required subclass held by the gvalue */
              return script_error (sc, "runtime: resource ID of improper subclass.", a);
            }
          g_value_set_object (value, resource);
          if (strvalue)
            *strvalue = g_strdup_printf ("%d", resource_id);
        }
    }
  else if (GIMP_VALUE_HOLDS_EXPORT_OPTIONS (value))
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
      g_value_set_object (value, NULL);
      if (strvalue)
        *strvalue = g_strdup_printf ("%d", -1);
    }
  else
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Argument %d for %s is unhandled type %s",
                  arg_index+1, proc_name, g_type_name (G_VALUE_TYPE (value)));
      return implementation_error (sc, error_str, 0);
    }

  return sc->NIL;
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

  g_debug ("%s, proc name: %s, args rcvd: %d",
           G_STRFUNC,
           proc_name,
           sc->vptr->list_length (sc, a)-1);

  if (deprecated )
    g_info ("PDB procedure name %s is deprecated, please use %s.",
               deprecated_name_for (proc_name),
               proc_name);

  if (script_fu_report_progress ())
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

  a = sc->vptr->pair_cdr (a);

  /* Check the supplied number of arguments.
   * This only gives messages to the console.
   * It does not ensure that the count of supplied args equals the count of formal args.
   * Subsequent code must not assume that.
   *
   * When too few supplied args, when permissive, scriptfu or downstream machinery
   * can try to provide missing args e.g. defaults.
   *
   * Extra supplied args can be discarded.
   * Formerly, this was a deprecated behavior depending on "permissive".
   */
  if (gimp_procedure_is_internal (procedure))
    {
      if (actual_arg_count > n_arg_specs)
        {
          /* Permit extra args. Will discard args from script, to next right paren.*/
          g_info ("in script, permitting too many args to %s", proc_name);
        }
      else if (actual_arg_count < n_arg_specs)
        {
          /* Permit too few args.  The config carries a sane default for most types. */
          g_info ("in script, permitting too few args to %s", proc_name);
        }
      /* else equal counts of args. */
    }

  /*  Marshall the supplied arguments  */
  if (gimp_procedure_is_internal (procedure) ||
      ! sc->vptr->is_arg_name (sc->vptr->pair_car (a)))
    {
      GString *deprecation_warning = NULL;

      if (! gimp_procedure_is_internal (procedure))
        {
          deprecation_warning = g_string_new ("Calling Plug-In PDB procedures with arguments as an ordered list is deprecated.\n"
                                              "Please use named arguments: (");
          g_string_append (deprecation_warning, proc_name);
        }

      for (i = 0; i < n_arg_specs; i++)
        {
          GParamSpec *arg_spec = arg_specs[i];
          GValue      value    = G_VALUE_INIT;
          gchar      *strvalue = NULL;

          consumed_arg_count++;

          if (gimp_procedure_is_internal (procedure) &&
              sc->vptr->is_arg_name (sc->vptr->pair_car (a)))
            {
              g_snprintf (error_str, sizeof (error_str),
                          "Calling Internal PDB procedures with named arguments is not authorized.\n"
                          "Only use the named arguments syntax for Plug-In PDB procedures.");
              return script_error (sc, error_str, 0);
            }

          if (consumed_arg_count > actual_arg_count)
            {
              /* Exhausted supplied arguments before formal specs. */

              /* Say formal type of first missing arg. */
              g_warning ("Missing arg type: %s", g_type_name (G_PARAM_SPEC_VALUE_TYPE (arg_spec)));

              /* Break loop over formal specs. Continuation is to call PDB with partial args. */
              break;
            }

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (arg_spec));

          debug_in_arg (sc, a, i, g_type_name (G_VALUE_TYPE (&value)));

          return_val = script_fu_marshal_arg_to_value (sc, a, proc_name, i, arg_spec, &value, &strvalue);

          if (return_val != sc->NIL)
            {
              g_value_unset (&value);
              return return_val;
            }

          debug_gvalue (&value);
          if (! (arg_spec->flags & GIMP_PARAM_NO_VALIDATE) &&
              g_param_value_validate (arg_spec, &value))
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

          if (deprecation_warning != NULL)
            g_string_append_printf (deprecation_warning, " #:%s %s",
                                    arg_specs[i]->name, strvalue);

          a = sc->vptr->pair_cdr (a);

          g_free (strvalue);
        }
      if (deprecation_warning != NULL)
        {
          g_string_append (deprecation_warning, ")");
          g_warning ("%s", deprecation_warning->str);

          g_string_free (deprecation_warning, TRUE);
        }
    }
  else
    {
      for (i = 0; i < actual_arg_count; i++)
        {
          GParamSpec *arg_spec;
          gchar      *arg_name;
          GValue      value = G_VALUE_INIT;

          if (! sc->vptr->is_arg_name (sc->vptr->pair_car (a)))
            {
              g_snprintf (error_str, sizeof (error_str),
                          "Expected argument name for argument %d", i);
              return script_error (sc, error_str, 0);
            }

          arg_name = g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));

          arg_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), arg_name);
          if (arg_spec == NULL)
            {
              g_snprintf (error_str, sizeof (error_str),
                          "Invalid argument name: %s", arg_name);
              g_free (arg_name);
              return script_error (sc, error_str, 0);
            }

          if (i == actual_arg_count - 1)
            {
              g_snprintf (error_str, sizeof (error_str),
                          "Lonely argument with no value: %s", arg_name);
              g_free (arg_name);
              return script_error (sc, error_str, 0);
            }
          else
            {
              a = sc->vptr->pair_cdr (a);
              i++;
            }

          g_value_init (&value, arg_spec->value_type);
          return_val = script_fu_marshal_arg_to_value (sc, a, proc_name, i, arg_spec, &value, NULL);
          if (return_val != sc->NIL)
            {
              g_value_unset (&value);
              g_free (arg_name);
              return return_val;
            }

          g_object_set_property (G_OBJECT (config), arg_name, &value);
          g_value_unset (&value);

          a = sc->vptr->pair_cdr (a);
        }
    }

  /* Omit refresh scripts from a script, better than crashing, see #575830. */
  if (strcmp (proc_name, "script-fu-refresh") == 0)
      return script_error (sc, "A script cannot refresh scripts", 0);

  g_debug ("%s, calling:%s", G_STRFUNC, proc_name);
  values = gimp_procedure_run_config (procedure, config);
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
script_fu_marshal_procedure_exists (scheme  *sc,
                                    pointer  a)
{
  const gchar *proc_name  = "gimp-pdb-procedure-exists";
  const gchar *test_proc_name;
  gboolean     exists     = FALSE;
  gchar        error_str[1024];

  if (a == sc->NIL)
    {
      g_snprintf (error_str, sizeof (error_str),
                  "(%s) was called with no arguments. "
                  "A procedure name must be specified.",
                  proc_name);

      return implementation_error (sc, error_str, 0);
    }

  if (sc->vptr->list_length (sc, a) != 1)
    {
      g_snprintf (error_str, sizeof (error_str),
                  "(%s) was called with %d arguments. "
                  "Only a procedure name must be specified.",
                  proc_name, sc->vptr->list_length (sc, a));

      return implementation_error (sc, error_str, 0);
    }

  if (! sc->vptr->is_string (sc->vptr->pair_car (a)))
    return script_type_error (sc, "string", 1, proc_name);

  test_proc_name = sc->vptr->string_value (sc->vptr->pair_car (a));

  exists = gimp_pdb_procedure_exists (gimp_get_pdb (), test_proc_name);

  if (is_interpret_v3_dialect ())
    return exists ? sc->T : sc->F;
  else
    return sc->vptr->cons (sc,
                           sc->vptr->mk_integer (sc, exists),
                           sc->NIL);
}

static pointer
script_fu_marshal_procedure_call_deprecated (scheme  *sc,
                                             pointer  a)
{
  return script_fu_marshal_procedure_call (sc, a, TRUE, TRUE);
}

static pointer
script_fu_marshal_drawable_filter_configure (scheme             *sc,
                                             pointer             a,
                                             const gchar        *proc_name,
                                             gint                arg_index,
                                             GimpDrawableFilter *filter)
{
  pointer                   return_val  = sc->NIL;
  GimpLayerMode             mode        = GIMP_LAYER_MODE_REPLACE;
  gdouble                   opacity     = 1.0;
  GimpDrawableFilterConfig *config;
  gchar                     error_str[1024];

  if (sc->vptr->list_length (sc, a) > 0)
    {
      mode = sc->vptr->ivalue (sc->vptr->pair_car (a));
      a = sc->vptr->pair_cdr (a);
    }

  if (sc->vptr->list_length (sc, a) > 0)
    {
      opacity = sc->vptr->rvalue (sc->vptr->pair_car (a));
      a = sc->vptr->pair_cdr (a);
    }
  gimp_drawable_filter_set_opacity (filter, opacity);
  gimp_drawable_filter_set_blend_mode (filter, mode);

  config = gimp_drawable_filter_get_config (filter);
  while (sc->vptr->list_length (sc, a) > 1)
    {
      gchar      *argname;
      GParamSpec *arg_spec;
      GValue      value = G_VALUE_INIT;

      if (! sc->vptr->is_arg_name (sc->vptr->pair_car (a)) &&
          ! sc->vptr->is_string (sc->vptr->pair_car (a)))
        {
          g_snprintf (error_str, sizeof (error_str),
                      "Expected argument name for argument %d", arg_index);
          return script_error (sc, error_str, 0);
        }

      argname  = g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
      arg_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), argname);
      if (arg_spec == NULL)
        {
          g_snprintf (error_str, sizeof (error_str),
                      "Invalid argument name: %s", argname);
          g_free (argname);
          gimp_drawable_filter_delete (filter);
          return script_error (sc, error_str, 0);
        }
      g_value_init (&value, arg_spec->value_type);
      a = sc->vptr->pair_cdr (a);

      return_val = script_fu_marshal_arg_to_value (sc, a, proc_name, arg_index, arg_spec, &value, NULL);

      if (return_val != sc->NIL)
        {
          g_value_unset (&value);
          g_free (argname);
          gimp_drawable_filter_delete (filter);
          return return_val;
        }

      g_object_set_property (G_OBJECT (config), argname, &value);
      g_value_unset (&value);

      a = sc->vptr->pair_cdr (a);
      arg_index += 2;
    }

  return sc->NIL;
}

static pointer
script_fu_marshal_drawable_create_filter (scheme              *sc,
                                          pointer              a,
                                          const gchar         *proc_name,
                                          GimpDrawable       **drawable,
                                          GimpDrawableFilter **filter)
{
  gchar *operation_name;
  gchar *filter_name = NULL;
  gchar  error_str[1024];

  if (sc->vptr->list_length (sc, a) < 2)
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Drawable Filter marshaller was called with missing arguments. "
                  "The drawable ID, the GEGL operation, filter name, blend mode, opacity "
                  "and the arguments' names and values it requires (possibly none) must be specified: "
                  "(%s drawable op title mode opacity arg1 val1 arg2 val2...)",
                  proc_name);
      return implementation_error (sc, error_str, 0);
    }
  else if (sc->vptr->list_length (sc, a) > 5 && sc->vptr->list_length (sc, a) % 2 != 1)
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Drawable Filter marshaller was called with an even number of arguments. "
                  "The drawable ID, the GEGL operation, filter name, blend mode, opacity "
                  "and the arguments' names and values it requires (possibly none) must be specified: "
                  "(%s drawable op title mode opacity arg1 val1 arg2 val2...)",
                  proc_name);
      return implementation_error (sc, error_str, 0);
    }
  else if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
    {
      return script_type_error (sc, "numeric", 0, proc_name);
    }
  else
    {
      GimpItem *item;
      gint      id;

      id   = sc->vptr->ivalue (sc->vptr->pair_car (a));
      item = gimp_item_get_by_id (id);

      if (item == NULL || ! GIMP_IS_DRAWABLE (item))
        {
          g_snprintf (error_str, sizeof (error_str),
                      "Invalid Drawable ID: %d", id);
          return script_error (sc, error_str, 0);
        }

      *drawable = GIMP_DRAWABLE (item);
    }

  a = sc->vptr->pair_cdr (a);
  operation_name = g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
  a = sc->vptr->pair_cdr (a);

  if (sc->vptr->list_length (sc, a) > 0)
    {
      if (sc->vptr->is_string (sc->vptr->pair_car (a)))
        filter_name = g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
      a = sc->vptr->pair_cdr (a);
    }

  *filter = gimp_drawable_filter_new (*drawable, operation_name, filter_name);
  g_free (filter_name);

  if (*filter == NULL)
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Unknown GEGL Operation: %s", operation_name);
      g_free (operation_name);
      return script_error (sc, error_str, 0);
    }
  g_free (operation_name);

  return script_fu_marshal_drawable_filter_configure (sc, a, proc_name, 5, *filter);
}

static pointer
script_fu_marshal_drawable_filter_configure_call (scheme  *sc,
                                                  pointer  a)
{
  GimpDrawableFilter *filter    = NULL;
  const gchar        *proc_name = "gimp-drawable-filter-configure";
  gchar               error_str[1024];

  if (sc->vptr->list_length (sc, a) < 3)
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Drawable Filter marshaller was called with missing arguments. "
                  "The filter ID, blend mode, opacity and the arguments' names "
                  "and values it requires (possibly none) must be specified: "
                  "(%s mode opacity arg1 val1 arg2 val2...)",
                  proc_name);
      return implementation_error (sc, error_str, 0);
    }
  else if (sc->vptr->list_length (sc, a) > 3 && sc->vptr->list_length (sc, a) % 2 != 1)
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Drawable Filter marshaller was called with an even number of arguments. "
                  "The drawable ID, the GEGL operation, filter name, blend mode, opacity "
                  "and the arguments' names and values it requires (possibly none) must be specified: "
                  "(%s mode opacity arg1 val1 arg2 val2...)",
                  proc_name);
      return implementation_error (sc, error_str, 0);
    }
  else if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
    {
      return script_type_error (sc, "numeric", 0, proc_name);
    }
  else
    {
      gint id;

      id     = sc->vptr->ivalue (sc->vptr->pair_car (a));
      filter = gimp_drawable_filter_get_by_id (id);

      if (filter == NULL || ! GIMP_IS_DRAWABLE_FILTER (filter))
        {
          g_snprintf (error_str, sizeof (error_str),
                      "Invalid Drawable Filter ID: %d", id);
          return script_error (sc, error_str, 0);
        }

      a = sc->vptr->pair_cdr (a);
    }

  return script_fu_marshal_drawable_filter_configure (sc, a, proc_name, 3, filter);
}

static pointer
script_fu_marshal_drawable_filter_set_aux_call (scheme  *sc,
                                                pointer  a)
{
  const gchar        *proc_name = "gimp-drawable-filter-set-aux-input";
  GimpDrawableFilter *filter    = NULL;
  GimpItem           *input     = NULL;
  gchar              *pad_name  = NULL;
  gint                id;
  gchar               error_str[1024];

  if (sc->vptr->list_length (sc, a) != 3)
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Drawable Filter marshaller was called with missing arguments. "
                  "The filter ID, aux pad name and the aux drawable: "
                  "(%s filter-id pad-name drawable-id)",
                  proc_name);
      return implementation_error (sc, error_str, 0);
    }

  if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
     return script_type_error (sc, "numeric", 0, proc_name);

  id     = sc->vptr->ivalue (sc->vptr->pair_car (a));
  filter = gimp_drawable_filter_get_by_id (id);
  if (filter == NULL || ! GIMP_IS_DRAWABLE_FILTER (filter))
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid Drawable Filter ID: %d", id);
      return script_error (sc, error_str, 0);
    }
  a = sc->vptr->pair_cdr (a);

  if (! sc->vptr->is_string (sc->vptr->pair_car (a)))
    return script_type_error (sc, "string", 1, proc_name);

  pad_name = g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
  a = sc->vptr->pair_cdr (a);

  if (! sc->vptr->is_number (sc->vptr->pair_car (a)))
     return script_type_error (sc, "numeric", 2, proc_name);

  id    = sc->vptr->ivalue (sc->vptr->pair_car (a));
  input = gimp_item_get_by_id (id);
  if (input == NULL || ! GIMP_IS_DRAWABLE (input))
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid Drawable ID: %d", id);
      return script_error (sc, error_str, 0);
    }

  gimp_drawable_filter_set_aux_input (filter, pad_name, GIMP_DRAWABLE (input));

  return sc->NIL;
}

static pointer
script_fu_marshal_drawable_merge_filter_call (scheme  *sc,
                                              pointer  a)
{
  const gchar        *proc_name = "gimp-drawable-merge-filter";
  GimpItem           *item;
  GimpDrawableFilter *filter;
  gint                id;
  gchar               error_str[1024];

  if (sc->vptr->list_length (sc, a) != 2)
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Drawable Filter marshaller was called with missing arguments. "
                  "The drawable and filter IDs are required: "
                  "(%s mode opacity arg1 val1 arg2 val2...)",
                  proc_name);

      return implementation_error (sc, error_str, 0);
    }

  id   = sc->vptr->ivalue (sc->vptr->pair_car (a));
  item = gimp_item_get_by_id (id);
  if (item == NULL || ! GIMP_IS_DRAWABLE (item))
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid Drawable ID: %d", id);
      return script_error (sc, error_str, 0);
    }
  a = sc->vptr->pair_cdr (a);

  id     = sc->vptr->ivalue (sc->vptr->pair_car (a));
  filter = gimp_drawable_filter_get_by_id (id);
  if (filter == NULL || ! GIMP_IS_DRAWABLE_FILTER (filter))
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid Drawable Filter ID: %d", id);
      return script_error (sc, error_str, 0);
    }

  gimp_drawable_merge_filter (GIMP_DRAWABLE (item), filter);

  return sc->NIL;
}

static pointer
script_fu_marshal_drawable_append_filter_call (scheme  *sc,
                                               pointer  a)
{
  const gchar        *proc_name = "gimp-drawable-append-filter";
  GimpItem           *item;
  GimpDrawableFilter *filter;
  gint                id;
  gchar               error_str[1024];

  if (sc->vptr->list_length (sc, a) != 2)
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Drawable Filter marshaller was called with missing arguments. "
                  "The drawable and filter IDs are required: "
                  "(%s mode opacity arg1 val1 arg2 val2...)",
                  proc_name);

      return implementation_error (sc, error_str, 0);
    }

  id   = sc->vptr->ivalue (sc->vptr->pair_car (a));
  item = gimp_item_get_by_id (id);
  if (item == NULL || ! GIMP_IS_DRAWABLE (item))
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid Drawable ID: %d", id);
      return script_error (sc, error_str, 0);
    }
  a = sc->vptr->pair_cdr (a);

  id     = sc->vptr->ivalue (sc->vptr->pair_car (a));
  filter = gimp_drawable_filter_get_by_id (id);
  if (filter == NULL || ! GIMP_IS_DRAWABLE_FILTER (filter))
    {
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid Drawable Filter ID: %d", id);
      return script_error (sc, error_str, 0);
    }

  gimp_drawable_append_filter (GIMP_DRAWABLE (item), filter);

  return sc->NIL;
}

static pointer
script_fu_marshal_drawable_merge_new_filter_call (scheme  *sc,
                                                  pointer  a)
{
  GimpDrawable       *drawable = NULL;
  GimpDrawableFilter *filter   = NULL;
  pointer             return_val;

  return_val = script_fu_marshal_drawable_create_filter (sc, a,
                                                         "gimp-drawable-merge-new-filter",
                                                         &drawable, &filter);
  if (return_val != sc->NIL)
    return return_val;

  gimp_drawable_merge_filter (drawable, filter);

  return sc->NIL;
}

static pointer
script_fu_marshal_drawable_append_new_filter_call (scheme  *sc,
                                                   pointer  a)
{
  GimpDrawable       *drawable    = NULL;
  GimpDrawableFilter *filter      = NULL;
  pointer             return_val;

  return_val = script_fu_marshal_drawable_create_filter (sc, a,
                                                         "gimp-drawable-append-new-filter",
                                                         &drawable, &filter);
  if (return_val != sc->NIL)
    return return_val;

  gimp_drawable_append_filter (drawable, filter);

  return sc->vptr->mk_integer (sc, gimp_drawable_filter_get_id (filter));
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
script_fu_register_call_procedure (scheme  *sc,
                                 pointer  a)
{
  /* Internally "regular" means general "procedure" */
  return script_fu_add_script_regular (sc, a);
}

static pointer
script_fu_menu_register_call (scheme  *sc,
                              pointer  a)
{
  return script_fu_add_menu (sc, a);
}

static pointer
script_fu_register_i18n_call (scheme  *sc,
                              pointer  a)
{
  return script_fu_add_i18n (sc, a);
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
