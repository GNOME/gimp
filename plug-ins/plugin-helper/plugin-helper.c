/* The GIMP -- an image manipulation program
 *
 * plugin-helper -- support plugin for dynamically loaded plugin modules
 * Copyright (C) 2000 Nathan Summers
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


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include <gmodule.h>

/* why aren't these functions defined in gimp.h? */

extern void gimp_extension_ack (void);
extern void gimp_extension_process (int timeout);

/* Declare local functions */
static void helper_query (void);
static void helper_run (gchar * name,
		 gint nparams,
		 GimpParam * param,
		 gint * nreturn_vals, GimpParam ** return_vals);


static void query_module (gchar *);

GimpPlugInInfo PLUG_IN_INFO = {
  NULL,				/* init_proc  */
  NULL,				/* quit_proc  */
  helper_query,			/* query_proc */
  helper_run,				/* run_proc   */
};


MAIN ()

static void helper_query (void)
{
  g_message ("query called");
  gimp_install_procedure ("extension_plugin_helper",
			  "Proxy process for dynamically loaded plugin modules",
			  "Automagically called by the Gimp at startup",
			  "Nathan Summers",
			  "Nathan Summers",
			  "2000",
			  NULL, NULL, GIMP_EXTENSION, 0, 0, NULL, NULL);
  g_message ("query done");
}

static void
helper_run (gchar * name,
     gint nparams,
     GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
  static GimpParam values[2];
  GimpRunModeType run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  g_message ("run called %s", name);

  /* No parameters have been defined: asking for one */
  /* seg-faults, since param[0] == NULL              */
  /* See helper_query                       -garo-   */
   
  /* run_mode = param[0].data.d_int32; */

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, "extension_plugin_helper") == 0)
    {
      status = GIMP_PDB_SUCCESS;
      values[0].data.d_status = status;

	query_module ("/usr/local/lib/gimp/1.3/plugin-modules/libiwarp.so");

      g_message ("time for the evil loop");


      gimp_extension_ack ();

      while (TRUE)		/* this construction bothers me deeply */
	gimp_extension_process (0);

      /* doubtful that execution ever passes this point. */
      g_message ("Brigadoon!");
    }
  else
     {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}


void query_module (gchar *module)
{
	GModule *mod;
	GimpQueryProc query_proc = NULL;

	mod = g_module_open (module, G_MODULE_BIND_LAZY);

	if (!mod) {
		g_message ("Could not open module %s!", module);
		return;
	}
	
	if (g_module_symbol (mod, "query", (gpointer *)&query_proc)) {
		g_message ("alright!");
		(*query_proc)();
	} else {
		g_message ("doh!");
		g_error (g_module_error());
	}
}


void plugin_module_install_procedure (gchar *name,
					 gchar *blurb,
					 gchar *help,
					 gchar *author,
					 gchar *copyright,
					 gchar *date,
					 gchar *menu_path,
					 gchar *image_types,
					 gint nparams,
					 gint nreturn_vals,
					 GimpParamDef *params,
					 GimpParamDef *return_vals,
					 GimpRunProc run_proc)
{
	g_message ("Installing plug-in procedure %s (%p)", name, run_proc);

	gimp_install_temp_proc(name, blurb, help, author, copyright, date, menu_path, image_types, 
		GIMP_TEMPORARY, nparams, nreturn_vals, params, return_vals, 
		run_proc);

};






#if 0
      /* search for binaries in the plug-in directory path */
      find_modules (plug_in_path, plug_in_init_file,
				  MODE_EXECUTABLE);

      /* read the pluginrc file for cached data */
      filename = NULL;
      if (pluginrc_path)
	{
	  if (g_path_is_absolute (pluginrc_path))
	    filename = g_strdup (pluginrc_path);
	  else
	    filename = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s",
					gimp_directory (), pluginrc_path);
	}
      else
	filename = gimp_personal_rc_file ("pluginrc");

      app_init_update_status (_("Resource configuration"), filename, -1);
      parse_gimprc_file (filename);

      /* query any plug-ins that have changed since we last wrote out
         *  the pluginrc file.
       */
      tmp = plug_in_defs;
      app_init_update_status (_("Plug-ins"), "", 0);
      nplugins = g_slist_length (tmp);
      nth = 0;
      while (tmp)
	{
	  plug_in_def = tmp->data;
	  tmp = tmp->next;

	  if (plug_in_def->query)
	    {
	      write_pluginrc = TRUE;

	      if (be_verbose)
		g_print (_("query plug-in: \"%s\"\n"), plug_in_def->prog);

	      plug_in_query (plug_in_def);
	    }

	  app_init_update_status (NULL, plug_in_def->prog, nth / nplugins);
	  nth++;
	}

      /* insert the proc defs */
      for (tmp = gimprc_proc_defs; tmp; tmp = g_slist_next (tmp))
	{
	  proc_def = g_new (PlugInProcDef, 1);
	  *proc_def = *((PlugInProcDef *) tmp->data);
	  plug_in_proc_def_insert (proc_def, NULL);
	}

      tmp = plug_in_defs;
      while (tmp)
	{
	  plug_in_def = tmp->data;
	  tmp = tmp->next;

	  tmp = tmp->next;

	  tmp2 = plug_in_def->proc_defs;
	  while (tmp2)
	    {
	      proc_def = tmp2->data;
	      tmp2 = tmp2->next;

	      proc_def->mtime = plug_in_def->mtime;
	      plug_in_proc_def_insert (proc_def, plug_in_proc_def_dead);
	    }
	}

      /* write the pluginrc file if necessary */
      if (write_pluginrc)
	{
	  if (be_verbose)
	    g_print (_("writing \"%s\"\n"), filename);

	  plug_in_write_rc (filename);
	}

      g_free (filename);

      /* add the plug-in procs to the procedure database */
      plug_in_add_to_db ();

#endif

