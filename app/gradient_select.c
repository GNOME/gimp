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

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "appenv.h"
#include "colormaps.h"
#include "cursorutil.h"
#include "datafiles.h"
#include "dialog_handler.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"
#include "gimpui.h"
#include "gradient.h"
#include "gradient_header.h"
#include "indicator_area.h"
#include "interface.h"
#include "palette.h"
#include "session.h"

#include "libgimp/gimpintl.h"

GSList *grad_active_dialogs = NULL; /* List of active dialogs */
GradSelectP gradient_select_dialog = NULL; /* The main selection dialog */

static void grad_select_close_callback (GtkWidget *, gpointer);
static void grad_select_edit_callback  (GtkWidget *, gpointer);
static void grad_change_callbacks      (GradSelectP gsp, gint closing);
extern void import_palette_grad_update (gradient_t *); /* ALT Hmm... */

void
grad_free_gradient_editor (void)
{
  if (gradient_select_dialog)
    session_get_window_info (gradient_select_dialog->shell,
			     &gradient_select_session_info);
}

void
grad_sel_rename_all (gint        n,
		     gradient_t *grad)
{
  GSList *list = grad_active_dialogs;
  GradSelectP gsp; 

  while (list)
    {
      gsp = (GradSelectP) list->data;
      gtk_clist_set_text (GTK_CLIST (gsp->clist), n, 1, grad->name);  
      list = g_slist_next (list);
    }

  if(gradient_select_dialog)
    {
      gtk_clist_set_text (GTK_CLIST (gradient_select_dialog->clist),
			  n, 1, grad->name);  
    }
}

void
grad_sel_new_all (gint        pos,
		  gradient_t *grad)
{
  GSList *list = grad_active_dialogs;
  GradSelectP gsp; 

  while (list)
    {
      gsp = (GradSelectP) list->data;
      gtk_clist_freeze (GTK_CLIST (gsp->clist));
      ed_insert_in_gradients_listbox (gsp->gc, gsp->clist, grad, pos, 1);
      gtk_clist_thaw (GTK_CLIST (gsp->clist));
      list = g_slist_next (list);
    }

  if (gradient_select_dialog)
    {
      gtk_clist_freeze (GTK_CLIST (gradient_select_dialog->clist));
      ed_insert_in_gradients_listbox (gradient_select_dialog->gc,
				      gradient_select_dialog->clist,
				      grad, pos, 1);
      gtk_clist_thaw (GTK_CLIST (gradient_select_dialog->clist));
    }
}

void
grad_sel_copy_all (gint        pos,
		   gradient_t *grad)
{
  GSList *list = grad_active_dialogs;
  GradSelectP gsp; 

  while (list)
    {
      gsp = (GradSelectP) list->data;
      gtk_clist_freeze (GTK_CLIST (gsp->clist));
      ed_insert_in_gradients_listbox (gsp->gc, gsp->clist, grad, pos, 1);
      gtk_clist_thaw (GTK_CLIST (gsp->clist));
      list = g_slist_next (list);
    }

  if (gradient_select_dialog)
    {
      gtk_clist_freeze (GTK_CLIST (gradient_select_dialog->clist));
      ed_insert_in_gradients_listbox (gradient_select_dialog->gc,
				      gradient_select_dialog->clist,
				      grad, pos, 1);
      gtk_clist_thaw (GTK_CLIST (gradient_select_dialog->clist));
    }
}

void
grad_sel_delete_all (gint n)
{
  GSList *list = grad_active_dialogs;
  GradSelectP gsp; 

  while (list)
    {
      gsp = (GradSelectP) list->data;
      gtk_clist_remove (GTK_CLIST (gsp->clist), n);
      list = g_slist_next (list);
    }

  if (gradient_select_dialog)
    {
      gtk_clist_remove (GTK_CLIST (gradient_select_dialog->clist), n);
    }
}

void
grad_sel_free_all()
{
  GSList *list = grad_active_dialogs;
  GradSelectP gsp; 

  while (list)
    {
      gsp = (GradSelectP) list->data;
      gtk_clist_freeze (GTK_CLIST (gsp->clist));
      gtk_clist_clear (GTK_CLIST (gsp->clist));
      gtk_clist_thaw (GTK_CLIST (gsp->clist));
      list = g_slist_next (list);
    }

  if (gradient_select_dialog)
    {
      gtk_clist_freeze (GTK_CLIST (gradient_select_dialog->clist));
      gtk_clist_clear (GTK_CLIST (gradient_select_dialog->clist));
      gtk_clist_thaw (GTK_CLIST (gradient_select_dialog->clist));
    }
}

void
grad_sel_refill_all ()
{
  GSList *list = grad_active_dialogs;
  GradSelectP gsp; 
  int select_pos = -1;

  while (list)
    {
      gsp = (GradSelectP) list->data;
      gsp->grad = curr_gradient;
      select_pos = ed_set_list_of_gradients (gsp->gc, 
					     gsp->clist, 
					     curr_gradient); 
      if (select_pos != -1) 
	gtk_clist_moveto (GTK_CLIST (gsp->clist), select_pos, 0, 0.0, 0.0); 

      list = g_slist_next (list);
    }

  if (gradient_select_dialog)
    {
      gradient_select_dialog->grad = curr_gradient;
      select_pos = ed_set_list_of_gradients (gradient_select_dialog->gc, 
					     gradient_select_dialog->clist, 
					     curr_gradient); 
      if (select_pos != -1) 
	gtk_clist_moveto (GTK_CLIST (gradient_select_dialog->clist),
			  select_pos, 0, 0.0, 0.0); 
    }
}

void
sel_update_dialogs (gint        row,
		    gradient_t *grad)
{
  /* Go around each updating the names and hopefully the previews */
  GSList *list = grad_active_dialogs;
  GradSelectP gsp; 

  while (list)
    {
      gsp = (GradSelectP) list->data;
      gtk_clist_set_text (GTK_CLIST (gsp->clist), row, 1, grad->name);  
      /* Are we updating one that is selected in a popup dialog? */
      if (grad == gsp->grad)
	grad_change_callbacks (gsp, 0);
      list = g_slist_next (list);
    }

  if (gradient_select_dialog)
    gtk_clist_set_text (GTK_CLIST (gradient_select_dialog->clist),
			row, 1, grad->name);  

  gradient_area_update ();  /*  update the indicator_area  */
  import_palette_grad_update (grad);
}

static void
sel_list_item_update (GtkWidget      *widget, 
		      gint            row,
		      gint            column,
		      GdkEventButton *event,
		      gpointer        data)
{
  GradSelectP gsp = (GradSelectP) data;

  GSList* tmp = g_slist_nth (gradients_list, row);
  gsp->grad = (gradient_t *) (tmp->data);

  /* If main one then make it the current selection */
  if (gsp == gradient_select_dialog)
    {
      grad_set_grad_to_name (gsp->grad->name);
      gradient_area_update ();  /*  update the indicator_area  */
      import_palette_grad_update (gsp->grad);
    }
  else
    {
      grad_change_callbacks (gsp, 0);
    }
}

static void
grad_select_edit_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GradSelectP gsp;

  gsp = (GradSelectP) client_data;

  grad_create_gradient_editor_init (TRUE);

  /* Set the current gradient in this dialog to the "real current"*/
  if (gsp && gsp->grad)
    grad_set_grad_to_name (gsp->grad->name);
}

void
grad_select_free (GradSelectP gsp)
{
  if (gsp)
    {
      if (gsp->callback_name)
	g_free (gsp->callback_name);

      /* remove from active list */
      grad_active_dialogs = g_slist_remove (grad_active_dialogs, gsp);

      g_free (gsp);
    }
}

/* Close active dialogs that no longer have PDB registered for them */
void
gradients_check_dialogs (void)
{
  GSList *list;
  GradSelectP gsp;
  gchar * name;
  ProcRecord *prec = NULL;

  list = grad_active_dialogs;

  while (list)
    {
      gsp = (GradSelectP) list->data;
      list = list->next;

      name = gsp->callback_name;
      prec = procedural_db_lookup (name);

      if (!prec)
	{
	  grad_active_dialogs = g_slist_remove (grad_active_dialogs, gsp);

	  /* Can alter grad_active_dialogs list*/
	  grad_select_close_callback (NULL, gsp); 
	}
    }
}

static void
grad_change_callbacks(GradSelectP gsp,
		      gint        closing)
{
  gchar * name;
  ProcRecord *prec = NULL;
  gradient_t *grad;
  int nreturn_vals;
  static int busy = 0;
  gradient_t *oldgrad = curr_gradient;

  /* Any procs registered to callback? */
  Argument *return_vals; 

  if (!gsp || !gsp->callback_name || busy != 0)
    return;

  busy = 1;
  name = gsp->callback_name;
  grad = gsp->grad;

  /* If its still registered run it */
  prec = procedural_db_lookup (name);

  if (prec && grad)
    {
      gdouble  *values, *pv;
      double    pos, delta;
      double    r, g, b, a;
      int i = gsp->sample_size;
      pos   = 0.0;
      delta = 1.0 / (i - 1);
      
      values = g_new (gdouble, 4 * i);
      pv     = values;

      curr_gradient = grad;
      
      while (i--)
	{
	  grad_get_color_at (pos, &r, &g, &b, &a);

	  *pv++ = r;
	  *pv++ = g;
	  *pv++ = b;
	  *pv++ = a;

	  pos += delta;
	}

      curr_gradient = oldgrad;

      return_vals = procedural_db_run_proc (name,
					    &nreturn_vals,
					    PDB_STRING, grad->name,
					    PDB_INT32, gsp->sample_size*4,
					    PDB_FLOATARRAY, values,
					    PDB_INT32, closing,
					    PDB_END);
 
      if (!return_vals || return_vals[0].value.pdb_int != PDB_SUCCESS)
	g_message ("failed to run gradient callback function");
      else
	procedural_db_destroy_args (return_vals, nreturn_vals);
    }
  busy = 0;
}

static void
grad_select_close_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GradSelectP gsp;

  gsp = (GradSelectP) client_data;

  if (GTK_WIDGET_VISIBLE (gsp->shell))
    gtk_widget_hide (gsp->shell);

  /* Free memory if poping down dialog which is not the main one */
  if(gsp != gradient_select_dialog)
    {
      grad_change_callbacks(gsp,1);
      gtk_widget_destroy(gsp->shell); 
      grad_select_free(gsp); 
    }
}

GradSelectP
gsel_new_selection (gchar *title,
		    gchar *initial_gradient)
{
  GradSelectP  gsp;
  gradient_t  *grad = NULL;
  GSList      *list;
  GtkWidget   *vbox;
  GtkWidget   *hbox;
  GtkWidget   *scrolled_win;
  GdkColormap *colormap;
  gint         select_pos;

  /* Load them if they are not already loaded */
  if (g_editor == NULL)
    {
      grad_create_gradient_editor_init(FALSE);
    }

  gsp = g_new (_GradSelect, 1);
  gsp->callback_name = NULL;
  
  /*  The shell and main vbox  */
  gsp->shell = gimp_dialog_new (title ? title : _("Gradient Selection"),
				"gradient_selection",
				gimp_standard_help_func,
				"dialogs/gradient_selection.html",
				GTK_WIN_POS_NONE,
				FALSE, TRUE, FALSE,

				_("Edit"), grad_select_edit_callback,
				gsp, NULL, FALSE, FALSE,
				_("Close"), grad_select_close_callback,
				gsp, NULL, TRUE, TRUE,

				NULL);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 1);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (gsp->shell)->vbox), vbox);

  /* clist preview of gradients */
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);

  gsp->clist = gtk_clist_new (2);
  gtk_clist_set_shadow_type (GTK_CLIST (gsp->clist), GTK_SHADOW_IN);

  gtk_clist_set_row_height (GTK_CLIST (gsp->clist), 18);

  gtk_clist_set_column_width (GTK_CLIST (gsp->clist), 0, 52);
  gtk_clist_set_column_title (GTK_CLIST (gsp->clist), 0, _("Gradient"));
  gtk_clist_set_column_title (GTK_CLIST (gsp->clist), 1, _("Name"));
  gtk_clist_column_titles_show (GTK_CLIST (gsp->clist));
  
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0); 
  gtk_container_add (GTK_CONTAINER (scrolled_win), gsp->clist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);

  gtk_widget_show (scrolled_win);
  gtk_widget_show (gsp->clist);
  gtk_widget_set_usize (gsp->clist, 200, 250);

  colormap = gtk_widget_get_colormap (gsp->clist);
  gdk_color_parse ("black", &gsp->black);
  gdk_color_alloc (colormap, &gsp->black);

  gtk_signal_connect (GTK_OBJECT (gsp->clist), "select_row",
		      GTK_SIGNAL_FUNC (sel_list_item_update),
		      (gpointer) gsp);

  if (initial_gradient && strlen (initial_gradient))
    {
      list = gradients_list;
      while (list) 
	{
	  grad = list->data;
	  if (strcmp (grad->name, initial_gradient) == 0) 
	    {
	      /* We found it! */
	      break;
	    }
	  list = g_slist_next (list);
	} 
    }

  if (grad == NULL)
    grad = curr_gradient;

  gsp->grad = grad;

  gtk_widget_realize (gsp->shell);
  gsp->gc = gdk_gc_new (gsp->shell->window);
  
  select_pos = ed_set_list_of_gradients (gsp->gc, 
					 gsp->clist, 
					 grad); 

  /* Now show the dialog */
  gtk_widget_show (vbox);
  gtk_widget_show (gsp->shell);

  if (select_pos != -1) 
    gtk_clist_moveto (GTK_CLIST (gsp->clist), select_pos, 0, 0.0, 0.0); 

  return gsp;
}

void
grad_create_gradient_editor (void)
{
  if (gradient_select_dialog == NULL)
    {
      gradient_select_dialog = gsel_new_selection (_("Gradients"), NULL);

      /* register this one only */
      dialog_register (gradient_select_dialog->shell);

      session_set_window_geometry (gradient_select_dialog->shell,
				   &gradient_select_session_info, TRUE);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (gradient_select_dialog->shell))
	{
	  gtk_widget_show (gradient_select_dialog->shell);
	}
      else
	gdk_window_raise (gradient_select_dialog->shell->window);
      return;
    }
}
