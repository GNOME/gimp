/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1998 Andy Thomas.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURIGHTE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "appenv.h"
#include "context_manager.h"
#include "dialog_handler.h"
#include "gimpcontainer.h"
#include "gimpcontainerlistview.h"
#include "gimpcontext.h"
#include "gimpdatafactory.h"
#include "gimpdnd.h"
#include "gimpgradient.h"
#include "gradient_editor.h"
#include "gradients.h"
#include "gradient_select.h"
#include "session.h"

#include "pdb/procedural_db.h"

#include "libgimp/gimpintl.h"


static void gradient_select_change_callbacks     (GradientSelect *gsp,
						  gboolean        closing);
static void gradient_select_drop_gradient        (GtkWidget      *widget,
						  GimpViewable   *viewable,
						  gpointer        data);
static void gradient_select_gradient_changed     (GimpContext    *context,
						  GimpGradient   *gradient,
						  GradientSelect *gsp);
static void gradient_select_close_callback       (GtkWidget      *widget,
						  gpointer        data);
static void gradient_select_edit_callback        (GtkWidget      *widget,
						  gpointer        data);


/*  list of active dialogs   */
GSList         *gradient_active_dialogs = NULL;

/*  the main gradient selection dialog  */
GradientSelect *gradient_select_dialog  = NULL;


void
gradient_dialog_create (void)
{
  if (! gradient_select_dialog)
    {
      gradient_select_dialog = gradient_select_new (NULL, NULL);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (gradient_select_dialog->shell))
	gtk_widget_show (gradient_select_dialog->shell);
      else
	gdk_window_raise (gradient_select_dialog->shell->window);
    }
}

void
gradient_dialog_free (void)
{
  if (gradient_select_dialog)
    {
      session_get_window_info (gradient_select_dialog->shell,
			       &gradient_select_session_info);

      gradient_select_free (gradient_select_dialog);
      gradient_select_dialog = NULL;

    }

  gradient_editor_free ();
}

/*  If title == NULL then it is the main gradient select dialog  */
GradientSelect *
gradient_select_new (gchar *title,
		     gchar *initial_gradient)
{
  GradientSelect  *gsp;
  GtkWidget       *vbox;
  GimpGradient    *active = NULL;

  static gboolean  first_call = TRUE;

  gsp = g_new0 (GradientSelect, 1);
  
  /*  The shell  */
  gsp->shell = gimp_dialog_new (title ? title : _("Gradient Selection"),
				"gradient_selection",
				gimp_standard_help_func,
				"dialogs/gradient_selection.html",
				title ? GTK_WIN_POS_MOUSE : GTK_WIN_POS_NONE,
				FALSE, TRUE, FALSE,

				_("Edit"), gradient_select_edit_callback,
				gsp, NULL, NULL, FALSE, FALSE,
				_("Close"), gradient_select_close_callback,
				gsp, NULL, NULL, TRUE, TRUE,

				NULL);

  if (title)
    {
      gsp->context = gimp_context_new (title, NULL);
    }
  else
    {
      gsp->context = gimp_context_get_user ();

      session_set_window_geometry (gsp->shell, &gradient_select_session_info,
				   TRUE);
      dialog_register (gsp->shell);
    }

  if (no_data && first_call)
    gradients_init (FALSE);

  first_call = FALSE;

  if (title && initial_gradient && strlen (initial_gradient))
    {
      active = (GimpGradient *)
	gimp_container_get_child_by_name (global_gradient_factory->container,
					  initial_gradient);
    }
  else
    {
      active = gimp_context_get_gradient (gimp_context_get_user ());
    }

  if (!active)
    active = gimp_context_get_gradient (gimp_context_get_standard ());

  if (title)
    gimp_context_set_gradient (gsp->context, active);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (gsp->shell)->vbox), vbox);

  /*  The Gradient List  */
  gsp->view = gimp_container_list_view_new (global_gradient_factory->container,
					    gsp->context,
					    16,
					    10, 10);
  gtk_box_pack_start (GTK_BOX (vbox), gsp->view, TRUE, TRUE, 0);
  gtk_widget_show (gsp->view);

  gimp_gtk_drag_dest_set_by_type (gsp->view,
                                  GTK_DEST_DEFAULT_ALL,
                                  GIMP_TYPE_GRADIENT,
                                  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (gsp->view),
                              GIMP_TYPE_GRADIENT,
                              gradient_select_drop_gradient,
                              gsp);

  gtk_widget_show (vbox);

  gtk_widget_show (gsp->shell);

  gtk_signal_connect (GTK_OBJECT (gsp->context), "gradient_changed",
                      GTK_SIGNAL_FUNC (gradient_select_gradient_changed),
                      (gpointer) gsp);

  /*  Add to active gradient dialogs list  */
  gradient_active_dialogs = g_slist_append (gradient_active_dialogs, gsp);

  return gsp;
}

void
gradient_select_free (GradientSelect *gsp)
{
  if (!gsp)
    return;

  /* remove from active list */
  gradient_active_dialogs = g_slist_remove (gradient_active_dialogs, gsp);

  gtk_signal_disconnect_by_data (GTK_OBJECT (gsp->context), gsp);

  if (gsp->callback_name)
    { 
      g_free (gsp->callback_name);
      gtk_object_unref (GTK_OBJECT (gsp->context));
    }
 
   g_free (gsp);
}

static void
gradient_select_change_callbacks (GradientSelect *gsp,
				  gboolean        closing)
{
  gchar           *name;
  ProcRecord      *prec = NULL;
  GimpGradient    *gradient;
  gint             nreturn_vals;
  static gboolean  busy = FALSE;

  /* Any procs registered to callback? */
  Argument *return_vals; 

  if (!gsp || !gsp->callback_name || busy != 0)
    return;

  busy = TRUE;

  name = gsp->callback_name;
  gradient = gimp_context_get_gradient (gsp->context);

  /*  If its still registered run it  */
  prec = procedural_db_lookup (name);

  if (prec && gradient)
    {
      gdouble  *values, *pv;
      double    pos, delta;
      GimpRGB   color;
      gint      i;

      i      = gsp->sample_size;
      pos    = 0.0;
      delta  = 1.0 / (i - 1);
      
      values = g_new (gdouble, 4 * i);
      pv     = values;

      while (i--)
	{
	  gimp_gradient_get_color_at (gradient, pos, &color);

	  *pv++ = color.r;
	  *pv++ = color.g;
	  *pv++ = color.b;
	  *pv++ = color.a;

	  pos += delta;
	}

      return_vals =
	procedural_db_run_proc (name,
				&nreturn_vals,
				PDB_STRING,     GIMP_OBJECT (gradient)->name,
				PDB_INT32,      gsp->sample_size * 4,
				PDB_FLOATARRAY, values,
				PDB_INT32,      (gint) closing,
				PDB_END);
 
      if (!return_vals || return_vals[0].value.pdb_int != PDB_SUCCESS)
	g_message ("failed to run gradient callback function");
      else
	procedural_db_destroy_args (return_vals, nreturn_vals);
    }

  busy = FALSE;
}

/*  Close active dialogs that no longer have PDB registered for them  */

void
gradient_select_dialogs_check (void)
{
  GradientSelect *gsp;
  GSList         *list;
  gchar          *name;
  ProcRecord     *prec = NULL;

  list = gradient_active_dialogs;

  while (list)
    {
      gsp = (GradientSelect *) list->data;
      list = g_slist_next (list);

      name = gsp->callback_name;

      if (name)
	{
	  prec = procedural_db_lookup (name);

	  if (!prec)
	    {
	      /*  Can alter gradient_active_dialogs list  */
	      gradient_select_close_callback (NULL, gsp); 
	    }
	}
    }
}

static void
gradient_select_drop_gradient (GtkWidget    *widget,
			       GimpViewable *viewable,
			       gpointer      data)
{
  GradientSelect *gsp;

  gsp = (GradientSelect *) data;

  gimp_context_set_gradient (gsp->context, GIMP_GRADIENT (viewable));
}

static void
gradient_select_gradient_changed (GimpContext    *context,
				  GimpGradient   *gradient,
				  GradientSelect *gsp)
{
  if (gradient)
    {
      if (gsp->callback_name)
	gradient_select_change_callbacks (gsp, FALSE);
    }
}

static void
gradient_select_edit_callback (GtkWidget *widget,
			       gpointer   data)
{
  GradientSelect *gsp;

  gsp = (GradientSelect *) data;

  gradient_editor_create ();

  gradient_editor_set_gradient (gimp_context_get_gradient (gsp->context));
}

static void
gradient_select_close_callback (GtkWidget *widget,
				gpointer   data)
{
  GradientSelect *gsp;

  gsp = (GradientSelect *) data;

  if (GTK_WIDGET_VISIBLE (gsp->shell))
    gtk_widget_hide (gsp->shell);

  /* Free memory if poping down dialog which is not the main one */
  if (gsp != gradient_select_dialog)
    {
      gradient_select_change_callbacks (gsp, TRUE);
      gtk_widget_destroy (gsp->shell);
      gradient_select_free (gsp);
    }
}
