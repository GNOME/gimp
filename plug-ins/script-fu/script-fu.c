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

#include "console/script-fu-console.h"
#include "script-fu-eval.h"
#include "script-fu-text-console.h"

#include "libscriptfu/script-fu-lib.h"
#include "libscriptfu/script-fu-intl.h"


#define SCRIPT_FU_TYPE (script_fu_get_type ())
G_DECLARE_FINAL_TYPE (ScriptFu, script_fu, SCRIPT, FU, GimpPlugIn)

struct _ScriptFu
{
  GimpPlugIn      parent_instance;
};

static GList          * script_fu_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * script_fu_create_procedure (GimpPlugIn           *plug_in,
                                                    const gchar          *name);

static GimpValueArray * script_fu_run              (GimpProcedure        *procedure,
                                                    GimpProcedureConfig  *config,
                                                    gpointer              run_data);
static GimpValueArray * script_fu_batch_run        (GimpProcedure        *procedure,
                                                    GimpRunMode           run_mode,
                                                    const gchar          *code,
                                                    GimpProcedureConfig  *config,
                                                    gpointer              run_data);
static void             script_fu_run_init         (GimpProcedure        *procedure,
                                                    GimpRunMode           run_mode);
static void             script_fu_extension_init   (GimpPlugIn           *plug_in);


G_DEFINE_TYPE (ScriptFu, script_fu, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SCRIPT_FU_TYPE)
DEFINE_STD_SET_I18N


static void
script_fu_class_init (ScriptFuClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = script_fu_query_procedures;
  plug_in_class->create_procedure = script_fu_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
script_fu_init (ScriptFu *script_fu)
{
}

static GList *
script_fu_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup ("extension-script-fu"));
  list = g_list_append (list, g_strdup ("plug-in-script-fu-console"));
  list = g_list_append (list, g_strdup ("plug-in-script-fu-text-console"));
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

      gimp_procedure_set_menu_label (procedure, _("Script-Fu _Console"));
      gimp_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Development/Script-Fu");

      gimp_procedure_set_documentation (procedure,
                                        _("Interactive console for Script-Fu "
                                          "development"),
                                        "Provides an interface which allows "
                                        "interactive scheme development.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1997");

      gimp_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        GIMP_TYPE_RUN_MODE,
                                        GIMP_RUN_INTERACTIVE,
                                        G_PARAM_READWRITE);
      gimp_procedure_add_string_array_argument (procedure, "history",
                                                "Command history",
                                                "History",
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

      gimp_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        GIMP_TYPE_RUN_MODE,
                                        GIMP_RUN_INTERACTIVE,
                                        G_PARAM_READWRITE);
    }
  else if (! strcmp (name, "plug-in-script-fu-eval"))
    {
      procedure = gimp_batch_procedure_new (plug_in, name, "Script-fu (scheme)",
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            script_fu_batch_run, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Evaluate scheme code",
                                        "Evaluate the code under the scheme "
                                        "interpreter (primarily for batch mode)",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Manish Singh",
                                      "Manish Singh",
                                      "1998");
    }

  return procedure;
}

static GimpValueArray *
script_fu_run (GimpProcedure        *procedure,
               GimpProcedureConfig  *config,
               gpointer              run_data)
{
  GimpPlugIn     *plug_in     = gimp_procedure_get_plug_in (procedure);
  const gchar    *name        = gimp_procedure_get_name (procedure);
  GimpValueArray *return_vals = NULL;
  GimpRunMode     run_mode    = GIMP_RUN_NONINTERACTIVE;

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (config), "run-mode") != NULL)
    g_object_get (config, "run-mode", &run_mode, NULL);
  script_fu_run_init (procedure, run_mode);

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

      return_vals = script_fu_text_console_run (procedure, config);
    }
  else if (strcmp (name, "plug-in-script-fu-console") == 0)
    {
      /*
       *  The script-fu console for interactive Scheme development
       */

      return_vals = script_fu_console_run (procedure, config);
    }

  if (! return_vals)
    return_vals = gimp_procedure_new_return_values (procedure,
                                                    GIMP_PDB_SUCCESS,
                                                    NULL);

  return return_vals;
}

static GimpValueArray *
script_fu_batch_run (GimpProcedure        *procedure,
                     GimpRunMode           run_mode,
                     const gchar          *code,
                     GimpProcedureConfig  *config,
                     gpointer              run_data)
{
  const gchar    *name        = gimp_procedure_get_name (procedure);
  GimpValueArray *return_vals = NULL;

  script_fu_run_init (procedure, run_mode);

  if (strcmp (name, "plug-in-script-fu-eval") == 0)
    {
      /*
       *  A non-interactive "console" (for batch mode)
       */

      if (g_strcmp0 (code, "-") == 0)
        /* Redirecting to script-fu text console, for backward compatibility  */
        return_vals = script_fu_text_console_run (procedure, config);
      else
        return_vals = script_fu_eval_run (procedure, run_mode, code, config);
    }

  if (! return_vals)
    return_vals = gimp_procedure_new_return_values (procedure,
                                                    GIMP_PDB_SUCCESS,
                                                    NULL);

  return return_vals;
}

static void
script_fu_run_init (GimpProcedure *procedure,
                    GimpRunMode    run_mode)
{
  GimpPlugIn     *plug_in     = gimp_procedure_get_plug_in (procedure);
  const gchar    *name        = gimp_procedure_get_name (procedure);
  GList          *path;

  path = script_fu_search_path ();

  /*  Determine before we allow scripts to register themselves
   *   whether this is the base, automatically installed script-fu extension
   */
  if (strcmp (name, "extension-script-fu") == 0)
    {
      /*  Setup auxiliary temporary procedures for the base extension  */
      script_fu_extension_init (plug_in);

      /*  Init the interpreter, allow register scripts */
      script_fu_init_embedded_interpreter (path, TRUE, run_mode);
    }
  else
    {
      /*  Init the interpreter, not allow register scripts */
      script_fu_init_embedded_interpreter (path, FALSE, run_mode);
    }

  script_fu_find_and_register_scripts (plug_in, path);

  g_list_free_full (path, (GDestroyNotify) g_object_unref);
}

static void
script_fu_extension_init (GimpPlugIn *plug_in)
{
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

  /* Commented out until fixed or replaced.
   * script_fu_register_refresh_procedure (plug_in);
   */
}
