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

#define DEBUG_HELP

/*  local function prototypes  */
static void gimp_help_callback                   (GtkWidget      *widget,
						  gpointer        data);
static gint gimp_help_tips_query_idle_show_help  (gpointer        data);
static gint gimp_help_tips_query_widget_selected (GtkWidget      *tips_query,
						  GtkWidget      *widget,
						  const gchar    *tip_text,
						  const gchar    *tip_private,
						  GdkEventButton *event,
						  gpointer        func_data);
static gint gimp_help_tips_query_idle_start      (gpointer        tips_query);
static void gimp_help_tips_query_start           (GtkWidget      *widget,
						  gpointer        tips_query);

/*  local variables  */
static GtkTooltips * tool_tips  = NULL;
static GtkWidget   * tips_query = NULL;

/**********************/
/*  public functions  */
/**********************/

void
gimp_help_init (void)
{
  tool_tips = gtk_tooltips_new ();
}

void
gimp_help_free (void)
{
  gtk_object_destroy (GTK_OBJECT (tool_tips));
  gtk_object_unref   (GTK_OBJECT (tool_tips));
}

void
gimp_help_enable_tooltips (void)
{
  gtk_tooltips_enable (tool_tips);
}

void
gimp_help_disable_tooltips (void)
{
  gtk_tooltips_disable (tool_tips);
}

/*  The standard help function  */
void
gimp_standard_help_func (gchar *help_data)
{
  gimp_help (help_data);
}

void
gimp_help_connect_help_accel (GtkWidget    *widget,
			      GimpHelpFunc  help_func,
			      gchar        *help_data)
{
  GtkAccelGroup *accel_group;

  if (!help_func)
    return;

  /*  set up the help signals and tips query widget  */
  if (!tips_query)
    {
      tips_query = gtk_tips_query_new ();

      gtk_widget_set (tips_query,
		      "GtkTipsQuery::emit_always", TRUE,
		      NULL);

      gtk_signal_connect (GTK_OBJECT (tips_query), "widget_selected",
			  GTK_SIGNAL_FUNC (gimp_help_tips_query_widget_selected),
			  NULL);

      /*  FIXME: EEEEEEEEEEEEEEEEEEEEK, this is very ugly and forbidden...
       *  does anyone know a way to do this tips query stuff without
       *  having to attach to some parent widget???
       */
      tips_query->parent = widget;
      gtk_widget_realize (tips_query);

      gtk_object_class_user_signal_new (GTK_OBJECT (widget)->klass,
					"tips_query",
					GTK_RUN_LAST,
					gtk_signal_default_marshaller,
					GTK_TYPE_NONE,
					0,
					NULL);

      gtk_object_class_user_signal_new (GTK_OBJECT (widget)->klass,
					"help",
					GTK_RUN_LAST,
					gtk_signal_default_marshaller,
					GTK_TYPE_NONE,
					0,
					NULL);
    }

  gimp_help_set_help_data (widget, NULL, help_data);

  gtk_signal_connect (GTK_OBJECT (widget), "help",
		      GTK_SIGNAL_FUNC (gimp_help_callback),
		      (gpointer) help_func);

  gtk_signal_connect (GTK_OBJECT (widget), "tips_query",
		      GTK_SIGNAL_FUNC (gimp_help_tips_query_start),
		      (gpointer) tips_query);

  gtk_widget_add_events (widget, GDK_BUTTON_PRESS_MASK);

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
  gtk_accel_group_add (accel_group, GDK_F1, GDK_SHIFT_MASK, 0,
		       GTK_OBJECT (widget), "tips_query");

  gtk_accel_group_attach (accel_group, GTK_OBJECT (widget));
}

void
gimp_help_set_help_data (GtkWidget *widget,
			 gchar     *tooltip,
			 gchar     *help_data)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (tooltip)
    gtk_tooltips_set_tip (tool_tips, widget, tooltip, help_data);
  else if (help_data)
    gtk_object_set_data (GTK_OBJECT (widget), "gimp_help_data", help_data);
}

/*  the main help function  */
void
gimp_help (gchar *help_data)
{
  ProcRecord *proc_rec;

  static gchar *current_locale = "C";

#ifdef DEBUG_HELP
  g_print ("Help Page: %s\n", help_data);
#endif  /*  DEBUG_HELP  */

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
gimp_context_help (void)
{
  if (tips_query)
    gimp_help_tips_query_start (NULL, tips_query);
}

/*********************/
/*  local functions  */
/*********************/

static void
gimp_help_callback (GtkWidget *widget,
		    gpointer   data)
{
  GimpHelpFunc help_function;
  gchar *help_data;

  help_function = (GimpHelpFunc) data;
  help_data     = (gchar *) gtk_object_get_data (GTK_OBJECT (widget),
						 "gimp_help_data");

  if (help_function && use_help)
    (* help_function) (help_data);
}

/*  Do all the actual GtkTipsQuery calls in idle functions and check for
 *  some widget holding a grab before starting the query because strange
 *  things happen if (1) the help browser pops up while the query has
 *  grabbed the pointer or (2) the query grabs the pointer while some
 *  other part of the gimp has grabbed it (e.g. a tool, eek)
 */

static gint
gimp_help_tips_query_idle_show_help (gpointer data)
{
  GtkWidget *event_widget;
  GtkWidget *toplevel_widget;
  GtkWidget *widget;

  GtkTooltipsData *tooltips_data;

  gchar *help_data = NULL;

  event_widget = GTK_WIDGET (data);
  toplevel_widget = gtk_widget_get_toplevel (event_widget);

  /*  search for help_data in this widget's parent containers  */
  for (widget = event_widget; widget; widget = widget->parent)
    {
      if ((tooltips_data = gtk_tooltips_data_get (widget)) &&
	  tooltips_data->tip_private)
	{
	  help_data = tooltips_data->tip_private;
	}
      else
	{
	  help_data = (gchar *) gtk_object_get_data (GTK_OBJECT (widget),
						     "gimp_help_data");
	}

      if (help_data || widget == toplevel_widget)
	break;
    }

  if (! help_data)
    return FALSE;

  if (help_data[0] == '#')
    {
      gchar *help_index;

      if (widget == toplevel_widget)
	return FALSE;

      help_index = help_data;
      help_data  = NULL;

      for (widget = widget->parent; widget; widget = widget->parent)
	{
	  if ((tooltips_data = gtk_tooltips_data_get (widget)) &&
	      tooltips_data->tip_private)
	    {
	      help_data = tooltips_data->tip_private;
	    }
	  else
	    {
	      help_data = (gchar *) gtk_object_get_data (GTK_OBJECT (widget),
							 "gimp_help_data");
	    }

	  if (help_data)
	    break;
	}

      if (help_data)
	{
	  gchar *help_text;

	  help_text = g_strconcat (help_data, help_index, NULL);
	  gimp_help (help_text);
	  g_free (help_text);
	}
    }
  else
    {
      gimp_help (help_data);
    }

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
  if (use_help && widget)
    gtk_idle_add ((GtkFunction) gimp_help_tips_query_idle_show_help,
		  (gpointer) widget);

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
  if (use_help && ! GTK_TIPS_QUERY (tips_query)->in_query)
    gtk_idle_add ((GtkFunction) gimp_help_tips_query_idle_start, tips_query);
}
