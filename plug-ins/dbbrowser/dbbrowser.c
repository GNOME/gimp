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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/* 
   dbbrowser 
   0.08 26th sept 97  by Thomas NOEL <thomas@minet.net> 
*/


/*
  This plugin gives you the list of available procedure, with the
  name, description and parameters for each procedure.
  You can do regexp search (by name and by description)
  Useful for scripts development.

  NOTE :
  this is only a exercice for me (my first "plug-in" (extension))
  so it's very (very) dirty. 
  Btw, hope it gives you some ideas about gimp possibilities.

  The core of the plugin is not here. See dbbrowser_utils (shared
  with script-fu-console).

  TODO
  - bug fixes... (my method : rewrite from scratch :)
*/

#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>

#include "dbbrowser.h"

static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, [non-interactive]" }
  };

  static int nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("extension_db_browser",
                          "list available procedures in the PDB",
                          "",
                          "Thomas Noel",
                          "Thomas Noel",
                          "23th june 1997",
                          "<Toolbox>/Xtns/DB Browser",
			  "",
                          PROC_EXTENSION,
			  nargs, 0,
                          args, NULL);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GRunModeType run_mode;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_SUCCESS;

  switch (run_mode)
    {
    case RUN_INTERACTIVE: 
      {
	gchar **argv;
	gint  argc;  

	argc = 1;
	argv = g_new (gchar *, 1);
	argv[0] = g_strdup ("dbbrowser");
	gtk_init (&argc, &argv);
	gtk_rc_parse (gimp_gtkrc ());
	
	gtk_quit_add_destroy (1, (GtkObject*) gimp_db_browser (NULL));

	gtk_main ();
	gdk_flush ();
      }
      break;
      
    case RUN_WITH_LAST_VALS:
    case RUN_NONINTERACTIVE:
      g_warning ("dbbrowser allows only interactive invocation");
      values[0].data.d_status = STATUS_CALLING_ERROR;
      break;
      
    default:
      break;
    }  
  
}













