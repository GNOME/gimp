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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "appenv.h"
#include "colormaps.h"
#include "cursorutil.h"
#include "datafiles.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"
#include "gradient.h"
#include "gradient_header.h"
#include "interface.h"
#include "palette.h"
#include "session.h"
#include "actionarea.h"

#include "libgimp/gimpintl.h"

#define G_SAMPLE 40

typedef struct _GradSelect _GradSelect, *GradSelectP;

struct _GradSelect {
  GtkWidget         *shell;
  GtkWidget         *frame;
  GtkWidget         *preview;
  GtkWidget         *clist;
  gchar             *callback_name;
  gradient_t        *grad;
  gint              sample_size;
  GdkColor          black;
  GdkGC             *gc;
};    

static GSList *active_dialogs = NULL; /* List of active dialogs */
GradSelectP gradient_select_dialog = NULL; /* The main selection dialog */
static int success;
static Argument    *return_args;

static void grad_select_close_callback    (GtkWidget *, gpointer);
static void grad_select_edit_callback  (GtkWidget *, gpointer);
static void grad_change_callbacks(GradSelectP gsp, gint closing);
extern void import_palette_grad_update(gradient_t *); /* ALT Hmm... */


static ActionAreaItem action_items[2] =
{
  { N_("Close"), grad_select_close_callback, NULL, NULL },
  { N_("Edit"), grad_select_edit_callback, NULL, NULL },
};

void
grad_free_gradient_editor(void)
{
  if (gradient_select_dialog)
    session_get_window_info (gradient_select_dialog->shell, &gradient_select_session_info);
} /* grad_free_gradient_editor */

void
grad_sel_rename_all(gint n, gradient_t *grad)
{
  GSList *list = active_dialogs;
  GradSelectP gsp; 

  while(list)
    {
      gsp = (GradSelectP)list->data;
      gtk_clist_set_text(GTK_CLIST(gsp->clist),n,1,grad->name);  
      list = g_slist_next(list);
    }

  if(gradient_select_dialog)
    {
      gtk_clist_set_text(GTK_CLIST(gradient_select_dialog->clist),n,1,grad->name);  
    }
}

void
grad_sel_new_all(gint pos, gradient_t *grad)
{
  GSList *list = active_dialogs;
  GradSelectP gsp; 

  while(list)
    {
      gsp = (GradSelectP)list->data;
      gtk_clist_freeze(GTK_CLIST(gsp->clist));
      ed_insert_in_gradients_listbox(gsp->gc,gsp->clist,grad, pos, 1);
      gtk_clist_thaw(GTK_CLIST(gsp->clist));
      list = g_slist_next(list);
    }

  if(gradient_select_dialog)
    {
      gtk_clist_freeze(GTK_CLIST(gradient_select_dialog->clist));
      ed_insert_in_gradients_listbox(gradient_select_dialog->gc,gradient_select_dialog->clist,grad, pos, 1);
      gtk_clist_thaw(GTK_CLIST(gradient_select_dialog->clist));
    }
}

void
grad_sel_copy_all(gint pos, gradient_t *grad)
{
  GSList *list = active_dialogs;
  GradSelectP gsp; 

  while(list)
    {
      gsp = (GradSelectP)list->data;
      gtk_clist_freeze(GTK_CLIST(gsp->clist));
      ed_insert_in_gradients_listbox(gsp->gc,gsp->clist,grad, pos, 1);
      gtk_clist_thaw(GTK_CLIST(gsp->clist));
      list = g_slist_next(list);
    }

  if(gradient_select_dialog)
    {
      gtk_clist_freeze(GTK_CLIST(gradient_select_dialog->clist));
      ed_insert_in_gradients_listbox(gradient_select_dialog->gc,gradient_select_dialog->clist,grad, pos, 1);
      gtk_clist_thaw(GTK_CLIST(gradient_select_dialog->clist));
    }
}

void
grad_sel_delete_all(gint n)
{
  GSList *list = active_dialogs;
  GradSelectP gsp; 

  while(list)
    {
      gsp = (GradSelectP)list->data;
      gtk_clist_remove(GTK_CLIST(gsp->clist), n);
      list = g_slist_next(list);
    }

  if(gradient_select_dialog)
    {
      gtk_clist_remove(GTK_CLIST(gradient_select_dialog->clist), n);
    }
}

void
grad_sel_free_all()
{
  GSList *list = active_dialogs;
  GradSelectP gsp; 

  while(list)
    {
      gsp = (GradSelectP)list->data;
      gtk_clist_freeze(GTK_CLIST(gsp->clist));
      gtk_clist_clear(GTK_CLIST(gsp->clist));
      gtk_clist_thaw(GTK_CLIST(gsp->clist));
      list = g_slist_next(list);
    }

  if(gradient_select_dialog)
    {
      gtk_clist_freeze(GTK_CLIST(gradient_select_dialog->clist));
      gtk_clist_clear(GTK_CLIST(gradient_select_dialog->clist));
      gtk_clist_thaw(GTK_CLIST(gradient_select_dialog->clist));
    }
}

void
grad_sel_refill_all()
{
  GSList *list = active_dialogs;
  GradSelectP gsp; 
  int select_pos = -1;

  while(list)
    {
      gsp = (GradSelectP)list->data;
      gsp->grad = curr_gradient;
      select_pos = ed_set_list_of_gradients(gsp->gc, 
					    gsp->clist, 
					    curr_gradient); 
      if(select_pos != -1) 
	gtk_clist_moveto(GTK_CLIST(gsp->clist),select_pos,0,0.0,0.0); 
      
      list = g_slist_next(list);
    }

  if(gradient_select_dialog)
    {
      gradient_select_dialog->grad = curr_gradient;
      select_pos = ed_set_list_of_gradients(gradient_select_dialog->gc, 
					    gradient_select_dialog->clist, 
					    curr_gradient); 
      if(select_pos != -1) 
	gtk_clist_moveto(GTK_CLIST(gradient_select_dialog->clist),select_pos,0,0.0,0.0); 
    }
}


void
sel_update_dialogs(gint row, gradient_t *grad)
{
  /* Go around each updating the names and hopefully the previews */
  GSList *list = active_dialogs;
  GradSelectP gsp; 

  while(list)
    {
      gsp = (GradSelectP)list->data;
      gtk_clist_set_text(GTK_CLIST(gsp->clist),row,1,grad->name);  
      /* Are we updating one that is selected in a popup dialog? */
      if(grad == gsp->grad)
	grad_change_callbacks(gsp, 0);
      list = g_slist_next(list);
    }

  if(gradient_select_dialog)
    gtk_clist_set_text(GTK_CLIST(gradient_select_dialog->clist),row,1,grad->name);  

  import_palette_grad_update(grad);
}

static void
sel_list_item_update(GtkWidget *widget, 
		     gint row,
		     gint column,
		     GdkEventButton *event,
		     gpointer data)
{
  GradSelectP gsp = (GradSelectP)data;

  GSList* tmp = g_slist_nth(gradients_list,row);
  gsp->grad = (gradient_t *)(tmp->data);

  /* If main one then make it the current selection */
  if(gsp == gradient_select_dialog)
    {
      grad_set_grad_to_name(gsp->grad->name);
      import_palette_grad_update(gsp->grad);
    }
  else
    {
      grad_change_callbacks(gsp, 0);
    }
}

static void
grad_select_edit_callback (GtkWidget *w,
			   gpointer   client_data)
{
  GradSelectP gsp;

  gsp = (GradSelectP) client_data;

  grad_create_gradient_editor_init(TRUE);

  /* Set the current gradient in this dailog to the "real  current"*/
  if(gsp && gsp->grad)
    grad_set_grad_to_name(gsp->grad->name);
}

void
grad_select_free (GradSelectP gsp)
{
  if (gsp)
    {
      if(gsp->callback_name)
	g_free(gsp->callback_name);

      /* remove from active list */

      active_dialogs = g_slist_remove(active_dialogs,gsp);

      g_free (gsp);
    }
}

/* Close active dialogs that no longer have PDB registered for them */

void
gradients_check_dialogs()
{
  GSList *list;
  GradSelectP gsp;
  gchar * name;
  ProcRecord *prec = NULL;

  list = active_dialogs;

  while (list)
    {
      gsp = (GradSelectP) list->data;
      list = list->next;

      name = gsp->callback_name;
      prec = procedural_db_lookup(name);
      
      if(!prec)
	{
	  active_dialogs = g_slist_remove(active_dialogs,gsp);

	  /* Can alter active_dialogs list*/
	  grad_select_close_callback(NULL,gsp); 
	}
    }
}


static void
grad_change_callbacks(GradSelectP gsp, gint closing)
{
  gchar * name;
  ProcRecord *prec = NULL;
  gradient_t *grad;
  int nreturn_vals;
  static int busy = 0;
  gradient_t *oldgrad = curr_gradient;

  /* Any procs registered to callback? */
  Argument *return_vals; 
  
  if(!gsp || !gsp->callback_name || busy != 0)
    return;

  busy = 1;
  name = gsp->callback_name;
  grad = gsp->grad;

  /* If its still registered run it */
  prec = procedural_db_lookup(name);

  if(prec && grad)
    {
      gdouble  *values, *pv;
      double    pos, delta;
      double    r, g, b, a;
      int i = gsp->sample_size;
      pos   = 0.0;
      delta = 1.0 / (i - 1);
      
      values = g_malloc(i * 4 * sizeof(gdouble));
      pv     = values;

      curr_gradient = grad;
      
      while (i--) {
	grad_get_color_at(pos, &r, &g, &b, &a);
	
	*pv++ = r;
	*pv++ = g;
	*pv++ = b;
	*pv++ = a;
	
	pos += delta;
      } /* while */

      curr_gradient = oldgrad;
      
      return_vals = procedural_db_run_proc (name,
					    &nreturn_vals,
					    PDB_STRING,grad->name,
					    PDB_INT32,gsp->sample_size*4,
					    PDB_FLOATARRAY,values,
					    PDB_INT32,closing,
					    PDB_END);
 
      if (!return_vals || return_vals[0].value.pdb_int != PDB_SUCCESS)
	g_message (_("failed to run gradient callback function"));
      else
	procedural_db_destroy_args (return_vals, nreturn_vals);
    }
  busy = 0;
}

static void
grad_select_close_callback (GtkWidget *w,
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

static gint
grad_select_delete_callback (GtkWidget *w,
			     GdkEvent *e,
			     gpointer client_data)
{
  grad_select_close_callback (w, client_data);
  return TRUE;
}

GradSelectP
gsel_new_selection(gchar * title,
		   gchar * initial_gradient)
{
  GradSelectP gsp;
  gradient_t *grad = NULL;
  GSList     *list;
  GtkWidget  *vbox;
  GtkWidget  *hbox;
  GtkWidget *scrolled_win;
  GdkColormap *colormap;
  int select_pos;

  /* Load them if they are not already loaded */
  if(g_editor == NULL)
    {
      grad_create_gradient_editor_init(FALSE);
    }

  gsp = g_malloc(sizeof(_GradSelect));
  gsp->callback_name = NULL;
  
  /*  The shell and main vbox  */
  gsp->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (gsp->shell), "gradselection", "Gimp");
  
  gtk_window_set_policy(GTK_WINDOW(gsp->shell), FALSE, TRUE, FALSE);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gsp->shell)->vbox), vbox, TRUE, TRUE, 0);

  /* handle the wm close event */
  gtk_signal_connect (GTK_OBJECT (gsp->shell), "delete_event",
		      GTK_SIGNAL_FUNC (grad_select_delete_callback),
		      gsp);

  /* clist preview of gradients */
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);

  gsp->clist = gtk_clist_new(2);
  gtk_clist_set_shadow_type(GTK_CLIST(gsp->clist), GTK_SHADOW_IN);
  
  gtk_clist_set_row_height(GTK_CLIST(gsp->clist), 18);
  
  gtk_clist_set_column_width(GTK_CLIST(gsp->clist), 0, 52);
  gtk_clist_set_column_title(GTK_CLIST(gsp->clist), 0, _("Gradient"));
  gtk_clist_set_column_title(GTK_CLIST(gsp->clist), 1, _("Name"));
  gtk_clist_column_titles_show(GTK_CLIST(gsp->clist));
  
  hbox = gtk_hbox_new(FALSE, 8);
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);

  gtk_box_pack_start(GTK_BOX(hbox), scrolled_win, TRUE, TRUE, 0); 
  gtk_container_add (GTK_CONTAINER (scrolled_win), gsp->clist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_ALWAYS);

  gtk_widget_show(scrolled_win);
  gtk_widget_show(gsp->clist);
  gtk_widget_set_usize (gsp->clist, 200, 250);

  colormap = gtk_widget_get_colormap(gsp->clist);
  gdk_color_parse("black", &gsp->black);
  gdk_color_alloc(colormap, &gsp->black);

  gtk_signal_connect(GTK_OBJECT(gsp->clist), "select_row",
		     GTK_SIGNAL_FUNC(sel_list_item_update),
		     (gpointer) gsp);

  action_items[0].user_data = gsp;
  action_items[1].user_data = gsp;
  build_action_area (GTK_DIALOG (gsp->shell), action_items, 2, 0);

  if(!title)
    {
      gtk_window_set_title (GTK_WINDOW (gsp->shell), _("Gradient Selection"));
    }
  else
    {
      gtk_window_set_title (GTK_WINDOW (gsp->shell), title);
    }
  if(initial_gradient && strlen(initial_gradient))
    {
      list = gradients_list;
      while (list) 
	{
	  grad = list->data;
	  if (strcmp(grad->name, initial_gradient) == 0) 
	    {
	      /* We found it! */
	      break;
	    }
	  list = g_slist_next(list);
	} 
    }
  
  if(grad == NULL)
    grad = curr_gradient;

  gsp->grad = grad;

  gtk_widget_realize(gsp->shell);
  gsp->gc = gdk_gc_new(gsp->shell->window);
  
  select_pos = ed_set_list_of_gradients(gsp->gc, 
 					gsp->clist, 
 					grad); 

  /* Now show the dialog */
  gtk_widget_show(vbox);
  gtk_widget_show(gsp->shell);

  if(select_pos != -1) 
    gtk_clist_moveto(GTK_CLIST(gsp->clist),select_pos,0,0.0,0.0); 

  return gsp;
}

void
grad_create_gradient_editor(void)
{
  if(gradient_select_dialog == NULL)
    {
      gradient_select_dialog = gsel_new_selection(_("Gradients"),NULL);
  
      session_set_window_geometry (gradient_select_dialog->shell, &gradient_select_session_info, TRUE);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE(gradient_select_dialog->shell))
	{
	  gtk_widget_show(gradient_select_dialog->shell);
	}
      else
	gdk_window_raise(gradient_select_dialog->shell->window);
      return;
    }
}

/************
 * PDB interfaces.
 */


static Argument *
gradients_popup_invoker (Argument *args)
{
  gchar * name; 
  gchar * title;
  gchar * initial_gradient;
  ProcRecord *prec = NULL;
  gint sample_sz;
  GradSelectP newdialog;

  success = (name = (char *) args[0].value.pdb_pointer) != NULL;
  title = (char *) args[1].value.pdb_pointer;
  initial_gradient = (char *) args[2].value.pdb_pointer;
  sample_sz = args[3].value.pdb_int;

  if(sample_sz < 0 || sample_sz > 10000)
    sample_sz = G_SAMPLE;

  /* Check the proc exists */
  if(!success || (prec = procedural_db_lookup(name)) == NULL)
    {
      success = 0;
      return procedural_db_return_args (&gradients_popup_proc, success);
    }

  if(initial_gradient && strlen(initial_gradient))
    newdialog = gsel_new_selection(title,
				 initial_gradient);
  else
    newdialog = gsel_new_selection(title,NULL);

  /* Add to list of proc to run when pattern changes */
  newdialog->callback_name = g_strdup(name);
  newdialog->sample_size = sample_sz;

  /* Add to active gradient dialogs list */
  active_dialogs = g_slist_append(active_dialogs,newdialog);

  return procedural_db_return_args (&gradients_popup_proc, success);
}

/*  The procedure definition  */
ProcArg gradients_popup_in_args[] =
{
  { PDB_STRING,
    "gradients_callback",
    "the callback PDB proc to call when gradient selection is made"
  },
  { PDB_STRING,
    "popup title",
    "title to give the gradient popup window",
  },
  { PDB_STRING,
    "initial gradient",
    "The name of the pattern to set as the first selected",
  },
  { PDB_INT32,
    "sample size",
    "size of the sample to return when the gradient is changed (< 10000)",
  },
};

ProcRecord gradients_popup_proc =
{
  "gimp_gradients_popup",
  "Invokes the Gimp gradients selection",
  "This procedure popups the gradients selection dialog",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(gradients_popup_in_args) / sizeof(gradients_popup_in_args[0]),
  gradients_popup_in_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gradients_popup_invoker } },
};

static GradSelectP
gradients_get_gradientselect(gchar *name)
{
  GSList *list;
  GradSelectP gsp;

  list = active_dialogs;

  while (list)
    {
      gsp = (GradSelectP) list->data;
      list = list->next;
      
      if(strcmp(name,gsp->callback_name) == 0)
	{
	  return gsp;
	}
    }

  return NULL;
}

static Argument *
gradients_close_popup_invoker (Argument *args)
{
  gchar * name; 
  ProcRecord *prec = NULL;
  GradSelectP gsp;

  success = (name = (char *) args[0].value.pdb_pointer) != NULL;

  /* Check the proc exists */
  if(!success || (prec = procedural_db_lookup(name)) == NULL)
    {
      success = 0;
      return procedural_db_return_args (&gradients_close_popup_proc, success);
    }

  gsp = gradients_get_gradientselect(name);

  if(gsp)
    {
      active_dialogs = g_slist_remove(active_dialogs,gsp);
      
      if (GTK_WIDGET_VISIBLE (gsp->shell))
	gtk_widget_hide (gsp->shell);
      
      /* Free memory if poping down dialog which is not the main one */
      if(gsp != gradient_select_dialog)
	{
	  /* Send data back */
	  gtk_widget_destroy(gsp->shell); 
	  grad_select_free(gsp); 
	}
    }
  else
    {
      success = FALSE;
    }

  return procedural_db_return_args (&gradients_close_popup_proc, success);
}

/*  The procedure definition  */
ProcArg gradients_close_popup_in_args[] =
{
  { PDB_STRING,
    "callback PDB entry name",
    "The name of the callback registered for this popup",
  },
};

ProcRecord gradients_close_popup_proc =
{
  "gimp_gradients_close_popup",
  "Popdown the Gimp gradient selection",
  "This procedure closes an opened gradient selection dialog",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(gradients_close_popup_in_args) / sizeof(gradients_close_popup_in_args[0]),
  gradients_close_popup_in_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gradients_close_popup_invoker } },
};

static Argument *
gradients_set_popup_invoker (Argument *args)
{
  gchar * pdbname; 
  gchar * gradient_name;
  ProcRecord *prec = NULL;
  GradSelectP gsp;

  success = (pdbname = (char *) args[0].value.pdb_pointer) != NULL;
  gradient_name = (char *) args[1].value.pdb_pointer;

  /* Check the proc exists */
  if(!success || (prec = procedural_db_lookup(pdbname)) == NULL)
    {
      success = 0;
      return procedural_db_return_args (&gradients_set_popup_proc, success);
    }

  gsp = gradients_get_gradientselect(pdbname);

  if(gsp)
    {
      /* Can alter active_dialogs list*/
	GSList     *tmp;
	gradient_t *active = NULL;
	int pos = 0;

	tmp = gradients_list;

	while (tmp) {
		active = tmp->data;

		if (strcmp(gradient_name, active->name) == 0)
			break; /* We found the one we want */

		pos++;
		tmp = g_slist_next(tmp);
	} /* while */

      if(active)
	{
	  gtk_clist_select_row(GTK_CLIST(gsp->clist),pos,-1);
	  gtk_clist_moveto(GTK_CLIST(gsp->clist),pos,0,0.0,0.0);
	  success = TRUE;
	}
    }
  else
    {
      success = FALSE;
    }

  return procedural_db_return_args (&gradients_close_popup_proc, success);
}

/*  The procedure definition  */
ProcArg gradients_set_popup_in_args[] =
{
  { PDB_STRING,
    "callback PDB entry name",
    "The name of the callback registered for this popup",
  },
  { PDB_STRING,
    "gradient name to set",
    "The name of the gradient to set as selected",
  },
};

ProcRecord gradients_set_popup_proc =
{
  "gimp_gradients_set_popup",
  "Sets the current gradient selection in a popup",
  "Sets the current gradient selection in a popup",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(gradients_set_popup_in_args) / sizeof(gradients_set_popup_in_args[0]),
  gradients_set_popup_in_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gradients_set_popup_invoker } },
};

/*********************************/
/*  GRADIENTS_GET_GRADIENT_DATA  */

static Argument *
gradients_get_gradient_data_invoker (Argument *args)
{
  gradient_t * grad = NULL;
  GSList *list;
  char *name;
  gint sample_sz;

  success = (name = (char *) args[0].value.pdb_pointer) != NULL;
  sample_sz = args[1].value.pdb_int;

  if(sample_sz < 0 || sample_sz > 10000)
    sample_sz = G_SAMPLE;

  if (!success)
    {
      /* No name use active pattern */
      success = (grad = curr_gradient) != NULL;
    }
  else
    {
      success = FALSE;

      list = gradients_list;
      
      while (list) {
	grad = list->data;
	
	if (strcmp(grad->name, name) == 0) 
	  {
	      success = TRUE;
	      break;	  /* We found it! */
	  }
	  
	  /* Select that gradient in the listbox */
	  /* Only if gradient editor has been created */
	  
	list = g_slist_next(list);
      } /* while */
    }

  return_args = procedural_db_return_args (&gradients_get_gradient_data_proc, success);

  if (success)
    {
      gdouble  *values, *pv;
      double    pos, delta;
      double    r, g, b, a;
      int i = sample_sz;
      gradient_t *oldgrad = curr_gradient;
      pos   = 0.0;
      delta = 1.0 / (i - 1);
      
      values = g_malloc(i * 4 * sizeof(gdouble));
      pv     = values;

      curr_gradient = grad;
     
      while (i--) {
	grad_get_color_at(pos, &r, &g, &b, &a);
	
	*pv++ = r;
	*pv++ = g;
	*pv++ = b;
	*pv++ = a;
	
	pos += delta;
      } /* while */

      curr_gradient = oldgrad;

      return_args[1].value.pdb_pointer = g_strdup (grad->name);
      return_args[2].value.pdb_int = sample_sz*4;
      return_args[3].value.pdb_pointer = values;
    }

  return return_args;
}

/*  The procedure definition  */

ProcArg gradients_get_gradient_data_in_args[] =
{
  { PDB_STRING,
    "name",
    "the gradient name (\"\" means current active gradient) "
  },
 { PDB_INT32,
    "sample size",
    "size of the sample to return when the gradient is changed (< 10000)"
 },
};

ProcArg gradients_get_gradient_data_out_args[] =
{
  { PDB_STRING,
    "name",
    "the gradient name"
  },
  { PDB_INT32,
    "width",
    "the gradient sample width (r,g,b,a) "
  },
  { PDB_FLOATARRAY,
    "grad data",
    "the gradient sample data"},
};

ProcRecord gradients_get_gradient_data_proc =
{
  "gimp_gradients_get_gradient_data",
  "Retrieve information about the specified gradient (including data)",
  "This procedure retrieves information about the gradient.  This includes the gradient name, and the sample data for the gradient.",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(gradients_get_gradient_data_in_args) / sizeof(gradients_get_gradient_data_in_args[0]),
  gradients_get_gradient_data_in_args,

  /*  Output arguments  */
  sizeof(gradients_get_gradient_data_out_args) / sizeof(gradients_get_gradient_data_out_args[0]),
  gradients_get_gradient_data_out_args,

  /*  Exec method  */
  { { gradients_get_gradient_data_invoker } },
};
