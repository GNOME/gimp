/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphelp.c
 * Copyright (C) 1999 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "gimphelp.h"
#include "gimprc.h"
#include "plug_in.h"
#include "procedural_db.h"

#include "libgimp/gimpenv.h"
#include "libgimp/gimpintl.h"

#define DEBUG_HELP

/*  local function prototypes  */
static gint gimp_idle_help                       (gpointer        help_data);
static void gimp_help_internal                   (gchar          *current_locale,
						  gchar          *help_data);
static void gimp_help_netscape                   (gchar          *current_locale,
						  gchar          *help_data);

/**********************/
/*  public functions  */
/**********************/

/*  The standard help function  */
void
gimp_standard_help_func (gchar *help_data)
{
  gimp_help (help_data);
}

/*  the main help function  */
void
gimp_help (gchar *help_data)
{
  if (use_help)
    {
      if (help_data)
	help_data = g_strdup (help_data);

      gtk_idle_add ((GtkFunction) gimp_idle_help, (gpointer) help_data);
    }
}

/*********************/
/*  local functions  */
/*********************/

static gint
gimp_idle_help (gpointer help_data)
{
  static gchar *current_locale = "C";

  if (help_data == NULL && help_browser != HELP_BROWSER_GIMP)
    help_data = g_strdup ("welcome.html");

#ifdef DEBUG_HELP
  g_print ("Help Page: %s\n", (gchar *) help_data);
#endif  /*  DEBUG_HELP  */

  switch (help_browser)
    {
    case HELP_BROWSER_GIMP:
      gimp_help_internal (current_locale, (gchar *) help_data);
      break;

    case HELP_BROWSER_NETSCAPE:
      gimp_help_netscape (current_locale, (gchar *) help_data);
      break;

    default:
      break;
    }

  if (help_data)
    g_free (help_data);

  return FALSE;
}

void
gimp_help_internal (gchar *current_locale,
		    gchar *help_data)
{
  ProcRecord *proc_rec;

  /*  Check if a help browser is already running  */
  proc_rec = procedural_db_lookup ("extension_gimp_help_browser_temp");

  if (proc_rec == NULL)
    {
      Argument *args = NULL;

      proc_rec = procedural_db_lookup ("extension_gimp_help_browser");

      if (proc_rec == NULL)
	{
	  g_message (_("Could not find the GIMP Help Browser procedure.\n"
		       "It probably was not compiled because\n"
		       "you don't have GtkXmHTML installed."));
	  return;
	}

      args = g_new (Argument, 3);
      args[0].arg_type = PDB_INT32;
      args[0].value.pdb_int = RUN_INTERACTIVE;
      args[1].arg_type = PDB_STRING;
      args[1].value.pdb_pointer = current_locale;
      args[2].arg_type = PDB_STRING;
      args[2].value.pdb_pointer = help_data;

      plug_in_run (proc_rec, args, 3, FALSE, TRUE, 0);

      g_free (args);
    }
  else
    {
      Argument *return_vals;
      gint      nreturn_vals;

      return_vals =
        procedural_db_run_proc ("extension_gimp_help_browser_temp",
                                &nreturn_vals,
				PDB_STRING, current_locale,
                                PDB_STRING, help_data,
                                PDB_END);

      procedural_db_destroy_args (return_vals, nreturn_vals);
    }
}

void
gimp_help_netscape (gchar *current_locale,
		    gchar *help_data)
{
  Argument *return_vals;
  gint      nreturn_vals;
  gchar    *url;

  url = g_strconcat ("file:",
		     gimp_data_directory (),
		     "/help/",
		     current_locale, "/",
		     help_data,
		     NULL);

  return_vals =
    procedural_db_run_proc ("extension_web_browser",
			    &nreturn_vals,
			    PDB_INT32,  RUN_NONINTERACTIVE,
			    PDB_STRING, url,
			    PDB_INT32,  FALSE,
			    PDB_END);

  procedural_db_destroy_args (return_vals, nreturn_vals);

  g_free (url);
}
