/* GIMP - The GNU Image Manipulation Program
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

#if 0
#define DEBUG_MARSHALL       0  /* No need to define this until you need it */
#define DEBUG_SCRIPTS        0
#endif

#include "config.h"

#include <string.h> /* memcpy, strcpy, strlen */

#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include "libgimp/gimp.h"

#include "tinyscheme/scheme-private.h"
#if USE_DL
#include "tinyscheme/dynload.h"
#endif
#include "ftx/ftx.h"
#include "re/re.h"

#include "script-fu-types.h"

#include "script-fu-console.h"
#include "script-fu-interface.h"
#include "script-fu-scripts.h"
#include "script-fu-server.h"

#include "scheme-wrapper.h"

static int   ts_console_mode;

SCHEME_EXPORT void *ts_output_routine;

#undef cons

struct
named_constant {
    const char *name;
    int value;
};

struct named_constant const script_constants[] =
{
  /* Useful values from libgimpbase/gimplimits.h */
  { "MIN-IMAGE-SIZE", GIMP_MIN_IMAGE_SIZE },
  { "MAX-IMAGE-SIZE", GIMP_MAX_IMAGE_SIZE },
  { "MIN-RESOLUTION", GIMP_MIN_RESOLUTION },
  { "MAX-RESOLUTION", GIMP_MAX_RESOLUTION },

  /* Useful misc stuff */
  { "TRUE",           TRUE  },
  { "FALSE",          FALSE },

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

/* The following constants are deprecated. They are */
/* included to keep backwards compatability with    */
/* older scripts used with version 2.0 of GIMP.     */
struct named_constant const old_constants[] =
{
  { "NORMAL",       GIMP_NORMAL_MODE       },
  { "DISSOLVE",     GIMP_DISSOLVE_MODE     },
  { "BEHIND",       GIMP_BEHIND_MODE       },
  { "MULTIPLY",     GIMP_MULTIPLY_MODE     },
  { "SCREEN",       GIMP_SCREEN_MODE       },
  { "OVERLAY",      GIMP_OVERLAY_MODE      },
  { "DIFFERENCE",   GIMP_DIFFERENCE_MODE   },
  { "ADDITION",     GIMP_ADDITION_MODE     },
  { "SUBTRACT",     GIMP_SUBTRACT_MODE     },
  { "DARKEN-ONLY",  GIMP_DARKEN_ONLY_MODE  },
  { "LIGHTEN-ONLY", GIMP_LIGHTEN_ONLY_MODE },
  { "HUE",          GIMP_HUE_MODE          },
  { "SATURATION",   GIMP_SATURATION_MODE   },
  { "COLOR",        GIMP_COLOR_MODE        },
  { "VALUE",        GIMP_VALUE_MODE        },
  { "DIVIDE",       GIMP_DIVIDE_MODE       },

  { "BLUR",         GIMP_BLUR_CONVOLVE     },
  { "SHARPEN",      GIMP_SHARPEN_CONVOLVE  },

  { "WHITE-MASK",     GIMP_ADD_WHITE_MASK     },
  { "BLACK-MASK",     GIMP_ADD_BLACK_MASK     },
  { "ALPHA-MASK",     GIMP_ADD_ALPHA_MASK     },
  { "SELECTION-MASK", GIMP_ADD_SELECTION_MASK },
  { "COPY-MASK",      GIMP_ADD_COPY_MASK      },

  { "ADD",          GIMP_CHANNEL_OP_ADD       },
  { "SUB",          GIMP_CHANNEL_OP_SUBTRACT  },
  { "REPLACE",      GIMP_CHANNEL_OP_REPLACE   },
  { "INTERSECT",    GIMP_CHANNEL_OP_INTERSECT },

  { "FG-BG-RGB",    GIMP_FG_BG_RGB_MODE       },
  { "FG-BG-HSV",    GIMP_FG_BG_HSV_MODE       },
  { "FG-TRANS",     GIMP_FG_TRANSPARENT_MODE  },
  { "CUSTOM",       GIMP_CUSTOM_MODE          },

  { "FG-IMAGE-FILL",    GIMP_FOREGROUND_FILL  },
  { "BG-IMAGE-FILL",    GIMP_BACKGROUND_FILL  },
  { "WHITE-IMAGE-FILL", GIMP_WHITE_FILL       },
  { "TRANS-IMAGE-FILL", GIMP_TRANSPARENT_FILL },

  { "APPLY",        GIMP_MASK_APPLY   },
  { "DISCARD",      GIMP_MASK_DISCARD },

  { "HARD",         GIMP_BRUSH_HARD },
  { "SOFT",         GIMP_BRUSH_SOFT },

  { "CONTINUOUS",   GIMP_PAINT_CONSTANT    },
  { "INCREMENTAL",  GIMP_PAINT_INCREMENTAL },

  { "HORIZONTAL",   GIMP_ORIENTATION_HORIZONTAL },
  { "VERTICAL",     GIMP_ORIENTATION_VERTICAL   },
  { "UNKNOWN",      GIMP_ORIENTATION_UNKNOWN    },

  { "LINEAR",               GIMP_GRADIENT_LINEAR               },
  { "BILINEAR",             GIMP_GRADIENT_BILINEAR             },
  { "RADIAL",               GIMP_GRADIENT_RADIAL               },
  { "SQUARE",               GIMP_GRADIENT_SQUARE               },
  { "CONICAL-SYMMETRIC",    GIMP_GRADIENT_CONICAL_SYMMETRIC    },
  { "CONICAL-ASYMMETRIC",   GIMP_GRADIENT_CONICAL_ASYMMETRIC   },
  { "SHAPEBURST-ANGULAR",   GIMP_GRADIENT_SHAPEBURST_ANGULAR   },
  { "SHAPEBURST-SPHERICAL", GIMP_GRADIENT_SHAPEBURST_SPHERICAL },
  { "SHAPEBURST-DIMPLED",   GIMP_GRADIENT_SHAPEBURST_DIMPLED   },
  { "SPIRAL-CLOCKWISE",     GIMP_GRADIENT_SPIRAL_CLOCKWISE     },
  { "SPIRAL-ANTICLOCKWISE", GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE },

  { "VALUE-LUT",      GIMP_HISTOGRAM_VALUE },
  { "RED-LUT",        GIMP_HISTOGRAM_RED   },
  { "GREEN-LUT",      GIMP_HISTOGRAM_GREEN },
  { "BLUE-LUT",       GIMP_HISTOGRAM_BLUE  },
  { "ALPHA-LUT",      GIMP_HISTOGRAM_ALPHA },

  { NULL, 0 }
};


static scheme sc;
static FILE  *ts_output;


/* wrapper functions */
FILE *
ts_get_output_file (void)
{
  return ts_output;
}

void
ts_set_output_file (FILE *file)
{
  scheme_set_output_port_file (&sc, file);
  ts_output = file;
}

void
ts_set_console_mode (int flag)
{
  ts_console_mode = flag;
}

void
ts_set_print_flag (gint print_flag)
{
  sc.print_output = print_flag;
}

void
ts_print_welcome (void)
{
  fprintf (ts_output, "Welcome to TinyScheme, Version 1.38\n");
  fprintf (ts_output, "Copyright (c) Dimitrios Souflis\n");
}

gint
ts_interpret_string (const gchar *expr)
{
  port *pt=sc.outport->_object._port;

  memset(sc.linebuff, '\0', LINESIZE);
  pt->rep.string.curr = sc.linebuff;
  /* Somewhere 'past_the_end' gets altered so it needs to be reset ~~~~~ */
  pt->rep.string.past_the_end = &sc.linebuff[LINESIZE-1];

#if DEBUG_SCRIPTS
  sc.print_output = 1;
  sc.tracing = 1;
#endif

  sc.vptr->load_string (&sc, (char *)expr);

  return sc.retcode;
}

const char *
ts_get_error_msg (void)
{
  return sc.linebuff;
}

const gchar *
ts_get_success_msg (void)
{
  if (sc.vptr->is_string(sc.value)) {
    return sc.vptr->string_value(sc.value);
  }
  else
    return "Success";
}

void
ts_output_string (FILE *fp, char *string, int len)
{
  g_return_if_fail (len >= 0);

  if (len > 0 && ts_console_mode && fp == stdout)
  {
    /* len is the number of UTF-8 characters; we need the number of bytes */
    len = g_utf8_offset_to_pointer (string, len) - string;

    script_fu_output_to_console (string, len);
  }
}


static void  init_constants   (void);
static void  init_procedures  (void);

static gboolean register_scripts = FALSE;

void
tinyscheme_init (const gchar *path,
                 gboolean     local_register_scripts)
{
  register_scripts = local_register_scripts;
  ts_output_routine = ts_output_string;

  /* init the interpreter */
  if (!scheme_init (&sc))
    {
      g_message ("Could not initialize TinyScheme!");
      return;
    }

  scheme_set_input_port_file (&sc, stdin);
  ts_set_output_file (stdout);

  /* Initialize the TinyScheme extensions */
  init_ftx(&sc);
  init_re(&sc);

  /* register in the interpreter the gimp functions and types. */
  init_constants ();
  init_procedures ();

  if (path)
    {
      GList *dir_list = gimp_path_parse (path, 16, TRUE, NULL);
      GList *list;

      for (list = dir_list; list; list = g_list_next (list))
        {
          gchar *filename = g_build_filename (list->data,
                                              "script-fu.init", NULL);
          FILE  *fin      = g_fopen (filename, "rb");

          g_free (filename);

          if (fin)
            {
              scheme_load_file (&sc, fin);
              fclose (fin);

              /*  To improve compatibility with older Script-Fu scripts,
               *  load script-fu-compat.init from the same directory.
               */
              filename = g_build_filename (list->data,
                                           "script-fu-compat.init", NULL);
              fin = g_fopen (filename, "rb");
              g_free (filename);

              if (fin)
                {
                  scheme_load_file (&sc, fin);
                  fclose (fin);
                }

              break;
            }
        }

      if (list == NULL)
        g_printerr ("Unable to read initialization file script-fu.init\n");

      gimp_path_free (dir_list);
    }
}

void
tinyscheme_deinit (void)
{
  scheme_deinit (&sc);
}

static void     convert_string               (gchar  *str);
static pointer  marshall_proc_db_call        (scheme *sc, pointer  a);
static pointer  script_fu_register_call      (scheme *sc, pointer  a);
static pointer  script_fu_menu_register_call (scheme *sc, pointer  a);
static pointer  script_fu_quit_call          (scheme *sc, pointer  a);


/*
 * Below can be found the functions responsible for registering the
 * gimp functions and types against the scheme interpreter.
 */


static void
init_constants (void)
{
  const gchar **enum_type_names;
  gint          n_enum_type_names;
  gint          i;
  GimpUnit      unit;
  pointer       symbol;

  symbol = sc.vptr->mk_symbol (&sc, "gimp-directory");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                sc.vptr->mk_string (&sc, gimp_directory () ));
  sc.vptr->setimmutable(symbol);

  symbol = sc.vptr->mk_symbol (&sc, "gimp-data-directory");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                sc.vptr->mk_string (&sc, gimp_data_directory () ));
  sc.vptr->setimmutable(symbol);

  symbol = sc.vptr->mk_symbol (&sc, "gimp-plug-in-directory");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                sc.vptr->mk_string (&sc, gimp_plug_in_directory () ));
  sc.vptr->setimmutable(symbol);

  symbol = sc.vptr->mk_symbol (&sc, "gimp-locale-directory");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                sc.vptr->mk_string (&sc, gimp_locale_directory () ));
  sc.vptr->setimmutable(symbol);

  symbol = sc.vptr->mk_symbol (&sc, "gimp-sysconf-directory");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                sc.vptr->mk_string (&sc, gimp_sysconf_directory () ));
  sc.vptr->setimmutable(symbol);

  enum_type_names = gimp_enums_get_type_names (&n_enum_type_names);

  for (i = 0; i < n_enum_type_names; i++)
    {
      const gchar *enum_name  = enum_type_names[i];
      GType        enum_type  = g_type_from_name (enum_name);
      GEnumClass  *enum_class = g_type_class_ref (enum_type);
      GEnumValue  *value;

      for (value = enum_class->values; value->value_name; value++)
        {
          if (g_str_has_prefix (value->value_name, "GIMP_"))
            {
              gchar *scheme_name;

              scheme_name = g_strdup (value->value_name + strlen ("GIMP_"));
              convert_string (scheme_name);

              symbol = sc.vptr->mk_symbol (&sc, scheme_name);
              sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                                      sc.vptr->mk_integer (&sc, value->value));
              sc.vptr->setimmutable(symbol);

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

      /* FIXME: gimp_unit_get_singular() returns a translated string */
      tmp = g_ascii_strup (gimp_unit_get_singular (unit), -1);
      scheme_name = g_strconcat ("UNIT-", tmp, NULL);
      g_free (tmp);

      symbol = sc.vptr->mk_symbol (&sc, scheme_name);
      sc.vptr->scheme_define (&sc, sc.global_env, symbol,
            sc.vptr->mk_integer (&sc, unit));
      sc.vptr->setimmutable(symbol);

      g_free (scheme_name);
    }

  /* Constants used in the register block of scripts */
  for (i = 0; script_constants[i].name != NULL; ++i)
    {
      symbol = sc.vptr->mk_symbol (&sc, script_constants[i].name);
      sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                    sc.vptr->mk_integer (&sc, script_constants[i].value));
      sc.vptr->setimmutable(symbol);
    }

  /* Define string constant for use in building paths to files/directories */
  symbol = sc.vptr->mk_symbol (&sc, "DIR-SEPARATOR");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                sc.vptr->mk_string (&sc, G_DIR_SEPARATOR_S));
  sc.vptr->setimmutable(symbol);

  /* These constants are deprecated and will be removed at a later date. */
  symbol = sc.vptr->mk_symbol (&sc, "gimp-dir");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                sc.vptr->mk_string (&sc, gimp_directory () ));
  sc.vptr->setimmutable(symbol);

  symbol = sc.vptr->mk_symbol (&sc, "gimp-data-dir");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                sc.vptr->mk_string (&sc, gimp_data_directory () ));
  sc.vptr->setimmutable(symbol);

  symbol = sc.vptr->mk_symbol (&sc, "gimp-plugin-dir");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                sc.vptr->mk_string (&sc, gimp_plug_in_directory () ));
  sc.vptr->setimmutable(symbol);

  for (i = 0; old_constants[i].name != NULL; ++i)
    {
      symbol = sc.vptr->mk_symbol (&sc, old_constants[i].name);
      sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                    sc.vptr->mk_integer (&sc, old_constants[i].value));
      sc.vptr->setimmutable(symbol);
    }
}

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
  gchar           *buff;
  pointer          symbol;

#if USE_DL
  symbol = sc.vptr->mk_symbol (&sc,"load-extension");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                  sc.vptr->mk_foreign_func (&sc, scm_load_ext));
  sc.vptr->setimmutable(symbol);
#endif

  symbol = sc.vptr->mk_symbol (&sc, "script-fu-register");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                  sc.vptr->mk_foreign_func (&sc, script_fu_register_call));
  sc.vptr->setimmutable(symbol);

  symbol = sc.vptr->mk_symbol (&sc, "script-fu-menu-register");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                  sc.vptr->mk_foreign_func (&sc, script_fu_menu_register_call));
  sc.vptr->setimmutable(symbol);

  symbol = sc.vptr->mk_symbol (&sc, "script-fu-quit");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                  sc.vptr->mk_foreign_func (&sc, script_fu_quit_call));
  sc.vptr->setimmutable(symbol);

  /*  register the database execution procedure  */
  symbol = sc.vptr->mk_symbol (&sc, "gimp-proc-db-call");
  sc.vptr->scheme_define (&sc, sc.global_env, symbol,
                  sc.vptr->mk_foreign_func (&sc, marshall_proc_db_call));
  sc.vptr->setimmutable(symbol);

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
         /* Build a define that will call the foreign function */
         /* The Scheme statement was suggested by Simon Budig  */
         if (nparams == 0)
           {
             buff = g_strdup_printf (
                      " (define (%s) (gimp-proc-db-call \"%s\"))",
                      proc_list[i], proc_list[i]);
           }
         else
           {
             buff = g_strdup_printf (
                      " (define %s (lambda x (apply gimp-proc-db-call (cons \"%s\" x))))",
                      proc_list[i], proc_list[i]);
           }

         /*  Execute the 'define'  */
         sc.vptr->load_string (&sc, buff);

         g_free (buff);

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
convert_string (gchar *str)
{
  while (*str)
    {
      if (*str == '_') *str = '-';
      str++;
    }
}

static pointer
my_err (char *msg, pointer a)
{
  if (ts_console_mode)
    {
      gchar *tmp = g_strdup_printf ("Error: %s\n", msg);

      script_fu_output_to_console (tmp, -1);

      g_free (tmp);
    }
  else
    {
      g_message (msg);
    }

  return sc.NIL;
}


/* This is called by the Scheme interpreter to allow calls to GIMP functions */
static pointer
marshall_proc_db_call (scheme *sc, pointer a)
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
  gint             j;
  gint             success = TRUE;
  pointer          intermediate_val;
  pointer          return_val = sc->NIL;
  gchar           *string;
  gint32           n_elements;
  pointer          vector;
#if DEBUG_MARSHALL
char *ret_types[] = {
  "GIMP_PDB_INT32",       "GIMP_PDB_INT16",     "GIMP_PDB_INT8",
  "GIMP_PDB_FLOAT",       "GIMP_PDB_STRING",    "GIMP_PDB_INT32ARRAY",
  "GIMP_PDB_INT16ARRAY",  "GIMP_PDB_INT8ARRAY", "GIMP_PDB_FLOATARRAY",
  "GIMP_PDB_STRINGARRAY", "GIMP_PDB_COLOR",     "GIMP_PDB_REGION",
  "GIMP_PDB_DISPLAY",     "GIMP_PDB_IMAGE",     "GIMP_PDB_LAYER",
  "GIMP_PDB_CHANNEL",     "GIMP_PDB_DRAWABLE",  "GIMP_PDB_SELECTION",
  "GIMP_PDB_BOUNDARY",    "GIMP_PDB_VECTORS",   "GIMP_PDB_PARASITE",
  "GIMP_PDB_STATUS",      "GIMP_PDB_END"
};

char *ts_types[] = {
  "T_NONE",
  "T_STRING",    "T_NUMBER",     "T_SYMBOL",       "T_PROC",
  "T_PAIR",      "T_CLOSURE",    "T_CONTINUATION", "T_FOREIGN",
  "T_CHARACTER", "T_PORT",       "T_VECTOR",       "T_MACRO",
  "T_PROMISE",   "T_ENVIRONMENT","T_ARRAY"
};

char *status_types[] = {
  "GIMP_PDB_EXECUTION_ERROR", "GIMP_PDB_CALLING_ERROR",
  "GIMP_PDB_PASS_THROUGH",    "GIMP_PDB_SUCCESS",
  "GIMP_PDB_CANCEL"
};

g_printerr ("\nIn marshall_proc_db_call ()\n");
#endif

  /*  Make sure there are arguments  */
  if (a == sc->NIL)
    return my_err ("Procedure database argument marshaller was called with no arguments. "
                   "The procedure to be executed and the arguments it requires "
                   " (possibly none) must be specified.", sc->NIL);

  /*  The PDB procedure name is the argument or first argument of the list  */
  if (sc->vptr->is_pair (a))
    proc_name = g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
  else
    proc_name = g_strdup (sc->vptr->string_value (a));

#ifdef DEBUG_MARSHALL
g_printerr ("  proc name: %s\n", proc_name);
#endif
#if DEBUG_MARSHALL
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
      return my_err (error_str, sc->NIL);
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
  if ( (sc->vptr->list_length (sc, a) - 1) != nparams)
    {
#if DEBUG_MARSHALL
g_printerr ("  Invalid number of arguments (expected %d but received %d)",
                 nparams, (sc->vptr->list_length (sc, a) - 1));
#endif
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid number of arguments for %s (expected %d but received %d)",
                  proc_name, nparams, (sc->vptr->list_length (sc, a) - 1));
      return my_err (error_str, sc->NIL);
    }

  /*  Marshall the supplied arguments  */
  if (nparams)
    args = g_new (GimpParam, nparams);
  else
    args = NULL;

  for (i = 0; i < nparams; i++)
    {
      a = sc->vptr->pair_cdr (a);

#if DEBUG_MARSHALL
g_printerr ("    param %d - expecting type %s (%d)\n",
                 i+1, ret_types[ params[i].type ], params[i].type);
g_printerr ("      passed arg is type %s (%d)\n",
                 ts_types[ type(sc->vptr->pair_car (a)) ],
                 type(sc->vptr->pair_car (a)));
#endif

      args[i].type = params[i].type;

      switch (params[i].type)
        {
        case GIMP_PDB_INT32:
        case GIMP_PDB_DISPLAY:
        case GIMP_PDB_IMAGE:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_BOUNDARY:
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
              if (n_elements < 0 || n_elements > sc->vptr->vector_length (vector))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "INT32 vector (argument %d) for function %s has "
                              "size of %ld but expected size of %d",
                              i+1, proc_name, sc->vptr->vector_length (vector), n_elements);
                  return my_err (error_str, sc->NIL);
                }

              /* FIXME: Check that g_new returned non-NULL value. */
              args[i].data.d_int32array = g_new (gint32, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);

                  /* FIXME: Check values in vector stay within range for each type. */
                  if (!sc->vptr->is_integer (v_element))
                    {
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not INT32 (argument %d for function %s)\n",
                                  j+1, i+1, proc_name);
                      return my_err (error_str, vector);
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
                  return my_err (error_str, sc->NIL);
                }

              args[i].data.d_int16array = g_new (gint16, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);

                  if (!sc->vptr->is_integer (v_element))
                    {
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not INT16 (argument %d for function %s)\n",
                                  j+1, i+1, proc_name);
                      return my_err (error_str, vector);
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
              if (n_elements < 0 || n_elements > sc->vptr->vector_length (vector))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "INT8 vector (argument %d) for function %s has "
                              "size of %ld but expected size of %d",
                              i+1, proc_name, sc->vptr->vector_length (vector), n_elements);
                  return my_err (error_str, sc->NIL);
                }

              args[i].data.d_int8array = g_new (guint8, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);

                  if (!sc->vptr->is_integer (v_element))
                    {
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not INT8 (argument %d for function %s)\n",
                                  j+1, i+1, proc_name);
                      return my_err (error_str, vector);
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
              if (n_elements < 0 || n_elements > sc->vptr->vector_length (vector))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "FLOAT vector (argument %d) for function %s has "
                              "size of %ld but expected size of %d",
                              i+1, proc_name, sc->vptr->vector_length (vector), n_elements);
                  return my_err (error_str, sc->NIL);
                }

              args[i].data.d_floatarray = g_new (gdouble, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);

                  if (!sc->vptr->is_number (v_element))
                    {
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not FLOAT (argument %d for function %s)\n",
                                  j+1, i+1, proc_name);
                      return my_err (error_str, vector);
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
          vector = sc->vptr->pair_car (a);
          if (!sc->vptr->is_vector (vector))
            success = FALSE;
          if (success)
            {
              n_elements = args[i-1].data.d_int32;
              if (n_elements < 0 || n_elements > sc->vptr->vector_length (vector))
                {
                  g_snprintf (error_str, sizeof (error_str),
                              "STRING vector (argument %d) for function %s has "
                              "length of %ld but expected length of %d",
                              i+1, proc_name, sc->vptr->vector_length (vector), n_elements);
                  return my_err (error_str, sc->NIL);
                }

              args[i].data.d_stringarray = g_new (gchar *, n_elements);

              for (j = 0; j < n_elements; j++)
                {
                  pointer v_element = sc->vptr->vector_elem (vector, j);

                  if (!sc->vptr->is_string (v_element))
                    {
                      g_snprintf (error_str, sizeof (error_str),
                                  "Item %d in vector is not STRING (argument %d for function %s)\n",
                                  j+1, i+1, proc_name);
                      return my_err (error_str, vector);
                    }

                  args[i].data.d_stringarray[j] =
                      (gchar *) sc->vptr->string_value (v_element);
                }

#if DEBUG_MARSHALL
{
glong count = sc->vptr->vector_length (vector);
g_printerr ("      string vector has %ld elements\n", count);
if (count > 0)
  {
    g_printerr ("     ");
    for (j = 0; j < count; ++j)
      g_printerr (" \"%s\"",
               sc->vptr->string_value ( sc->vptr->vector_elem (vector, j) ));
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
              guchar  r, g, b;

              color_list = sc->vptr->pair_car (a);
              r = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)), 0, 255);
              color_list = sc->vptr->pair_cdr (color_list);
              g = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)), 0, 255);
              color_list = sc->vptr->pair_cdr (color_list);
              b = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)), 0, 255);

              gimp_rgba_set_uchar (&args[i].data.d_color, r, g, b, 255);
#if DEBUG_MARSHALL
g_printerr ("      (%d %d %d)\n", r, g, b);
#endif
            }
          else
            {
              success = FALSE;
            }
          break;

        case GIMP_PDB_REGION:
          if (! (sc->vptr->is_list (sc, sc->vptr->pair_car (a)) &&
            sc->vptr->list_length (sc, sc->vptr->pair_car (a)) == 4))
            success = FALSE;
          if (success)
            {
              pointer region;

              region = sc->vptr->pair_car (a);
              args[i].data.d_region.x =
                           sc->vptr->ivalue (sc->vptr->pair_car (region));
              region = sc->vptr->pair_cdr (region);
              args[i].data.d_region.y =
                           sc->vptr->ivalue (sc->vptr->pair_car (region));
              region = sc->vptr->pair_cdr (region);
              args[i].data.d_region.width =
                           sc->vptr->ivalue (sc->vptr->pair_car (region));
              region = sc->vptr->pair_cdr (region);
              args[i].data.d_region.height =
                           sc->vptr->ivalue (sc->vptr->pair_car (region));
#if DEBUG_MARSHALL
g_printerr ("      (%d %d %d %d)\n",
                 args[i].data.d_region.x,
                 args[i].data.d_region.y,
                 args[i].data.d_region.width,
                 args[i].data.d_region.height);
#endif
            }
          break;

        case GIMP_PDB_PARASITE:
          if (!sc->vptr->is_list (sc, sc->vptr->pair_car (a)) ||
              sc->vptr->list_length (sc, sc->vptr->pair_car (a)) != 3)
            success = FALSE;
          if (success)
            {
              /* parasite->name */
              intermediate_val = sc->vptr->pair_car (a);

              if (!sc->vptr->is_string (sc->vptr->pair_car (intermediate_val)))
                {
                  success = FALSE;
                  break;
                }

              args[i].data.d_parasite.name =
                sc->vptr->string_value (sc->vptr->pair_car (intermediate_val));
#if DEBUG_MARSHALL
g_printerr ("      name '%s'\n", args[i].data.d_parasite.name);
#endif

              /* parasite->flags */
              intermediate_val = sc->vptr->pair_cdr (intermediate_val);

              if (!sc->vptr->is_number (sc->vptr->pair_car (intermediate_val)))
                {
                  success = FALSE;
                  break;
                }

              args[i].data.d_parasite.flags =
                sc->vptr->ivalue (sc->vptr->pair_car (intermediate_val));
#if DEBUG_MARSHALL
g_printerr ("      flags %d", args[i].data.d_parasite.flags);
#endif

              /* parasite->data */
              intermediate_val = sc->vptr->pair_cdr (intermediate_val);

              if (!sc->vptr->is_string (sc->vptr->pair_car (intermediate_val)))
                {
                  success = FALSE;
                  break;
                }

              args[i].data.d_parasite.size =
                sc->vptr->ivalue (sc->vptr->pair_car (intermediate_val));
              args[i].data.d_parasite.data =
                sc->vptr->string_value (sc->vptr->pair_car (intermediate_val));
#if DEBUG_MARSHALL
g_printerr (", size %d\n", args[i].data.d_parasite.size);
g_printerr ("      data '%s'\n", (char *)args[i].data.d_parasite.data);
#endif
            }
          break;

        case GIMP_PDB_STATUS:
          return my_err ("Status is for return types, not arguments",
                         sc->vptr->pair_car (a));
          break;

        default:
          g_snprintf (error_str, sizeof (error_str),
                      "Argument %d for %s is an unknown type",
                      i+1, proc_name);
          return my_err (error_str, sc->NIL);
        }

      /* Break out of loop before i gets updated when error was detected */
      if (! success)
        break;
    }

  if (success)
#if DEBUG_MARSHALL
{
g_printerr ("    calling %s...", proc_name);
#endif
    values = gimp_run_procedure2 (proc_name, &nvalues, nparams, args);
#if DEBUG_MARSHALL
g_printerr ("  done.\n");
}
#endif
  else
    {
#if DEBUG_MARSHALL
g_printerr ("  Invalid type for argument %d\n", i+1);
#endif
      g_snprintf (error_str, sizeof (error_str),
                  "Invalid type for argument %d to %s",
                  i+1, proc_name);
      return my_err (error_str, sc->NIL);
    }

  /*  Check the return status  */
  if (! values)
    {
#if DEBUG_MARSHALL
g_printerr ("  Did not return status\n");
#endif
      g_snprintf (error_str, sizeof(error_str),
               "Procedural database execution of %s did not return a status:\n    ",
               proc_name);

      return my_err (error_str, sc->NIL);
    }

#if DEBUG_MARSHALL
g_printerr ("    return value is %s\n",
                 status_types[ values[0].data.d_status ]);
#endif

  switch (values[0].data.d_status)
    {
    case GIMP_PDB_EXECUTION_ERROR:
      g_snprintf (error_str, sizeof (error_str),
              "Procedural database execution of %s failed:\n    ",
              proc_name);
      return my_err (error_str, sc->NIL);
      break;

    case GIMP_PDB_CALLING_ERROR:
      g_snprintf (error_str, sizeof (error_str),
               "Procedural database execution of %s failed on invalid input arguments:\n    ",
               proc_name);
      return my_err (error_str, sc->NIL);
      break;

    case GIMP_PDB_SUCCESS:
#if DEBUG_MARSHALL
g_printerr ("    values returned: %d\n", nvalues-1);
#endif
      for (i = nvalues - 2; i >= 0; --i)
        {
#if DEBUG_MARSHALL
g_printerr ("      value %d is type %s (%d)\n",
                 i, ret_types[ return_vals[i].type ], return_vals[i].type);
#endif
          switch (return_vals[i].type)
            {
            case GIMP_PDB_INT32:
            case GIMP_PDB_DISPLAY:
            case GIMP_PDB_IMAGE:
            case GIMP_PDB_LAYER:
            case GIMP_PDB_CHANNEL:
            case GIMP_PDB_DRAWABLE:
            case GIMP_PDB_SELECTION:
            case GIMP_PDB_BOUNDARY:
            case GIMP_PDB_VECTORS:
              return_val = sc->vptr->cons (sc,
                             sc->vptr->mk_integer (sc,
                                                   values[i + 1].data.d_int32),
                                                   return_val);
              set_safe_foreign (sc, return_val);
              break;

            case GIMP_PDB_INT16:
              return_val = sc->vptr->cons (sc,
                             sc->vptr->mk_integer (sc,
                                                   values[i + 1].data.d_int16),
                                                   return_val);
              set_safe_foreign (sc, return_val);
              break;

            case GIMP_PDB_INT8:
              return_val = sc->vptr->cons (sc,
                             sc->vptr->mk_integer (sc,
                                                   values[i + 1].data.d_int8),
                                                   return_val);
              set_safe_foreign (sc, return_val);
              break;

            case GIMP_PDB_FLOAT:
              return_val = sc->vptr->cons (sc,
                             sc->vptr->mk_real (sc,
                                                values[i + 1].data.d_float),
                                                return_val);
              set_safe_foreign (sc, return_val);
              break;

            case GIMP_PDB_STRING:
              string = values[i + 1].data.d_string;
              if (! string)
                string = "";
              return_val = sc->vptr->cons (sc,
                                           sc->vptr->mk_string (sc, string),
                                           return_val);
              set_safe_foreign (sc, return_val);
              break;

            case GIMP_PDB_INT32ARRAY:
              /*  integer arrays are always implemented such that the previous
               *  return value contains the number of strings in the array
               */
              {
                gint32  num_int32s = values[i].data.d_int32;
                gint32 *array      = (gint32 *) values[i + 1].data.d_int32array;
                pointer vector     = sc->vptr->mk_vector (sc, num_int32s);

                return_val = sc->vptr->cons (sc, vector, return_val);
                set_safe_foreign (sc, return_val);

                for (j = 0; j < num_int32s; j++)
                  {
                    sc->vptr->set_vector_elem (vector, j,
                                               sc->vptr->mk_integer (sc,
                                                                     array[j]));
                  }
              }
              break;

            case GIMP_PDB_INT16ARRAY:
              /*  integer arrays are always implemented such that the previous
               *  return value contains the number of strings in the array
               */
              {
                gint32  num_int16s = values[i].data.d_int32;
                gint16 *array      = (gint16 *) values[i + 1].data.d_int16array;
                pointer vector     = sc->vptr->mk_vector (sc, num_int16s);

                return_val = sc->vptr->cons (sc, vector, return_val);
                set_safe_foreign (sc, return_val);

                for (j = 0; j < num_int16s; j++)
                  {
                    sc->vptr->set_vector_elem (vector, j,
                                               sc->vptr->mk_integer (sc,
                                                                     array[j]));
                  }
              }
              break;

            case GIMP_PDB_INT8ARRAY:
              /*  integer arrays are always implemented such that the previous
               *  return value contains the number of strings in the array
               */
              {
                gint32  num_int8s  = values[i].data.d_int32;
                guint8 *array      = (guint8 *) values[i + 1].data.d_int8array;
                pointer vector     = sc->vptr->mk_vector (sc, num_int8s);

                return_val = sc->vptr->cons (sc, vector, return_val);
                set_safe_foreign (sc, return_val);

                for (j = 0; j < num_int8s; j++)
                  {
                    sc->vptr->set_vector_elem (vector, j,
                                               sc->vptr->mk_integer (sc,
                                                                     array[j]));
                  }
              }
              break;

            case GIMP_PDB_FLOATARRAY:
              /*  float arrays are always implemented such that the previous
               *  return value contains the number of strings in the array
               */
              {
                gint32   num_floats = values[i].data.d_int32;
                gdouble *array  = (gdouble *) values[i + 1].data.d_floatarray;
                pointer  vector = sc->vptr->mk_vector (sc, num_floats);

                return_val = sc->vptr->cons (sc, vector, return_val);
                set_safe_foreign (sc, return_val);

                for (j = 0; j < num_floats; j++)
                  {
                    sc->vptr->set_vector_elem (vector, j,
                                               sc->vptr->mk_real (sc,
                                                                  array[j]));
                  }
              }
              break;

            case GIMP_PDB_STRINGARRAY:
              /*  string arrays are always implemented such that the previous
               *  return value contains the number of strings in the array
               */
              {
                gint    num_strings = values[i].data.d_int32;
                gchar **array  = (gchar **) values[i + 1].data.d_stringarray;
                pointer vector = sc->vptr->mk_vector (sc, num_strings);

                return_val = sc->vptr->cons (sc, vector, return_val);
                set_safe_foreign (sc, return_val);

                for (j = 0; j < num_strings; j++)
                  {
                    sc->vptr->set_vector_elem (vector, j,
                                               sc->vptr->mk_string (sc,
                                                                    array[j]));
                  }
              }
              break;

            case GIMP_PDB_COLOR:
              {
                guchar r, g, b;

                gimp_rgb_get_uchar (&values[i + 1].data.d_color, &r, &g, &b);

                intermediate_val = sc->vptr->cons (sc,
                    sc->vptr->mk_integer (sc, r),
                    sc->vptr->cons (sc,
                        sc->vptr->mk_integer (sc, g),
                        sc->vptr->cons (sc,
                            sc->vptr->mk_integer (sc, b),
                            sc->NIL)));
                return_val = sc->vptr->cons (sc,
                                             intermediate_val,
                                             return_val);
                set_safe_foreign (sc, return_val);
                break;
              }

            case GIMP_PDB_REGION:
              {
                gint32 x, y, w, h;

                x = values[i + 1].data.d_region.x;
                y = values[i + 1].data.d_region.y;
                w = values[i + 1].data.d_region.width;
                h = values[i + 1].data.d_region.height;

                intermediate_val = sc->vptr->cons (sc,
                    sc->vptr->mk_integer (sc, x),
                    sc->vptr->cons (sc,
                        sc->vptr->mk_integer (sc, y),
                        sc->vptr->cons (sc,
                            sc->vptr->mk_integer (sc, w),
                            sc->vptr->cons (sc,
                                sc->vptr->mk_integer (sc, h),
                                sc->NIL))));
                return_val = sc->vptr->cons (sc,
                                             intermediate_val,
                                             return_val);
                set_safe_foreign (sc, return_val);
                break;
              }
              break;

            case GIMP_PDB_PARASITE:
              {
                if (values[i + 1].data.d_parasite.name == NULL)
                    return_val = my_err ("Error: null parasite", sc->NIL);
                else
                  {
                    /* don't move the mk_foo() calls outside this function call,
                     * otherwise they might be garbage collected away!  */
                    intermediate_val = sc->vptr->cons (sc,
                        sc->vptr->mk_string (sc,
                                             values[i + 1].data.d_parasite.name),
                        sc->vptr->cons (sc,
                            sc->vptr->mk_integer (sc,
                                                  values[i + 1].data.d_parasite.flags),
                            sc->vptr->cons (sc,
                                sc->vptr->mk_counted_string (sc,
                                                             values[i + 1].data.d_parasite.data,
                                                             values[i + 1].data.d_parasite.size),
                                 sc->NIL)));
                    return_val = sc->vptr->cons (sc,
                                                 intermediate_val,
                                                 return_val);
                    set_safe_foreign (sc, return_val);

#if DEBUG_MARSHALL
g_printerr ("      name '%s'\n", values[i+1].data.d_parasite.name);
g_printerr ("      flags %d", values[i+1].data.d_parasite.flags);
g_printerr (", size %d\n", values[i+1].data.d_parasite.size);
g_printerr ("      data '%.*s'\n",
                 values[i+1].data.d_parasite.size,
                 (char *)values[i+1].data.d_parasite.data);
#endif
                  }
              }
              break;

            case GIMP_PDB_STATUS:
              return my_err ("Procedural database execution returned multiple status values", sc->NIL);
              break;

            default:
              return my_err ("Unknown return type", sc->NIL);
            }
        }

    case GIMP_PDB_PASS_THROUGH:
    case GIMP_PDB_CANCEL:   /*  should we do something here?  */
      break;
    }

  /* If we have no return value(s) from PDB call, return */
  /* either TRUE or FALSE to indicate if call succeeded. */
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
  g_free (args);

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
script_fu_register_call (scheme *sc, pointer a)
{
  if (register_scripts)
    return script_fu_add_script (sc, a);
  else
    return sc->NIL;
}

static pointer
script_fu_menu_register_call (scheme *sc, pointer a)
{
  if (register_scripts)
    return script_fu_add_menu (sc, a);
  else
    return sc->NIL;
}

static pointer
script_fu_quit_call (scheme *sc, pointer a)
{
  script_fu_server_quit ();

  scheme_deinit (sc);

  return sc->NIL;
}
