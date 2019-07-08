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

/*
 * dbbrowser
 * 0.08 26th sept 97  by Thomas NOEL <thomas@minet.net>
 */

/*
 * This plugin gives you the list of available procedure, with the
 * name, description and parameters for each procedure.
 * You can do regexp search (by name and by description)
 * Useful for scripts development.
 *
 * NOTE :
 * this is only a exercice for me (my first "plug-in" (extension))
 * so it's very (very) dirty.
 * Btw, hope it gives you some ideas about gimp possibilities.
 *
 * The core of the plugin is not here. See dbbrowser_utils (shared
 * with script-fu-console).
 *
 * TODO
 * - bug fixes... (my method : rewrite from scratch :)
 */

#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-dbbrowser"
#define PLUG_IN_BINARY "procedure-browser"
#define PLUG_IN_ROLE   "gimp-procedure-browser"


static void   query (void);
static void   run   (const gchar      *name,
                     gint              nparams,
                     const GimpParam  *param,
                     gint             *nreturn_vals,
                     GimpParam       **return_vals);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run-mode", "The run mode { RUN-INTERACTIVE (0) }" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("List available procedures in the PDB"),
                          "",
                          "Thomas Noel",
                          "Thomas Noel",
                          "23th june 1997",
                          N_("Procedure _Browser"),
                          "",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Help/Programming");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam values[1];
  GimpRunMode      run_mode;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  INIT_I18N ();

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      {
        GtkWidget *dialog;

        gimp_ui_init (PLUG_IN_BINARY, FALSE);

        dialog =
          gimp_proc_browser_dialog_new (_("Procedure Browser"), PLUG_IN_BINARY,
                                        gimp_standard_help_func, PLUG_IN_PROC,

                                        _("_Close"), GTK_RESPONSE_CLOSE,

                                        NULL);

        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
      }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
    case GIMP_RUN_NONINTERACTIVE:
      g_warning (PLUG_IN_PROC " allows only interactive invocation");
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      break;

    default:
      break;
    }
}
