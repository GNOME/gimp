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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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


/* Declare local functions. */

static void    script_fu_query          (void);
static void    script_fu_run            (const gchar      *name,
                                         gint              nparams,
                                         const GimpParam  *params,
                                         gint             *nreturn_vals,
                                         GimpParam       **return_vals);
static GList * script_fu_search_path    (void);
static void    script_fu_extension_init (void);
static void    script_fu_refresh_proc   (const gchar      *name,
                                         gint              nparams,
                                         const GimpParam  *params,
                                         gint             *nreturn_vals,
                                         GimpParam       **return_vals);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,             /* init_proc  */
  NULL,             /* quit_proc  */
  script_fu_query,  /* query_proc */
  script_fu_run     /* run_proc   */
};


MAIN ()


static void
script_fu_query (void)
{
  static const GimpParamDef console_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode", "The run mode { RUN-INTERACTIVE (0) }" }
  };

  static const GimpParamDef textconsole_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode", "The run mode { RUN-INTERACTIVE (0) }" }
  };

  static const GimpParamDef eval_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode", "The run mode { RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "code",     "The code to evaluate"                    }
  };

  static const GimpParamDef server_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode", "The run mode { RUN-NONINTERACTIVE (1) }"  },
    { GIMP_PDB_STRING, "ip",       "The ip on which to listen for requests"   },
    { GIMP_PDB_INT32,  "port",     "The port on which to listen for requests" },
    { GIMP_PDB_STRING, "logfile",  "The file to log server activity to"       }
  };

  gimp_plugin_domain_register (GETTEXT_PACKAGE "-script-fu", NULL);

  gimp_install_procedure ("extension-script-fu",
                          "A scheme interpreter for scripting GIMP operations",
                          "More help here later",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1997",
                          NULL,
                          NULL,
                          GIMP_EXTENSION,
                          0, 0, NULL, NULL);

  gimp_install_procedure ("plug-in-script-fu-console",
                          N_("Interactive console for Script-Fu development"),
                          "Provides an interface which allows interactive "
                                      "scheme development.",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1997",
                          N_("_Console"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (console_args), 0,
                          console_args, NULL);

  gimp_plugin_menu_register ("plug-in-script-fu-console",
                             "<Image>/Filters/Languages/Script-Fu");

  gimp_install_procedure ("plug-in-script-fu-text-console",
                          "Provides a text console mode for script-fu "
                          "development",
                          "Provides an interface which allows interactive "
                          "scheme development.",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1997",
                          NULL,
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (textconsole_args), 0,
                          textconsole_args, NULL);

  gimp_install_procedure ("plug-in-script-fu-server",
                          N_("Server for remote Script-Fu operation"),
                          "Provides a server for remote script-fu operation. "
                          "NOTE that for security reasons this procedure's "
                          "API was changed in an incompatible way since "
                          "GIMP 2.8.12. You now have to pass the IP to listen "
                          "on as first parameter. Calling this procedure with "
                          "the old API will fail on purpose.",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1997",
                          N_("_Start Server..."),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (server_args), 0,
                          server_args, NULL);

  gimp_plugin_menu_register ("plug-in-script-fu-server",
                             "<Image>/Filters/Languages/Script-Fu");

  gimp_install_procedure ("plug-in-script-fu-eval",
                          "Evaluate scheme code",
                          "Evaluate the code under the scheme interpreter "
                                      "(primarily for batch mode)",
                          "Manish Singh",
                          "Manish Singh",
                          "1998",
                          NULL,
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (eval_args), 0,
                          eval_args, NULL);
}

static void
script_fu_run (const gchar      *name,
               gint              nparams,
               const GimpParam  *param,
               gint             *nreturn_vals,
               GimpParam       **return_vals)
{
  GList *path;

  INIT_I18N();

  path = script_fu_search_path ();

  /*  Determine before we allow scripts to register themselves
   *   whether this is the base, automatically installed script-fu extension
   */
  if (strcmp (name, "extension-script-fu") == 0)
    {
      /*  Setup auxiliary temporary procedures for the base extension  */
      script_fu_extension_init ();

      /*  Init the interpreter and register scripts */
      tinyscheme_init (path, TRUE);
    }
  else
    {
      /*  Init the interpreter  */
      tinyscheme_init (path, FALSE);
    }

  if (param != NULL)
    ts_set_run_mode ((GimpRunMode) param[0].data.d_int32);

  /*  Load all of the available scripts  */
  script_fu_find_scripts (path);

  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  if (strcmp (name, "extension-script-fu") == 0)
    {
      /*
       *  The main script-fu extension.
       */

      static GimpParam  values[1];

      /*  Acknowledge that the extension is properly initialized  */
      gimp_extension_ack ();

      /*  Go into an endless loop  */
      while (TRUE)
        gimp_extension_process (0);

      /*  Set return values; pointless because we never get out of the loop  */
      *nreturn_vals = 1;
      *return_vals  = values;

      values[0].type          = GIMP_PDB_STATUS;
      values[0].data.d_status = GIMP_PDB_SUCCESS;
    }
  else if (strcmp (name, "plug-in-script-fu-text-console") == 0)
    {
      /*
       *  The script-fu text console for interactive Scheme development
       */

      script_fu_text_console_run (name, nparams, param,
                                  nreturn_vals, return_vals);
    }
  else if (strcmp (name, "plug-in-script-fu-console") == 0)
    {
      /*
       *  The script-fu console for interactive Scheme development
       */

      script_fu_console_run (name, nparams, param,
                             nreturn_vals, return_vals);
    }
  else if (strcmp (name, "plug-in-script-fu-server") == 0)
    {
      /*
       *  The script-fu server for remote operation
       */

      script_fu_server_run (name, nparams, param,
                            nreturn_vals, return_vals);
    }
  else if (strcmp (name, "plug-in-script-fu-eval") == 0)
    {
      /*
       *  A non-interactive "console" (for batch mode)
       */

      script_fu_eval_run (name, nparams, param,
                          nreturn_vals, return_vals);
    }
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
script_fu_extension_init (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run-mode", "[Interactive], non-interactive" }
  };

  gimp_plugin_menu_branch_register ("<Image>/Help", N_("_GIMP Online"));
  gimp_plugin_menu_branch_register ("<Image>/Help", N_("_User Manual"));

  gimp_plugin_menu_branch_register ("<Image>/Filters/Languages",
                                    N_("_Script-Fu"));
  gimp_plugin_menu_branch_register ("<Image>/Filters/Languages/Script-Fu",
                                    N_("_Test"));

  gimp_plugin_menu_branch_register ("<Image>/File/Create",
                                    N_("_Buttons"));
  gimp_plugin_menu_branch_register ("<Image>/File/Create",
                                    N_("_Logos"));
  gimp_plugin_menu_branch_register ("<Image>/File/Create",
                                    N_("_Patterns"));

  gimp_plugin_menu_branch_register ("<Image>/File/Create",
                                    N_("_Web Page Themes"));
  gimp_plugin_menu_branch_register ("<Image>/File/Create/Web Page Themes",
                                    N_("_Alien Glow"));
  gimp_plugin_menu_branch_register ("<Image>/File/Create/Web Page Themes",
                                    N_("_Beveled Pattern"));
  gimp_plugin_menu_branch_register ("<Image>/File/Create/Web Page Themes",
                                    N_("_Classic.Gimp.Org"));

  gimp_plugin_menu_branch_register ("<Image>/Filters",
                                    N_("Alpha to _Logo"));

  gimp_install_temp_proc ("script-fu-refresh",
                          N_("Re-read all available Script-Fu scripts"),
                          "Re-read all available Script-Fu scripts",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1997",
                          N_("_Refresh Scripts"),
                          NULL,
                          GIMP_TEMPORARY,
                          G_N_ELEMENTS (args), 0,
                          args, NULL,
                          script_fu_refresh_proc);

  gimp_plugin_menu_register ("script-fu-refresh",
                             "<Image>/Filters/Languages/Script-Fu");
}

static void
script_fu_refresh_proc (const gchar      *name,
                        gint              nparams,
                        const GimpParam  *params,
                        gint             *nreturn_vals,
                        GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status;

  if (script_fu_interface_is_active ())
    {
      g_message (_("You can not use \"Refresh Scripts\" while a "
                   "Script-Fu dialog box is open.  Please close "
                   "all Script-Fu windows and try again."));

      status = GIMP_PDB_EXECUTION_ERROR;
    }
  else
    {
      /*  Reload all of the available scripts  */
      GList *path = script_fu_search_path ();

      script_fu_find_scripts (path);

      g_list_free_full (path, (GDestroyNotify) g_object_unref);

      status = GIMP_PDB_SUCCESS;
    }

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}
