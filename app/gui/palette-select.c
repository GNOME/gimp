/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1998 Andy Thomas (alt@picnic.demon.co.uk)
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "color_area.h"
#include "color_select.h"
#include "datafiles.h"
#include "devices.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"
#include "gradient_header.h"
#include "gradient.h"
#include "interface.h"
#include "palette.h"
#include "palette_entries.h"
#include "session.h"
#include "palette_select.h"

#include "libgimp/gimpintl.h"

static GSList *active_dialogs = NULL; /* List of active dialogs */
static void   palette_select_close_callback (GtkWidget *w,gpointer   client_data);
static void   palette_select_edit_callback (GtkWidget *w,gpointer   client_data);

static ActionAreaItem action_items[2] =
{
  { N_("Edit"), palette_select_edit_callback, NULL, NULL },
  { N_("Close"), palette_select_close_callback, NULL, NULL }
};

void
palette_select_set_text_all(PaletteEntriesP entries)
{
  gint pos = 0;
  char *num_buf;
  GSList *aclist = active_dialogs;
  GSList *plist;
  PaletteSelectP psp; 
  PaletteEntriesP p_entries = NULL;

  plist = palette_entries_list;
  
  while (plist)
    {
      p_entries = (PaletteEntriesP) plist->data;
      plist = g_slist_next (plist);
      
      if (p_entries == entries)
	    break;
      pos++;
    }

  if(p_entries == NULL)
    return; /* This is actually and error */

  num_buf = g_strdup_printf("%d",p_entries->n_colors);;

  while(aclist)
    {
      char *num_copy = g_strdup(num_buf);

      psp = (PaletteSelectP)aclist->data;
      gtk_clist_set_text(GTK_CLIST(psp->clist),pos,1,num_copy);
      aclist = g_slist_next(aclist);
    }
}

void
palette_select_refresh_all()
{
  GSList *list = active_dialogs;
  PaletteSelectP psp; 

  while(list)
    {
      psp = (PaletteSelectP)list->data;
      gtk_clist_freeze(GTK_CLIST(psp->clist));
      gtk_clist_clear(GTK_CLIST(psp->clist));
      palette_clist_init(psp->clist,psp->shell,psp->gc);
      gtk_clist_thaw(GTK_CLIST(psp->clist));
      list = g_slist_next(list);
    }
}

void
palette_select_clist_insert_all(PaletteEntriesP p_entries)
{
  GSList *aclist = active_dialogs;
  PaletteEntriesP chk_entries;
  PaletteSelectP psp; 
  GSList *plist;
  gint pos = 0;

  plist = palette_entries_list;
  
  while (plist)
    {
      chk_entries = (PaletteEntriesP) plist->data;
      plist = g_slist_next (plist);
      
      /*  to make sure we get something!  */
      if (chk_entries == NULL)
	{
	  return;
	}
      if (strcmp(p_entries->name, chk_entries->name) == 0)
	break;
      pos++;
    }

  while(aclist)
    {
      psp = (PaletteSelectP)aclist->data;
      gtk_clist_freeze(GTK_CLIST(psp->clist));
      palette_insert_clist(psp->clist,psp->shell,psp->gc,p_entries,pos);
      gtk_clist_thaw(GTK_CLIST(psp->clist));
      aclist = g_slist_next(aclist);
    }

/*   if(gradient_select_dialog) */
/*     { */
/*       gtk_clist_set_text(GTK_CLIST(gradient_select_dialog->clist),n,1,grad->name);   */
/*     } */
}

void
palette_select_free (PaletteSelectP psp)
{
  if (psp)
    {
/*       if(psp->callback_name) */
/* 	g_free(gsp->callback_name); */

      /* remove from active list */

      active_dialogs = g_slist_remove(active_dialogs,psp); 

      g_free (psp);
    }
}

static void
palette_select_edit_callback (GtkWidget *w,
			       gpointer   client_data)
{
  PaletteEntriesP p_entries = NULL;
  PaletteSelectP psp = (PaletteSelectP)client_data;
  GList *sel_list;

  sel_list = GTK_CLIST(psp->clist)->selection;

  if(sel_list)
    {
      while (sel_list)
	{
	  gint row;

	  row = GPOINTER_TO_INT (sel_list->data);

	  p_entries = 
	    (PaletteEntriesP)gtk_clist_get_row_data(GTK_CLIST(psp->clist),row);

	  palette_create_edit(p_entries);

	  /* One only */
	  return;
	}
    }
}

static void
palette_select_close_callback (GtkWidget *w,
			       gpointer   client_data)
{
  PaletteSelectP psp;

  psp = (PaletteSelectP) client_data;

  if (GTK_WIDGET_VISIBLE (psp->shell))
    gtk_widget_hide (psp->shell);

  gtk_widget_destroy(psp->shell); 
  palette_select_free(psp); 

  /* Free memory if poping down dialog which is not the main one */
/*   if(gsp != gradient_select_dialog) */
/*     { */
/*       grad_change_callbacks(gsp,1); */
/*       gtk_widget_destroy(gsp->shell);  */
/*       grad_select_free(gsp);  */
/*     } */
}

static gint
palette_select_delete_callback (GtkWidget *w,
				GdkEvent *e,
				gpointer client_data)
{
  palette_select_close_callback (w, client_data);
  return TRUE;
}

PaletteSelectP
palette_new_selection(gchar * title,
		      gchar * initial_palette)
{
  PaletteSelectP psp;
/*   gradient_t *grad = NULL; */
  GSList     *list;
  GtkWidget  *vbox;
  GtkWidget  *hbox;
  GtkWidget *scrolled_win;
  PaletteEntriesP p_entries = NULL;
  int select_pos;

  palette_select_palette_init();

  psp = g_malloc(sizeof(struct _PaletteSelect));
  psp->callback_name = NULL;
  
  /*  The shell and main vbox  */
  psp->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (psp->shell), "paletteselection", "Gimp");
  
  gtk_window_set_policy(GTK_WINDOW(psp->shell), FALSE, TRUE, FALSE);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (psp->shell)->vbox), vbox, TRUE, TRUE, 0);

  /* handle the wm close event */
  gtk_signal_connect (GTK_OBJECT (psp->shell), "delete_event",
		      GTK_SIGNAL_FUNC (palette_select_delete_callback),
		      psp);

  /* clist preview of gradients */
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);

  psp->clist = gtk_clist_new(3);
  gtk_clist_set_shadow_type(GTK_CLIST(psp->clist), GTK_SHADOW_IN);
  
  gtk_clist_set_row_height(GTK_CLIST(psp->clist), SM_PREVIEW_HEIGHT+2);
  
  gtk_widget_set_usize (psp->clist, 203, 200);
  gtk_clist_set_column_title(GTK_CLIST(psp->clist), 0, _("Palette"));
  gtk_clist_set_column_title(GTK_CLIST(psp->clist), 1, _("Ncols"));
  gtk_clist_set_column_title(GTK_CLIST(psp->clist), 2, _("Name"));
  gtk_clist_column_titles_show(GTK_CLIST(psp->clist));
  gtk_clist_set_column_width(GTK_CLIST(psp->clist), 0, SM_PREVIEW_WIDTH+2);


  hbox = gtk_hbox_new(FALSE, 8);
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);

  gtk_box_pack_start(GTK_BOX(hbox), scrolled_win, TRUE, TRUE, 0); 
  gtk_container_add (GTK_CONTAINER (scrolled_win), psp->clist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_ALWAYS);

  gtk_widget_show(scrolled_win);
  gtk_widget_show(psp->clist);

/*   gtk_signal_connect(GTK_OBJECT(gsp->clist), "select_row", */
/* 		     GTK_SIGNAL_FUNC(sel_list_item_update), */
/* 		     (gpointer) gsp); */

  action_items[0].user_data = psp;
  action_items[1].user_data = psp;
  build_action_area (GTK_DIALOG (psp->shell), action_items, 2, 0);

  if(!title)
    {
      gtk_window_set_title (GTK_WINDOW (psp->shell), _("Palette Selection"));
    }
  else
    {
      gtk_window_set_title (GTK_WINDOW (psp->shell), title);
    }

  select_pos = -1;
  if(initial_palette && strlen(initial_palette))
    {
      list = palette_entries_list;
      
      while (list)
	{
	  p_entries = (PaletteEntriesP) list->data;
	  list = g_slist_next (list);
	  
	  if (strcmp(p_entries->name, initial_palette) > 0)
	    break;
	  select_pos++;
	}
    }

  gtk_widget_realize(psp->shell);
  psp->gc = gdk_gc_new(psp->shell->window);  
  
  palette_clist_init(psp->clist,psp->shell,psp->gc);

  /* Now show the dialog */
  gtk_widget_show(vbox);
  gtk_widget_show(psp->shell);

  if(select_pos != -1) 
    {
      gtk_clist_select_row(GTK_CLIST(psp->clist),select_pos,-1);
      gtk_clist_moveto(GTK_CLIST(psp->clist),select_pos,0,0.0,0.0); 
    }
  else
    gtk_clist_select_row(GTK_CLIST(psp->clist),0,-1);

  active_dialogs = g_slist_append(active_dialogs,psp);

  return psp;
}
