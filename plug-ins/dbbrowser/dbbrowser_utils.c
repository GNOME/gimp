/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* 
 * dbbrowser_utils.c
 * 0.08  26th sept 97  by Thomas NOEL <thomas@minet.net>
 *
 * 98/12/13  Sven Neumann <sven@gimp.org> : added help display
 */

#include "config.h"

#include <string.h>
#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "dbbrowser_utils.h"

#include "libgimp/stdplugins-intl.h"

/* FIXME: remove usage of the 'broken' GtkText */
#define GTK_ENABLE_BROKEN
#include <gtk/gtktext.h>

/* configuration */

#define DBL_LIST_WIDTH 220
#define DBL_WIDTH DBL_LIST_WIDTH+400
#define DBL_HEIGHT 250

/* end of configuration */

typedef struct
{
  gchar *label;
  gchar *func;
} ListEntry_t;

typedef struct
{
  GtkWidget *dlg;

  GtkWidget *search_entry;
  GtkWidget *name_button;
  GtkWidget *blurb_button;

  GtkWidget *descr_vbox;
  GtkWidget *descr_table;

  GtkWidget *clist;
  GtkWidget *scrolled_win;

  /* the currently selected procedure */
  gchar           *selected_proc_name;
  gchar           *selected_scheme_proc_name;
  gchar           *selected_proc_blurb;
  gchar           *selected_proc_help;
  gchar           *selected_proc_author;
  gchar           *selected_proc_copyright;
  gchar           *selected_proc_date;
  GimpPDBProcType  selected_proc_type;
  gint             selected_nparams;
  gint             selected_nreturn_vals;
  GimpParamDef    *selected_params;
  GimpParamDef    *selected_return_vals; 

  void (* apply_callback) (gchar           *proc_name,
			   gchar           *scheme_proc_name,
			   gchar           *proc_blurb,
			   gchar           *proc_help,
			   gchar           *proc_author,
			   gchar           *proc_copyright,
			   gchar           *proc_date,
			   GimpPDBProcType  proc_type,
			   gint             nparams,
			   gint             nreturn_vals,
			   GimpParamDef    *params,
			   GimpParamDef    *return_vals);

} dbbrowser_t;

/* local functions */

static void    dialog_apply_callback     (GtkWidget      *widget,
					  gpointer        data);
static gint    procedure_select_callback (GtkWidget      *widget,
					  gint            row,
					  gint            column,
					  GdkEventButton *bevent,
					  gpointer        data);
static void    dialog_search_callback    (GtkWidget      *widget, 
					  gpointer        data);
static void    dialog_select             (dbbrowser_t    *dbbrowser, 
					  gchar          *proc_name);
static void    dialog_close_callback     (GtkWidget      *widget, 
					  gpointer        data);
static void    convert_string            (gchar          *str);
static gchar * GParamType2char           (GimpPDBArgType  t);

GList * proc_table = NULL;

/* create the dialog box
 * console_entry != NULL => called from the script-fu-console
 */
GtkWidget *
gimp_db_browser (GimpDBBrowserApplyCallback apply_callback)
{
  dbbrowser_t* dbbrowser;
  
  GtkWidget *hbox;
  GtkWidget *searchhbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *scrolled_window;

  dbbrowser = g_new (dbbrowser_t, 1);
  
  dbbrowser->apply_callback = apply_callback;
  
  /* the dialog box */

  if (apply_callback)
    {
      dbbrowser->dlg =
	gimp_dialog_new (_("DB Browser (init...)"), "dbbrowser",
			 gimp_standard_help_func, "filters/dbbrowser.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("Search by Name"), dialog_search_callback,
			 dbbrowser, NULL, &dbbrowser->name_button, TRUE, FALSE,
			 _("Search by Blurb"), dialog_search_callback,
			 dbbrowser, NULL, &dbbrowser->blurb_button, FALSE, FALSE,
			 GTK_STOCK_APPLY, dialog_apply_callback,
			 dbbrowser, NULL, NULL, FALSE, FALSE,
			 GTK_STOCK_CLOSE, dialog_close_callback,
			 dbbrowser, NULL, NULL, FALSE, TRUE,

			 NULL);
    }
  else
    {
      dbbrowser->dlg =
	gimp_dialog_new (_("DB Browser (init...)"), "dbbrowser",
			 gimp_standard_help_func, "filters/dbbrowser.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("Search by Name"), dialog_search_callback,
			 dbbrowser, NULL, &dbbrowser->name_button, TRUE, FALSE,
			 _("Search by Blurb"), dialog_search_callback,
			 dbbrowser, NULL, &dbbrowser->blurb_button, FALSE, FALSE,
			 GTK_STOCK_CLOSE, dialog_close_callback,
			 dbbrowser, NULL, NULL, FALSE, TRUE,

			 NULL);
    }

  gtk_signal_connect (GTK_OBJECT (dbbrowser->dlg), "destroy",
                      GTK_SIGNAL_FUNC (dialog_close_callback),
                      dbbrowser);
  
  /* hbox : left=list ; right=description */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dlg)->vbox), 
		      hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /* left = vbox : the list and the search entry */
  
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /* list : list in a scrolled_win */
  
  dbbrowser->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (dbbrowser->scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), dbbrowser->scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (dbbrowser->scrolled_win);

  dbbrowser->clist = gtk_clist_new (1);
  gtk_widget_set_usize (dbbrowser->clist, DBL_LIST_WIDTH, DBL_HEIGHT);
  gtk_clist_set_selection_mode (GTK_CLIST (dbbrowser->clist),
			        GTK_SELECTION_BROWSE);
  gtk_container_add (GTK_CONTAINER (dbbrowser->scrolled_win), dbbrowser->clist);
  gtk_widget_show (dbbrowser->clist);

  g_signal_connect (G_OBJECT (dbbrowser->clist), "select_row",
		    G_CALLBACK (procedure_select_callback),
		    dbbrowser);

  /* search entry */

  searchhbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), searchhbox, FALSE, FALSE, 0);
  gtk_widget_show (searchhbox);

  label = gtk_label_new (_("Search:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (searchhbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  dbbrowser->search_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (searchhbox), dbbrowser->search_entry,
		      TRUE, TRUE, 0);
  gtk_widget_show (dbbrowser->search_entry);

  /* right = description */

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrolled_window, DBL_WIDTH - DBL_LIST_WIDTH, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  dbbrowser->descr_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (dbbrowser->descr_vbox), 4);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
					 dbbrowser->descr_vbox);
  gtk_widget_show (dbbrowser->descr_vbox);

  /* now build the list */

  gtk_widget_show (dbbrowser->dlg);

  /* initialize the "return" value (for "apply") */

  dbbrowser->descr_table               = NULL;
  dbbrowser->selected_proc_name        = NULL;
  dbbrowser->selected_scheme_proc_name = NULL;
  dbbrowser->selected_proc_blurb       = NULL;
  dbbrowser->selected_proc_help        = NULL;
  dbbrowser->selected_proc_author      = NULL;
  dbbrowser->selected_proc_copyright   = NULL;
  dbbrowser->selected_proc_date        = NULL;
  dbbrowser->selected_proc_type        = 0;
  dbbrowser->selected_nparams          = 0;
  dbbrowser->selected_nreturn_vals     = 0;
  dbbrowser->selected_params           = NULL;
  dbbrowser->selected_return_vals      = NULL;

  /* first search (all procedures) */
  dialog_search_callback (NULL, (gpointer) dbbrowser);

  return dbbrowser->dlg;
}

static gint
procedure_select_callback (GtkWidget      *widget,
			   gint            row,
			   gint            column,
			   GdkEventButton *bevent,
			   gpointer        data)
{
  dbbrowser_t *dbbrowser = data;
  gchar       *func;

  g_return_val_if_fail (widget != NULL, FALSE);
  /*  g_return_val_if_fail (bevent != NULL, FALSE);  */
  g_return_val_if_fail (dbbrowser != NULL, FALSE);

  if ((func = (gchar *) (gtk_clist_get_row_data (GTK_CLIST (widget), row))))
    dialog_select (dbbrowser, func);
  return FALSE;
}

/* update the description box (right) */
static void 
dialog_select (dbbrowser_t *dbbrowser, 
	       gchar       *proc_name)
{
  GtkWidget     *old_table;
  GtkWidget     *label;
  GtkWidget     *entry;
  GtkWidget     *sep;
  GtkWidget     *scrolled_window;
  GtkWidget     *text_view;
  GtkTextBuffer *text_buffer = NULL;
  gint           i;
  gint           row = 0;

  g_free (dbbrowser->selected_proc_name);
  dbbrowser->selected_proc_name = g_strdup (proc_name);

  g_free (dbbrowser->selected_scheme_proc_name);
  dbbrowser->selected_scheme_proc_name = g_strdup (proc_name);
  convert_string (dbbrowser->selected_scheme_proc_name);

  g_free (dbbrowser->selected_proc_blurb);
  g_free (dbbrowser->selected_proc_help);
  g_free (dbbrowser->selected_proc_author);
  g_free (dbbrowser->selected_proc_copyright);
  g_free (dbbrowser->selected_proc_date);
  g_free (dbbrowser->selected_params);
  g_free (dbbrowser->selected_return_vals);

  gimp_procedural_db_proc_info (proc_name, 
				&(dbbrowser->selected_proc_blurb), 
				&(dbbrowser->selected_proc_help), 
				&(dbbrowser->selected_proc_author),
				&(dbbrowser->selected_proc_copyright), 
				&(dbbrowser->selected_proc_date), 
				&(dbbrowser->selected_proc_type), 
				&(dbbrowser->selected_nparams),
				&(dbbrowser->selected_nreturn_vals), 
				&(dbbrowser->selected_params), 
				&(dbbrowser->selected_return_vals));

  /* save the "old" table */
  old_table = dbbrowser->descr_table;

  dbbrowser->descr_table = gtk_table_new (10 +
					  dbbrowser->selected_nparams +
					  dbbrowser->selected_nreturn_vals,
					  5, FALSE);

  gtk_table_set_col_spacings (GTK_TABLE (dbbrowser->descr_table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (dbbrowser->descr_table), 0, 2);

  /* show the name */

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), dbbrowser->selected_scheme_proc_name);
  gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);

  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->descr_table), 0, row++,
			     _("Name:"), 1.0, 0.5,
			     entry, 3, FALSE);

  /* show the description */

  label = gtk_label_new (dbbrowser->selected_proc_blurb);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->descr_table), 0, row++,
			     _("Blurb:"), 1.0, 0.5,
			     label, 3, FALSE);

  sep = gtk_hseparator_new ();
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), sep,
		    0, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 6);
  gtk_widget_show (sep);
  row++;

  /* in parameters */
  if (dbbrowser->selected_nparams) 
    {
      label = gtk_label_new (_("In:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			0, 1, row, row + (dbbrowser->selected_nparams), 
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      for (i = 0; i < dbbrowser->selected_nparams; i++) 
	{
	  /* name */
	  label = gtk_label_new ((dbbrowser->selected_params[i]).name);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			    1, 2, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  /* type */
	  label = gtk_label_new (GParamType2char ((dbbrowser->selected_params[i]).type));
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			    2, 3, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  /* description */
	  label = gtk_label_new ((dbbrowser->selected_params[i]).description);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			    3, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  row++;
	}
    }

  if ((dbbrowser->selected_nparams) && 
      (dbbrowser->selected_nreturn_vals))
    {
      sep = gtk_hseparator_new ();
      gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), sep,
			0, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 6);
      gtk_widget_show (sep);
      row++;
    }

  /* out parameters */
  if (dbbrowser->selected_nreturn_vals)
    {
      label = gtk_label_new (_("Out:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			0, 1, row, row + (dbbrowser->selected_nreturn_vals), 
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      for (i = 0; i < dbbrowser->selected_nreturn_vals; i++) 
	{
	  /* name */
	  label = gtk_label_new ((dbbrowser->selected_return_vals[i]).name);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			    1, 2, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  /* type */
	  label = gtk_label_new (GParamType2char (dbbrowser->selected_return_vals[i].type));
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			    2, 3, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  /* description */
	  label = gtk_label_new (dbbrowser->selected_return_vals[i].description);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			    3, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  row++;
	}
    }

  if (dbbrowser->selected_nparams || 
      dbbrowser->selected_nreturn_vals)
    {
      sep = gtk_hseparator_new ();
      gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), sep,
			0, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 6);
      gtk_widget_show (sep);
      row++;
    }

  /* show the help */
  if (dbbrowser->selected_proc_help &&
      (strlen (dbbrowser->selected_proc_help) > 1))
    {
      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);

      gimp_table_attach_aligned (GTK_TABLE (dbbrowser->descr_table), 0, row++,
				 _("Help:"), 1.0, 0.5,
				 scrolled_window, 3, FALSE);

      text_buffer = gtk_text_buffer_new (NULL);
      text_view = gtk_text_view_new_with_buffer (text_buffer);
      g_object_unref (G_OBJECT (text_buffer));

      gtk_text_buffer_set_text (text_buffer, dbbrowser->selected_proc_help, -1);

      gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
      gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
      gtk_widget_set_usize (text_view, -1, 60);
      gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
      gtk_widget_show (text_view);

      sep = gtk_hseparator_new ();
      gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), sep,
			0, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 6);
      gtk_widget_show (sep);

      row++;
    }

  /* show the author & the copyright */

  label = gtk_label_new (dbbrowser->selected_proc_author);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gtk_table_set_row_spacing (GTK_TABLE (dbbrowser->descr_table), row, 2);
  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->descr_table), 0, row++,
			     _("Author:"), 1.0, 0.5,
			     label, 3, FALSE);

  label = gtk_label_new (dbbrowser->selected_proc_date);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gtk_table_set_row_spacing (GTK_TABLE (dbbrowser->descr_table), row, 2);
  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->descr_table), 0, row++,
			     _("Date:"), 1.0, 0.5,
			     label, 3, FALSE);

  label = gtk_label_new (dbbrowser->selected_proc_copyright);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gtk_table_set_row_spacing (GTK_TABLE (dbbrowser->descr_table), row, 2);
  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->descr_table), 0, row++,
			     _("Copyright:"), 1.0, 0.5,
			     label, 3, FALSE);

  if (old_table)
    gtk_container_remove (GTK_CONTAINER (dbbrowser->descr_vbox), old_table);

  gtk_box_pack_start (GTK_BOX (dbbrowser->descr_vbox), dbbrowser->descr_table,
		      FALSE, FALSE, 0);
  gtk_widget_show (dbbrowser->descr_table);
}

/* end of the dialog */
static void
dialog_close_callback (GtkWidget *widget, 
		       gpointer   data)
{
  dbbrowser_t* dbbrowser = data;

  if (dbbrowser->apply_callback)
    {
      /* we are called by another application : just kill the dialog box */
      gtk_widget_hide (dbbrowser->dlg);
      gtk_widget_destroy (dbbrowser->dlg);
    }
  else
    {
      /* we are in the plug_in : kill the gtk application */
      gtk_widget_destroy (dbbrowser->dlg);
      gtk_main_quit ();
    }
}

/* end of the dialog */
static void 
dialog_apply_callback (GtkWidget *widget, 
		       gpointer   data)
{
  dbbrowser_t* dbbrowser = data;

  (dbbrowser->apply_callback) (dbbrowser->selected_proc_name,
			       dbbrowser->selected_scheme_proc_name,
			       dbbrowser->selected_proc_blurb,
			       dbbrowser->selected_proc_help,
			       dbbrowser->selected_proc_author,
			       dbbrowser->selected_proc_copyright,
			       dbbrowser->selected_proc_date,
			       dbbrowser->selected_proc_type,
			       dbbrowser->selected_nparams,
			       dbbrowser->selected_nreturn_vals,
			       dbbrowser->selected_params,
			       dbbrowser->selected_return_vals);
}

/* search in the whole db */
static void 
dialog_search_callback (GtkWidget *widget, 
			gpointer   data)
{
  gchar       **proc_list;
  gint          num_procs;
  gint          i, j;
  dbbrowser_t  *dbbrowser = data;
  gchar        *func_name;
  gchar        *label;
  const gchar  *query_text;
  GString      *query;

  gtk_clist_freeze (GTK_CLIST (dbbrowser->clist));
  gtk_clist_clear (GTK_CLIST (dbbrowser->clist));

  /* search */

  if (widget == (dbbrowser->name_button))
    {
      gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), 
			    _("DB Browser (by name - please wait)"));

      query = g_string_new ("");
      query_text = gtk_entry_get_text (GTK_ENTRY (dbbrowser->search_entry));

      while (*query_text)
	{
	  if ((*query_text == '_') || (*query_text == '-'))
	    g_string_append (query, "[-_]");
	  else
	    g_string_append_c (query, *query_text);

	  query_text++;
	}

      gimp_procedural_db_query (query->str,
			        ".*", ".*", ".*", ".*", ".*", ".*", 
			        &num_procs, &proc_list);

      g_string_free (query, TRUE);
    }
  else if (widget == (dbbrowser->blurb_button))
    {
      gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), 
			    _("DB Browser (by blurb - please wait)"));
      gimp_procedural_db_query (".*", 
			        gtk_entry_get_text
				(GTK_ENTRY (dbbrowser->search_entry)),
			        ".*", ".*", ".*", ".*", ".*",
			        &num_procs, &proc_list);
    }
  else
    {
      gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), 
			    _("DB Browser (please wait)"));
      gimp_procedural_db_query (".*", ".*", ".*", ".*", ".*", ".*", ".*", 
			        &num_procs, &proc_list);
    }

  for (i = 0; i < num_procs; i++)
    {
      j = 0;
      while ((j < i) &&
	     (strcmp (gtk_clist_get_row_data (GTK_CLIST (dbbrowser->clist), j),
		      proc_list[i]) < 0))
	j++;

      label = g_strdup (proc_list[i]);
      convert_string (label);
      gtk_clist_insert (GTK_CLIST (GTK_CLIST(dbbrowser->clist)), j,
			&label);
      func_name = g_strdup (proc_list[i]);

      gtk_clist_set_row_data_full (GTK_CLIST (dbbrowser->clist), j,
				   func_name, g_free);
    }

  if (num_procs > 0)
    {
      dialog_select (dbbrowser,
		     gtk_clist_get_row_data (GTK_CLIST (dbbrowser->clist), 0));
      gtk_clist_select_row (GTK_CLIST (dbbrowser->clist), 0, 0);
    }

  /*
  if (num_procs != 0) {
    gchar *insert_name, *label_name;
    int i,j,savej;
    
    for (i = 0; i < num_procs ; i++) {

      insert_name=g_strdup(proc_list[0]); savej=0;
      for (j = 0; j < num_procs ; j++) {
	if (strcmp(proc_list[j],insert_name)<0) {
	  g_free(insert_name);
	  insert_name=g_strdup(proc_list[j]);
	  savej=j;
	}
      }
  
      proc_list[savej][0]='\255';

      label_name = g_strdup( insert_name );
      convert_string( label_name );
      gtk_clist_append (GTK_CLIST (dbbrowser->clist), &label_name);

      if (i==0) dialog_select( dbbrowser , insert_name );

      g_free(label_name);
    }
  }
  */
  
  if (dbbrowser->clist)
    {
      ;
    }
  
  g_free (proc_list);

  gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), _("DB Browser"));
  gtk_clist_thaw (GTK_CLIST (dbbrowser->clist));
}

/* utils ... */

static void 
convert_string (gchar *str)
{
  while (*str)
    {
      if (*str == '_') *str = '-';
      str++;
    }
}

static gchar * 
GParamType2char (GimpPDBArgType t)
{
  switch (t)
    {
    case GIMP_PDB_INT32: return "INT32";
    case GIMP_PDB_INT16: return "INT16";
    case GIMP_PDB_INT8: return "INT8";
    case GIMP_PDB_FLOAT: return "FLOAT";
    case GIMP_PDB_STRING: return "STRING";
    case GIMP_PDB_INT32ARRAY: return "INT32ARRAY";
    case GIMP_PDB_INT16ARRAY: return "INT16ARRAY";
    case GIMP_PDB_INT8ARRAY: return "INT8ARRAY";
    case GIMP_PDB_FLOATARRAY: return "FLOATARRAY";
    case GIMP_PDB_STRINGARRAY: return "STRINGARRAY";
    case GIMP_PDB_COLOR: return "COLOR";
    case GIMP_PDB_REGION: return "REGION";
    case GIMP_PDB_DISPLAY: return "DISPLAY";
    case GIMP_PDB_IMAGE: return "IMAGE";
    case GIMP_PDB_LAYER: return "LAYER";
    case GIMP_PDB_CHANNEL: return "CHANNEL";
    case GIMP_PDB_DRAWABLE: return "DRAWABLE";
    case GIMP_PDB_SELECTION: return "SELECTION";
    case GIMP_PDB_BOUNDARY: return "BOUNDARY";
    case GIMP_PDB_PATH: return "PATH";
    case GIMP_PDB_PARASITE: return "PARASITE";
    case GIMP_PDB_STATUS: return "STATUS";
    case GIMP_PDB_END: return "END";
    default: return "UNKNOWN?";
    }
}
