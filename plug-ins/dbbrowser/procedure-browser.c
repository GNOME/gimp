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

#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpprocbrowser.h"

#include "libgimp/stdplugins-intl.h"


static void   query (void);
static void   run   (const gchar      *name,
                     gint              nparams,
                     const GimpParam  *param,
                     gint             *nreturn_vals,
                     GimpParam       **return_vals);

GimpPlugInInfo PLUG_IN_INFO =
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
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, [non-interactive]" }
  };

  gimp_install_procedure ("plug_in_db_browser",
                          "List available procedures in the PDB",
                          "",
                          "Thomas Noel",
                          "Thomas Noel",
                          "23th june 1997",
                          N_("Procedure _Browser"),
                          "",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register ("plug_in_db_browser", "<Toolbox>/Xtns/Extensions");
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
        gimp_ui_init ("dbbrowser", FALSE);

        gtk_quit_add_destroy (1, (GtkObject *)
                              gimp_proc_browser_dialog_new (FALSE, NULL, NULL));

        gtk_main ();
        gdk_flush ();
      }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
    case GIMP_RUN_NONINTERACTIVE:
      g_warning ("dbbrowser allows only interactive invocation");
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      break;

    default:
      break;
    }
}
