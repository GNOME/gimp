/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphelp.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include "sys/types.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "pdb/procedural_db.h"

#include "plug-in/plug-in-run.h"

#include "gimphelp.h"

#include "libgimp/gimpintl.h"


#ifndef G_OS_WIN32
#define DEBUG_HELP
#endif

typedef struct _GimpIdleHelp GimpIdleHelp;

struct _GimpIdleHelp
{
  Gimp  *gimp;
  gchar *help_path;
  gchar *help_data;
};


/*  local function prototypes  */

static gint      gimp_idle_help     (gpointer     data);
static gboolean  gimp_help_internal (Gimp        *gimp,
                                     const gchar *help_path,
				     const gchar *current_locale,
				     const gchar *help_data);
static void      gimp_help_netscape (Gimp        *gimp,
                                     const gchar *help_path,
				     const gchar *current_locale,
				     const gchar *help_data);


/*  public functions  */

void
gimp_help (Gimp        *gimp,
           const gchar *help_path,
	   const gchar *help_data)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (GIMP_GUI_CONFIG (gimp->config)->use_help)
    {
      GimpIdleHelp *idle_help;

      idle_help = g_new0 (GimpIdleHelp, 1);

      idle_help->gimp = gimp;

      if (help_path && strlen (help_path))
	idle_help->help_path = g_strdup (help_path);

      if (help_data && strlen (help_data))
	idle_help->help_data = g_strdup (help_data);

      g_idle_add (gimp_idle_help, idle_help);
    }
}


/*  private functions  */

static gboolean
gimp_idle_help (gpointer data)
{
  GimpIdleHelp        *idle_help;
  GimpHelpBrowserType  browser;
  static gchar        *current_locale = "C";

  idle_help = (GimpIdleHelp *) data;

  browser = GIMP_GUI_CONFIG (idle_help->gimp->config)->help_browser;

#ifdef DEBUG_HELP
  if (idle_help->help_path)
    g_print ("Help Path: %s\n", idle_help->help_path);
  else
    g_print ("Help Path: NULL\n");

  if (idle_help->help_data)
    g_print ("Help Page: %s\n", idle_help->help_data);
  else
    g_print ("Help Page: NULL\n");

  g_print ("\n");
#endif  /*  DEBUG_HELP  */

  switch (browser)
    {
    case GIMP_HELP_BROWSER_GIMP:
      if (gimp_help_internal (idle_help->gimp,
                              idle_help->help_path,
			      current_locale,
			      idle_help->help_data))
	break;

    case GIMP_HELP_BROWSER_NETSCAPE:
      gimp_help_netscape (idle_help->gimp,
                          idle_help->help_path,
			  current_locale,
			  idle_help->help_data);
      break;

    default:
      break;
    }

  if (idle_help->help_path)
    g_free (idle_help->help_path);
  if (idle_help->help_data)
    g_free (idle_help->help_data);
  g_free (idle_help);

  return FALSE;
}

static void
gimp_help_internal_not_found_callback (GtkWidget *widget,
				       gboolean   use_netscape,
				       gpointer   data)
{
  Gimp *gimp = GIMP (data);

  if (use_netscape)
    g_object_set (gimp->config,
		  "help-browser", GIMP_HELP_BROWSER_NETSCAPE,
		  NULL);
  
  gtk_main_quit ();
}

static gboolean
gimp_help_internal (Gimp        *gimp,
                    const gchar *help_path,
		    const gchar *current_locale,
		    const gchar *help_data)
{
  ProcRecord *proc_rec;

  /*  Check if a help browser is already running  */
  proc_rec = procedural_db_lookup (gimp, "extension_gimp_help_browser_temp");

  if (proc_rec == NULL)
    {
      Argument *args = NULL;

      proc_rec = procedural_db_lookup (gimp, "extension_gimp_help_browser");

      if (proc_rec == NULL)
	{
	  GtkWidget *not_found =
	    gimp_query_boolean_box (_("Could not find GIMP Help Browser"),
				    NULL, NULL, FALSE,
				    _("Could not find the GIMP Help Browser procedure.\n"
				      "It probably was not compiled because\n"
				      "you don't have GtkXmHTML installed."),
				    _("Use Netscape instead"),
				    GTK_STOCK_CANCEL,
				    NULL, NULL,
				    gimp_help_internal_not_found_callback,
				    gimp);
	  gtk_widget_show (not_found);
	  gtk_main ();

	  return (GIMP_GUI_CONFIG (gimp->config)->help_browser
		  != GIMP_HELP_BROWSER_NETSCAPE);
	}

      args = g_new (Argument, 4);
      args[0].arg_type          = GIMP_PDB_INT32;
      args[0].value.pdb_int     = GIMP_RUN_INTERACTIVE;
      args[1].arg_type          = GIMP_PDB_STRING;
      args[1].value.pdb_pointer = (gpointer) help_path;
      args[2].arg_type          = GIMP_PDB_STRING;
      args[2].value.pdb_pointer = (gpointer) current_locale;
      args[3].arg_type          = GIMP_PDB_STRING;
      args[3].value.pdb_pointer = (gpointer) help_data;

      plug_in_run (gimp, proc_rec, args, 4, FALSE, TRUE, -1);

      g_free (args);
    }
  else
    {
      Argument *return_vals;
      gint      nreturn_vals;

      return_vals =
        procedural_db_run_proc (gimp,
				"extension_gimp_help_browser_temp",
                                &nreturn_vals,
				GIMP_PDB_STRING, help_path,
				GIMP_PDB_STRING, current_locale,
                                GIMP_PDB_STRING, help_data,
                                GIMP_PDB_END);

      procedural_db_destroy_args (return_vals, nreturn_vals);
    }

  return TRUE;
}

static void
gimp_help_netscape (Gimp        *gimp,
                    const gchar *help_path,
		    const gchar *current_locale,
		    const gchar *help_data)
{
  Argument *return_vals;
  gint      nreturn_vals;
  gchar    *url;

  if (!help_data)
    help_data = "introduction.html";

  if (help_data[0] == '/')  /* _not_ g_path_is_absolute() */
    {
      url = g_strconcat ("file:", help_data, NULL);
    }
  else
    {
      if (!help_path)
	{
	  url = g_strconcat ("file:",
			     gimp_data_directory (),
			     "/help/",
			     current_locale, "/",
			     help_data,
			     NULL);
	}
      else
	{
	  url = g_strconcat ("file:",
			     help_path, "/",
			     current_locale, "/",
			     help_data,
			     NULL);
	}
    }

  return_vals =
    procedural_db_run_proc (gimp,
			    "extension_web_browser",
			    &nreturn_vals,
			    GIMP_PDB_INT32,  GIMP_RUN_NONINTERACTIVE,
			    GIMP_PDB_STRING, url,
			    GIMP_PDB_INT32,  FALSE,
			    GIMP_PDB_END);

  procedural_db_destroy_args (return_vals, nreturn_vals);

  g_free (url);
}
