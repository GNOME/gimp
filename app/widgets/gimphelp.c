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

#define USE_TIPS_QUERY

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

/*  Do all the actual GtkTipsQuery calls in idle functions and check for
 *  some widget holding a grab before starting the query because strange
 *  things happen if (1) the help browser pops up while the query has
 *  grabbed the pointer or (2) the query grabs the pointer while some
 *  other part of the gimp has grabbed it (e.g. the move tool, eek)
 */

#ifdef USE_TIPS_QUERY

static gint
gimp_help_tips_query_idle_show_help (gpointer help_data)
{
  gimp_help ((gchar *) help_data);

  return FALSE;
}

static gint
gimp_help_tips_query_widget_selected (GtkWidget      *tips_query,
				      GtkWidget      *widget,
				      const gchar    *tip_text,
				      const gchar    *tip_private,
				      GdkEventButton *event,
				      gpointer        func_data)
{
  if (widget && tip_private && use_help)
    gtk_idle_add ((GtkFunction) gimp_help_tips_query_idle_show_help,
		  (gpointer) tip_private);

  return TRUE;
}

static gint
gimp_help_tips_query_idle_start (gpointer tips_query)
{
  if (! gtk_grab_get_current ())
    gtk_tips_query_start_query (GTK_TIPS_QUERY (tips_query));

  return FALSE;
}

static void
gimp_help_tips_query_start (GtkWidget *widget,
			    gpointer   tips_query)
{
  if (! GTK_TIPS_QUERY (tips_query)->in_query)
    gtk_idle_add ((GtkFunction) gimp_help_tips_query_idle_start, tips_query);
}

#endif

void
gimp_help_connect_help_accel (GtkWidget    *widget,
			      GimpHelpFunc  help_func,
			      gpointer      help_data)
{
  GtkAccelGroup *accel_group;

  static guint      help_signal_id       = 0;
#ifdef USE_TIPS_QUERY
  static guint      tips_query_signal_id = 0;
  static GtkWidget *tips_query           = NULL;
#endif

  if (!help_func)
    return;

  /*  set up the help signals and tips query widget  */
  if (! help_signal_id)
    {
#ifdef USE_TIPS_QUERY
      tips_query = gtk_tips_query_new ();

      gtk_signal_connect (GTK_OBJECT (tips_query), "widget_selected",
			  GTK_SIGNAL_FUNC (gimp_help_tips_query_widget_selected),
			  NULL);

      /*  FIXME: EEEEEEEEEEEEEEEEEEEEK, this is very ugly and forbidden...
       *  does anyone know a way to do this tips query stuff without
       *  having to attach to some parent widget???
       */
      tips_query->parent = widget;
      gtk_widget_realize (tips_query);

      tips_query_signal_id =
	gtk_object_class_user_signal_new (GTK_OBJECT (widget)->klass,
					  "tips_query",
					  GTK_RUN_LAST,
					  gtk_signal_default_marshaller,
					  GTK_TYPE_NONE,
					  0,
					  NULL);
#endif

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

#ifdef USE_TIPS_QUERY
  gtk_signal_connect (GTK_OBJECT (widget), "tips_query",
		      GTK_SIGNAL_FUNC (gimp_help_tips_query_start),
		      (gpointer) tips_query);
#endif

  /*  a new accelerator group for this widget  */
  accel_group = gtk_accel_group_new ();

  /*  FIXME: does not work for some reason...
  gtk_widget_add_accelerator (widget, "help", accel_group,
			      GDK_F1, 0, GTK_ACCEL_LOCKED);
  gtk_widget_add_accelerator (widget, "tips_query", accel_group,
			      GDK_F1, GDK_SHIFT_MASK, GTK_ACCEL_LOCKED);
  */

  /*  ...while using this internal stuff works  */
  gtk_accel_group_add (accel_group, GDK_F1, 0, 0,
		       GTK_OBJECT (widget), "help");
#ifdef USE_TIPS_QUERY
  gtk_accel_group_add (accel_group, GDK_F1, GDK_SHIFT_MASK, 0,
		       GTK_OBJECT (widget), "tips_query");
#endif

  gtk_accel_group_attach (accel_group, GTK_OBJECT (widget));
}

/*  the main help function  */
void
gimp_help (gchar *help_page)
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

      args = g_new (Argument, 2);
      args[0].arg_type = PDB_INT32;
      args[0].value.pdb_int = RUN_INTERACTIVE;
      args[1].arg_type = PDB_STRING;
      args[1].value.pdb_pointer = help_page;

      plug_in_run (proc_rec, args, 2, FALSE, TRUE, 0);

      g_free (args);
    }
  else
    {
      Argument *return_vals;
      gint      nreturn_vals;

      return_vals =
        procedural_db_run_proc ("extension_gimp_help_browser_temp",
                                &nreturn_vals,
                                PDB_STRING, help_page,
                                PDB_END);

      procedural_db_destroy_args (return_vals, nreturn_vals);
    }
}
