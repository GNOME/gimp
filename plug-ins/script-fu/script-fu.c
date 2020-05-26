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

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "tinyscheme/scheme.h"

#include "script-fu-types.h"

#include "script-fu-console.h"
#include "script-fu-eval.h"
#include "script-fu-interface.h"
#include "script-fu-scripts.h"
#include "script-fu-server.h"
#include "script-fu-text-console.h"

#include "scheme-wrapper.h"

#include "script-fu-intl.h"


typedef struct _ScriptFu      ScriptFu;
typedef struct _ScriptFuClass ScriptFuClass;

struct _ScriptFu
{
  GimpPlugIn      parent_instance;
};

struct _ScriptFuClass
{
  GimpPlugInClass parent_class;
};


#define SCRIPT_FU_TYPE  (script_fu_get_type ())
#define SCRIPT_FU (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCRIPT_FU_TYPE, ScriptFu))

GType                   script_fu_get_type         (void) G_GNUC_CONST;

static GList          * script_fu_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * script_fu_create_procedure (GimpPlugIn           *plug_in,
                                                    const gchar          *name);

static GimpValueArray * script_fu_run              (GimpProcedure        *procedure,
                                                    const GimpValueArray *args,
                                                    gpointer              run_data);
static GList *          script_fu_search_path      (void);
static void             script_fu_extension_init   (GimpPlugIn           *plug_in);
static GimpValueArray * script_fu_refresh_proc     (GimpProcedure        *procedure,
                                                    const GimpValueArray *args,
                                                    gpointer              run_data);


G_DEFINE_TYPE (ScriptFu, script_fu, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SCRIPT_FU_TYPE)


static void
script_fu_class_init (ScriptFuClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = script_fu_query_procedures;
  plug_in_class->create_procedure = script_fu_create_procedure;
}

static void
script_fu_init (ScriptFu *script_fu)
{
}

static GList *
script_fu_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  gimp_plug_in_set_translation_domain (plug_in,
                                       GETTEXT_PACKAGE "-script-fu", NULL);

  list = g_list_append (list, g_strdup ("extension-script-fu"));
  list = g_list_append (list, g_strdup ("plug-in-script-fu-console"));
  list = g_list_append (list, g_strdup ("plug-in-script-fu-text-console"));
  list = g_list_append (list, g_strdup ("plug-in-script-fu-server"));
  list = g_list_append (list, g_strdup ("plug-in-script-fu-eval"));

  return list;
}

static GimpProcedure *
script_fu_create_procedure (GimpPlugIn  *plug_in,
                            const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, "extension-script-fu"))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_EXTENSION,
                                      script_fu_run, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "A scheme interpreter for scripting "
                                        "GIMP operations",
                                        "More help here later",
                                        NULL);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1997");
    }
  else if (! strcmp (name, "plug-in-script-fu-console"))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      script_fu_run, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("Script-Fu _Console"));
      gimp_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Development/Script-Fu");

      gimp_procedure_set_documentation (procedure,
                                        N_("Interactive console for Script-Fu "
                                           "development"),
                                        "Provides an interface which allows "
                                        "interactive scheme development.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1997");

      GIMP_PROC_ARG_ENUM (procedure, "run-mode",
                          "Run mode",
                          "The run mode",
                          GIMP_TYPE_RUN_MODE,
                          GIMP_RUN_INTERACTIVE,
                          G_PARAM_READWRITE);
    }
  else if (! strcmp (name, "plug-in-script-fu-text-console"))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      script_fu_run, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Provides a text console mode for "
                                        "script-fu development",
                                        "Provides an interface which allows "
                                        "interactive scheme development.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1997");

      GIMP_PROC_ARG_ENUM (procedure, "run-mode",
                          "Run mode",
                          "The run mode",
                          GIMP_TYPE_RUN_MODE,
                          GIMP_RUN_INTERACTIVE,
                          G_PARAM_READWRITE);
    }
  else if (! strcmp (name, "plug-in-script-fu-server"))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      script_fu_run, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("_Start Server..."));
      gimp_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Development/Script-Fu");

      gimp_procedure_set_documentation (procedure,
                                        N_("Server for remote Script-Fu "
                                           "operation"),
                                        "Provides a server for remote "
                                        "script-fu operation. NOTE that for "
                                        "security reasons this procedure's "
                                        "API was changed in an incompatible "
                                        "way since GIMP 2.8.12. You now have "
                                        "to pass the IP to listen on as "
                                        "first parameter. Calling this "
                                        "procedure with the old API will "
                                        "fail on purpose.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1997");

      GIMP_PROC_ARG_ENUM (procedure, "run-mode",
                          "Run mode",
                          "The run mode",
                          GIMP_TYPE_RUN_MODE,
                          GIMP_RUN_INTERACTIVE,
                          G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "ip",
                            "IP",
                            "The IP on which to listen for requests",
                            NULL,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "port",
                         "Port",
                         "The port on which to listen for requests",
                         0, G_MAXINT, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "logfile",
                            "Log File",
                            "The file to log activity to",
                            NULL,
                            G_PARAM_READWRITE);
    }
  else if (! strcmp (name, "plug-in-script-fu-eval"))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      script_fu_run, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Evaluate scheme code",
                                        "Evaluate the code under the scheme "
                                        "interpreter (primarily for batch mode)",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Manish Singh",
                                      "Manish Singh",
                                      "1998");

      GIMP_PROC_ARG_ENUM (procedure, "run-mode",
                          "Run mode",
                          "The run mode",
                          GIMP_TYPE_RUN_MODE,
                          GIMP_RUN_INTERACTIVE,
                          G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "code",
                            "Code",
                            "The code to run",
                            NULL,
                            G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
script_fu_run (GimpProcedure        *procedure,
               const GimpValueArray *args,
               gpointer              run_data)
{
  GimpPlugIn     *plug_in     = gimp_procedure_get_plug_in (procedure);
  const gchar    *name        = gimp_procedure_get_name (procedure);
  GimpValueArray *return_vals = NULL;
  GList          *path;

  INIT_I18N();

  path = script_fu_search_path ();

  /*  Determine before we allow scripts to register themselves
   *   whether this is the base, automatically installed script-fu extension
   */
  if (strcmp (name, "extension-script-fu") == 0)
    {
      /*  Setup auxiliary temporary procedures for the base extension  */
      script_fu_extension_init (plug_in);

      /*  Init the interpreter and register scripts */
      tinyscheme_init (path, TRUE);
    }
  else
    {
      /*  Init the interpreter  */
      tinyscheme_init (path, FALSE);
    }

  if (gimp_value_array_length (args) > 0)
    ts_set_run_mode (GIMP_VALUES_GET_ENUM (args, 0));

  /*  Load all of the available scripts  */
  script_fu_find_scripts (plug_in, path);

  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  if (strcmp (name, "extension-script-fu") == 0)
    {
      /*
       *  The main script-fu extension.
       */

      /*  Acknowledge that the extension is properly initialized  */
      gimp_procedure_extension_ready (procedure);

      /*  Go into an endless loop  */
      while (TRUE)
        gimp_plug_in_extension_process (plug_in, 0);
    }
  else if (strcmp (name, "plug-in-script-fu-text-console") == 0)
    {
      /*
       *  The script-fu text console for interactive Scheme development
       */

      return_vals = script_fu_text_console_run (procedure, args);
    }
  else if (strcmp (name, "plug-in-script-fu-console") == 0)
    {
      /*
       *  The script-fu console for interactive Scheme development
       */

      return_vals = script_fu_console_run (procedure, args);
    }
  else if (strcmp (name, "plug-in-script-fu-server") == 0)
    {
      /*
       *  The script-fu server for remote operation
       */

      return_vals = script_fu_server_run (procedure, args);
    }
  else if (strcmp (name, "plug-in-script-fu-eval") == 0)
    {
      /*
       *  A non-interactive "console" (for batch mode)
       */

      return_vals = script_fu_eval_run (procedure, args);
    }

  if (! return_vals)
    return_vals = gimp_procedure_new_return_values (procedure,
                                                    GIMP_PDB_SUCCESS,
                                                    NULL);

  return return_vals;
}

static GList *
script_fu_search_path (void)
{
  gchar *path_str;
  GList *path  = NULL;

  path_str = gimp_gimprc_query ("script-fu-path");

  if (path_str)
    {
      GError *error = NULL;

      path = gimp_config_path_expand_to_files (path_str, &error);
      g_free (path_str);

      if (! path)
        {
          g_warning ("Can't convert script-fu-path to filesystem encoding: %s",
                     error->message);
          g_clear_error (&error);
        }
    }

  return path;
}

static void
script_fu_extension_init (GimpPlugIn *plug_in)
{
  GimpProcedure *procedure;

  gimp_plug_in_add_menu_branch (plug_in, "<Image>/Help", N_("_GIMP Online"));
  gimp_plug_in_add_menu_branch (plug_in, "<Image>/Help", N_("_User Manual"));

  gimp_plug_in_add_menu_branch (plug_in, "<Image>/Filters/Development",
                                N_("_Script-Fu"));
  gimp_plug_in_add_menu_branch (plug_in, "<Image>/Filters/Development/Script-Fu",
                                N_("_Test"));

  gimp_plug_in_add_menu_branch (plug_in, "<Image>/File/Create",
                                N_("_Buttons"));
  gimp_plug_in_add_menu_branch (plug_in, "<Image>/File/Create",
                                N_("_Logos"));
  gimp_plug_in_add_menu_branch (plug_in, "<Image>/File/Create",
                                N_("_Patterns"));

  gimp_plug_in_add_menu_branch (plug_in, "<Image>/File/Create",
                                N_("_Web Page Themes"));
  gimp_plug_in_add_menu_branch (plug_in, "<Image>/File/Create/Web Page Themes",
                                N_("_Alien Glow"));
  gimp_plug_in_add_menu_branch (plug_in, "<Image>/File/Create/Web Page Themes",
                                N_("_Beveled Pattern"));
  gimp_plug_in_add_menu_branch (plug_in, "<Image>/File/Create/Web Page Themes",
                                N_("_Classic.Gimp.Org"));

  gimp_plug_in_add_menu_branch (plug_in, "<Image>/Filters",
                                N_("Alpha to _Logo"));

  procedure = gimp_procedure_new (plug_in, "script-fu-refresh",
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  script_fu_refresh_proc, NULL, NULL);

  gimp_procedure_set_menu_label (procedure, N_("_Refresh Scripts"));
  gimp_procedure_add_menu_path (procedure,
                                "<Image>/Filters/Development/Script-Fu");

  gimp_procedure_set_documentation (procedure,
                                    N_("Re-read all available Script-Fu scripts"),
                                    "Re-read all available Script-Fu scripts",
                                    "script-fu-refresh");
  gimp_procedure_set_attribution (procedure,
                                  "Spencer Kimball & Peter Mattis",
                                  "Spencer Kimball & Peter Mattis",
                                  "1997");

  GIMP_PROC_ARG_ENUM (procedure, "run-mode",
                      "Run mode",
                      "The run mode",
                      GIMP_TYPE_RUN_MODE,
                      GIMP_RUN_INTERACTIVE,
                      G_PARAM_READWRITE);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);
}

static GimpValueArray *
script_fu_refresh_proc (GimpProcedure        *procedure,
                        const GimpValueArray *args,
                        gpointer              run_data)
{
  if (script_fu_interface_is_active ())
    {
      g_message (_("You can not use \"Refresh Scripts\" while a "
                   "Script-Fu dialog box is open.  Please close "
                   "all Script-Fu windows and try again."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }
  else
    {
      /*  Reload all of the available scripts  */
      GList *path = script_fu_search_path ();

      script_fu_find_scripts (gimp_procedure_get_plug_in (procedure), path);

      g_list_free_full (path, (GDestroyNotify) g_object_unref);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}
