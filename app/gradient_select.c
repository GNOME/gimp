/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1998 Andy Thomas.
 *
 * Gradient editor module copyight (C) 1996-1997 Federico Mena Quintero
 * federico@nuclecu.unam.mx
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

/* This is the popup for the gradient selection stuff..
 * idea is a cut down version of the gradient selection widget 
 * just a clist on which each gradient can be selected. 
 * Of course all the support functions for getting the slected gradient and
 * setting the slection all need to be done as well.
 */

/* Main structure for the dialog. There can be multiple of these 
 * so every thing has to go into the strcuture and we have to have a list 
 * the structures so we can find which one we are taking about.
 */
#include <string.h>

#include "appenv.h"
#include "dialog_handler.h"
#include "gimpcontext.h"
#include "gimpdnd.h"
#include "gimpui.h"
#include "gradient_header.h"
#include "gradientP.h"
#include "gradient_select.h"
#include "session.h"

#include "libgimp/gimpintl.h"

static void gradient_change_callbacks            (GradientSelect *gsp,
						  gboolean        closing);
static gradient_t* gradient_select_drag_gradient (GtkWidget      *widget,
					          gpointer        data);
static void gradient_select_drop_gradient        (GtkWidget      *widget,
						  gradient_t     *gradient,
						  gpointer        data);
static void gradient_select_gradient_changed     (GimpContext    *context,
						  gradient_t     *gradient,
						  GradientSelect *gsp);
static void gradient_select_select               (GradientSelect *gsp,
						  gradient_t     *gradient);

static gint gradient_select_button_press         (GtkWidget      *widget,
						  GdkEventButton *bevent,
						  gpointer        data);
static void gradient_select_list_item_update     (GtkWidget      *widget, 
						  gint            row,
						  gint            column,
						  GdkEventButton *event,
						  gpointer        data);

static void gradient_select_close_callback       (GtkWidget      *widget,
						  gpointer        data);
static void gradient_select_edit_callback        (GtkWidget      *widget,
						  gpointer        data);

/*  dnd stuff  */
static GtkTargetEntry clist_target_table[] =
{
  GIMP_TARGET_GRADIENT
};
static guint clist_n_targets = (sizeof (clist_target_table) /
				sizeof (clist_target_table[0]));

/*  list of active dialogs   */
GSList *gradient_active_dialogs = NULL;

/*  the main gradient selection dialog  */
GradientSelect *gradient_select_dialog = NULL;

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
  GradientSelect *gsp;
  GtkWidget   *vbox;
  GtkWidget   *scrolled_win;
  GdkColormap *colormap;
  gchar       *titles[2];
  gint         column_width;
  gint         select_pos;

  gradient_t *active = NULL;

  static gboolean first_call = TRUE;

  gsp = g_new (GradientSelect, 1);
  gsp->callback_name = NULL;
  gsp->dnd_gradient = NULL;
  
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
      active = gradient_list_get_gradient (gradients_list, initial_gradient);
    }
  else
    {
      active = gimp_context_get_gradient (gimp_context_get_user ());
    }

  if (!active)
    {
      active = gimp_context_get_gradient (gimp_context_get_standard ());
    }

  if (title)
    {
      gimp_context_set_gradient (gsp->context, active);
    }

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (gsp->shell)->vbox), vbox);

  /*  clist preview of gradients  */
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (vbox), scrolled_win);
  gtk_widget_show (scrolled_win);

  titles[0] = _("Gradient");
  titles[1] = _("Name");
  gsp->clist = gtk_clist_new_with_titles (2, titles);
  gtk_clist_set_shadow_type (GTK_CLIST (gsp->clist), GTK_SHADOW_IN);
  gtk_clist_set_selection_mode (GTK_CLIST (gsp->clist), GTK_SELECTION_BROWSE);
  gtk_clist_set_row_height (GTK_CLIST (gsp->clist), 18);
  gtk_clist_set_use_drag_icons (GTK_CLIST (gsp->clist), FALSE);
  gtk_clist_column_titles_passive (GTK_CLIST (gsp->clist));
  gtk_widget_set_usize (gsp->clist, 200, 250);
  gtk_container_add (GTK_CONTAINER (scrolled_win), gsp->clist);

  column_width =
    MAX (50, gtk_clist_optimal_column_width (GTK_CLIST (gsp->clist), 0));
  gtk_clist_set_column_min_width (GTK_CLIST (gsp->clist), 0, 50);
  gtk_clist_set_column_width (GTK_CLIST (gsp->clist), 0, column_width);

  gtk_widget_show (gsp->clist);

  colormap = gtk_widget_get_colormap (gsp->clist);
  gdk_color_parse ("black", &gsp->black);
  gdk_color_alloc (colormap, &gsp->black);

  gtk_widget_realize (gsp->shell);
  gsp->gc = gdk_gc_new (gsp->shell->window);

  select_pos = gradient_clist_init (gsp->shell, gsp->gc, gsp->clist, active);

  /* Now show the dialog */
  gtk_widget_show (vbox);
  gtk_widget_show (gsp->shell);

  gtk_signal_connect (GTK_OBJECT (gsp->clist), "button_press_event",
		      GTK_SIGNAL_FUNC (gradient_select_button_press),
		      (gpointer) gsp);

  gtk_signal_connect (GTK_OBJECT (gsp->clist), "select_row",
		      GTK_SIGNAL_FUNC (gradient_select_list_item_update),
		      (gpointer) gsp);

  gtk_signal_connect (GTK_OBJECT (gsp->context), "gradient_changed",
                      GTK_SIGNAL_FUNC (gradient_select_gradient_changed),
                      (gpointer) gsp);

  /*  dnd stuff  */
  gtk_drag_source_set (gsp->clist,
		       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
		       clist_target_table, clist_n_targets,
		       GDK_ACTION_COPY);
  gimp_dnd_gradient_source_set (gsp->clist, gradient_select_drag_gradient, gsp);

  gtk_drag_dest_set (gsp->clist,
                     GTK_DEST_DEFAULT_ALL,
                     clist_target_table, clist_n_targets,
                     GDK_ACTION_COPY);
  gimp_dnd_gradient_dest_set (gsp->clist, gradient_select_drop_gradient, gsp);

  if (active)
    gradient_select_select (gsp, active);

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

/*  Call this dialog's PDB callback  */

static void
gradient_change_callbacks (GradientSelect *gsp,
			   gboolean        closing)
{
  gchar * name;
  ProcRecord *prec = NULL;
  gradient_t *gradient;
  gint nreturn_vals;
  static gboolean busy = FALSE;

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
      double    r, g, b, a;
      int i = gsp->sample_size;
      pos   = 0.0;
      delta = 1.0 / (i - 1);
      
      values = g_new (gdouble, 4 * i);
      pv     = values;

      while (i--)
	{
	  gradient_get_color_at (gradient, pos, &r, &g, &b, &a);

	  *pv++ = r;
	  *pv++ = g;
	  *pv++ = b;
	  *pv++ = a;

	  pos += delta;
	}

      return_vals = procedural_db_run_proc (name,
					    &nreturn_vals,
					    PDB_STRING,     gradient->name,
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
gradients_check_dialogs (void)
{
  GradientSelect *gsp;
  GSList *list;
  gchar *name;
  ProcRecord *prec = NULL;

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

void
gradient_select_rename_all (gint        n,
			    gradient_t *gradient)
{
  GradientSelect *gsp; 
  GSList *list;

  for (list = gradient_active_dialogs; list; list = g_slist_next (list))
    {
      gsp = (GradientSelect *) list->data;

      gtk_clist_set_text (GTK_CLIST (gsp->clist), n, 1, gradient->name);  
    }
}

void
gradient_select_insert_all (gint        pos,
			    gradient_t *gradient)
{
  GradientSelect *gsp; 
  GSList *list;

  for (list = gradient_active_dialogs; list; list = g_slist_next (list))
    {
      gsp = (GradientSelect *) list->data;

      gtk_clist_freeze (GTK_CLIST (gsp->clist));
      gradient_clist_insert (gsp->shell, gsp->gc, gsp->clist,
			     gradient, pos, FALSE);
      gtk_clist_thaw (GTK_CLIST (gsp->clist));
    }
}

void
gradient_select_delete_all (gint n)
{
  GradientSelect *gsp; 
  GSList *list;

  for (list = gradient_active_dialogs; list; list = g_slist_next (list))
    {
      gsp = (GradientSelect *) list->data;

      gtk_clist_remove (GTK_CLIST (gsp->clist), n);
    }
}

void
gradient_select_free_all (void)
{
  GradientSelect *gsp; 
  GSList *list;

  for (list = gradient_active_dialogs; list; list = g_slist_next (list))
    {
      gsp = (GradientSelect *) list->data;

      gtk_clist_freeze (GTK_CLIST (gsp->clist));
      gtk_clist_clear (GTK_CLIST (gsp->clist));
      gtk_clist_thaw (GTK_CLIST (gsp->clist));
    }
}

void
gradient_select_refill_all (void)
{
  GradientSelect *gsp;
  GSList *list;
  gint index = -1;

  for (list = gradient_active_dialogs; list; list = g_slist_next (list))
    {
      gsp = (GradientSelect *) list->data;

      index = gradient_clist_init (gsp->shell, gsp->gc, gsp->clist, 
				   gimp_context_get_gradient (gsp->context));

      if (index != -1)
	gradient_select_select (gsp, gimp_context_get_gradient (gsp->context));
    }
}

void
gradient_select_update_all (gint        row,
			    gradient_t *gradient)
{
  GSList *list;
  GradientSelect *gsp; 

  for (list = gradient_active_dialogs; list; list = g_slist_next (list))
    {
      gsp = (GradientSelect *) list->data;

      gtk_clist_set_text (GTK_CLIST (gsp->clist), row, 1, gradient->name);  
    }
}

/*
 *  Local functions
 */

static gradient_t *
gradient_select_drag_gradient (GtkWidget  *widget,
			       gpointer    data)
{
  GradientSelect *gsp;

  gsp = (GradientSelect *) data;

  return gsp->dnd_gradient;
}

static void
gradient_select_drop_gradient (GtkWidget  *widget,
			       gradient_t *gradient,
			       gpointer    data)
{
  GradientSelect *gsp;

  gsp = (GradientSelect *) data;

  gimp_context_set_gradient (gsp->context, gradient);
}

static void
gradient_select_gradient_changed (GimpContext    *context,
				  gradient_t     *gradient,
				  GradientSelect *gsp)
{
  if (gradient)
    {
      gradient_select_select (gsp, gradient);

      if (gsp->callback_name)
	gradient_change_callbacks (gsp, FALSE);

      gsp->dnd_gradient = gradient;
    }
}

static void
gradient_select_select (GradientSelect *gsp,
			gradient_t     *gradient)
{
  gint index;

  index = gradient_list_get_gradient_index (gradients_list, gradient);

  if (index != -1)
    {
      gtk_signal_handler_block_by_data (GTK_OBJECT (gsp->clist), gsp);

      gtk_clist_select_row (GTK_CLIST (gsp->clist), index, -1);
      gtk_clist_moveto (GTK_CLIST (gsp->clist), index, 0, 0.5, 0.0);

      gtk_signal_handler_unblock_by_data (GTK_OBJECT (gsp->clist), gsp);
    }
}

static gint
gradient_select_button_press (GtkWidget      *widget,
			      GdkEventButton *bevent,
			      gpointer        data)
{
  GradientSelect *gsp;

  gsp = (GradientSelect *) data;

  if (bevent->button == 1 && bevent->type == GDK_2BUTTON_PRESS)
    {
      gradient_select_edit_callback (widget, data);

      return TRUE;
    }

  if (bevent->button == 2)
    {
      GSList *list = NULL;
      gint row;
      gint column;

      gtk_clist_get_selection_info (GTK_CLIST (gsp->clist),
				    bevent->x, bevent->y,
				    &row, &column);

      if (gradients_list)
	list = g_slist_nth (gradients_list, row);

      if (list)
	gsp->dnd_gradient = (gradient_t *) list->data;
      else
	gsp->dnd_gradient = NULL;

      return TRUE;
    }

  return FALSE;
}

static void
gradient_select_list_item_update (GtkWidget      *widget, 
				  gint            row,
				  gint            column,
				  GdkEventButton *event,
				  gpointer        data)
{
  GradientSelect *gsp;
  GSList *list;

  gsp = (GradientSelect *) data;
  list = g_slist_nth (gradients_list, row);

  gtk_signal_handler_block_by_data (GTK_OBJECT (gsp->context), gsp);

  gimp_context_set_gradient (gsp->context, (gradient_t *) list->data);

  gtk_signal_handler_unblock_by_data (GTK_OBJECT (gsp->context), gsp);

  gsp->dnd_gradient = (gradient_t *) list->data;

  if (gsp->callback_name)
    gradient_change_callbacks (gsp, FALSE);
}

static void
gradient_select_edit_callback (GtkWidget *widget,
			       gpointer   data)
{
  GradientSelect *gsp;

  gsp = (GradientSelect *) data;

  gradient_editor_create ();

  if (gsp)
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
      gradient_change_callbacks (gsp, TRUE);
      gtk_widget_destroy (gsp->shell);
      gradient_select_free (gsp);
    }
}
