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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gimphelp.h"
#include "gimprc.h"
#include "plug_in.h"
#include "procedural_db.h"

#include "libgimp/gimpintl.h"

/*  The standard help function  */
void
gimp_standard_help_func (gpointer help_data)
{
  gchar *help_page;

  help_page = (gchar *) help_data;
  gimp_help (help_page);
}

static void
gimp_help_callback (GtkWidget *widget,
		    gpointer   data)
{
  GimpHelpFunc help_function;
  gpointer     help_data;

  help_function = (GimpHelpFunc) data;
  help_data     = gtk_object_get_data (GTK_OBJECT (widget),
				       "gimp_help_data");

  if (help_function && use_help)
    (* help_function) (help_data);
}

void
gimp_help_connect_help_accel (GtkWidget    *widget,
			      GimpHelpFunc  help_func,
			      gpointer      help_data)
{
  GtkAccelGroup *accel_group;
  static guint help_signal_id = 0;

  if (! help_func)
    return;

  /*  create the help signal if not already done  */
  if (! help_signal_id)
    {
      help_signal_id =
	gtk_object_class_user_signal_new (GTK_OBJECT (widget)->klass,
					  "help",
					  GTK_RUN_LAST,
					  gtk_signal_default_marshaller,
					  GTK_TYPE_NONE,
					  0,
					  NULL);
    }

  if (help_data)
    {
      gtk_object_set_data (GTK_OBJECT (widget), "gimp_help_data",
			   help_data);
    }

  gtk_signal_connect (GTK_OBJECT (widget), "help",
		      GTK_SIGNAL_FUNC (gimp_help_callback),
		      (gpointer) help_func);

  /*  a new accelerator group for this widget  */
  accel_group = gtk_accel_group_new ();

  /*  FIXME: does not work for some reason...
  gtk_widget_add_accelerator (widget, "help", accel_group,
			      GDK_F1, 0, GTK_ACCEL_LOCKED);
  */

  /*  ...while using this internal stuff works  */
  gtk_accel_group_add (accel_group, GDK_F1, 0, 0,
		       GTK_OBJECT (widget), "help");
  gtk_accel_group_attach (accel_group, GTK_OBJECT (widget));
}

/*  the main help function  */
void
gimp_help (gchar *help_page)
{
  ProcRecord *proc_rec;
  Argument   *return_vals;
  gint        nreturn_vals;

  /*  Check if a help browser is already running  */
  proc_rec = procedural_db_lookup ("extension_gimp_help_browser_temp");
  if (proc_rec == NULL)
    {
      proc_rec = procedural_db_lookup ("extension_gimp_help_browser");
      if (proc_rec == NULL)
	{
	  g_message (_("Could not find the GIMP Help Browser procedure\n"
		       "Note that you still have to compile this plugin "
		       "manually"));
	  return;
	}

      return_vals =
	procedural_db_run_proc ("extension_gimp_help_browser",
				&nreturn_vals,
				PDB_INT32, RUN_INTERACTIVE,
				PDB_STRING, help_page,
				PDB_END);
    }
  else
    {
      return_vals =
	procedural_db_run_proc ("extension_gimp_help_browser_temp",
				&nreturn_vals,
				PDB_STRING, help_page,
				PDB_END);
    }

  procedural_db_destroy_args (return_vals, nreturn_vals);
}
