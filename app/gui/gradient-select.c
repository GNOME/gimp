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
#include "widgets/widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"

#include "pdb/procedural_db.h"

#include "widgets/gimpdatafactoryview.h"

#include "dialog_handler.h"
#include "dialogs-constructors.h"
#include "gradient-select.h"

#include "appenv.h"
#include "context_manager.h"

#include "libgimp/gimpintl.h"


static void gradient_select_change_callbacks     (GradientSelect *gsp,
						  gboolean        closing);
static void gradient_select_gradient_changed     (GimpContext    *context,
						  GimpGradient   *gradient,
						  GradientSelect *gsp);
static void gradient_select_close_callback       (GtkWidget      *widget,
						  gpointer        data);


/*  list of active dialogs   */
GSList *gradient_active_dialogs = NULL;

/*  the main gradient selection dialog  */
GradientSelect *gradient_select_dialog = NULL;


/*  public functions  */

GtkWidget *
gradient_dialog_create (void)
{
  if (! gradient_select_dialog)
    {
      gradient_select_dialog = gradient_select_new (NULL, NULL);
    }

  return gradient_select_dialog->shell;
}

void
gradient_dialog_free (void)
{
  if (gradient_select_dialog)
    {
      gradient_select_free (gradient_select_dialog);
      gradient_select_dialog = NULL;

    }
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

				"_delete_event_", gradient_select_close_callback,
				gsp, NULL, NULL, FALSE, TRUE,

				NULL);

  gtk_widget_hide (GTK_WIDGET (g_list_nth_data (gtk_container_children (GTK_CONTAINER (GTK_DIALOG (gsp->shell)->vbox)), 0)));

  gtk_widget_hide (GTK_DIALOG (gsp->shell)->action_area);

  if (title)
    {
      gsp->context = gimp_context_new (title, NULL);
    }
  else
    {
      gsp->context = gimp_context_get_user ();

      dialog_register (gsp->shell);
    }

  if (no_data && first_call)
    gimp_data_factory_data_init (global_gradient_factory, FALSE);

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
  gsp->view = gimp_data_factory_view_new (GIMP_VIEW_TYPE_LIST,
					  global_gradient_factory,
					  dialogs_edit_gradient_func,
					  gsp->context,
					  16,
					  10, 10);
  gtk_box_pack_start (GTK_BOX (vbox), gsp->view, TRUE, TRUE, 0);
  gtk_widget_show (gsp->view);

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
				GIMP_PDB_STRING,     GIMP_OBJECT (gradient)->name,
				GIMP_PDB_INT32,      gsp->sample_size * 4,
				GIMP_PDB_FLOATARRAY, values,
				GIMP_PDB_INT32,      (gint) closing,
				GIMP_PDB_END);
 
      if (!return_vals || return_vals[0].value.pdb_int != GIMP_PDB_SUCCESS)
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
