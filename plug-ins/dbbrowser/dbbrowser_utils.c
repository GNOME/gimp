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


#define DBL_LIST_WIDTH 220
#define DBL_WIDTH      (DBL_LIST_WIDTH + 400)
#define DBL_HEIGHT     250


typedef struct
{
  GtkWidget        *dialog;

  GtkWidget        *search_entry;
  GtkWidget        *name_button;
  GtkWidget        *blurb_button;

  GtkWidget        *descr_vbox;
  GtkWidget        *description;

  GtkListStore     *store;
  GtkWidget        *tv;
  GtkTreeSelection *sel;

  /* the currently selected procedure */
  gchar            *selected_proc_name;
  gchar            *selected_scheme_proc_name;
  gchar            *selected_proc_blurb;
  gchar            *selected_proc_help;
  gchar            *selected_proc_author;
  gchar            *selected_proc_copyright;
  gchar            *selected_proc_date;
  GimpPDBProcType   selected_proc_type;
  gint              selected_nparams;
  gint              selected_nreturn_vals;
  GimpParamDef     *selected_params;
  GimpParamDef     *selected_return_vals; 

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

static void         dialog_apply_callback     (GtkWidget        *widget,
					       dbbrowser_t      *dbbrowser);
static gint         procedure_select_callback (GtkTreeSelection *sel,
					       dbbrowser_t      *dbbrowser);
static void         dialog_search_callback    (GtkWidget        *widget, 
					       dbbrowser_t      *dbbrowser);
static void         dialog_select             (dbbrowser_t      *dbbrowser, 
					       gchar            *proc_name);
static void         dialog_close_callback     (GtkWidget        *widget, 
					       dbbrowser_t      *dbbrowser);
static void         convert_string            (gchar            *str);
static const gchar *GParamType2char           (GimpPDBArgType    t);

/* create the dialog box
 * console_entry != NULL => called from the script-fu-console
 */
GtkWidget *
gimp_db_browser (GimpDBBrowserApplyCallback apply_callback)
{
  dbbrowser_t *dbbrowser;
  GtkWidget   *hpaned;
  GtkWidget   *searchhbox;
  GtkWidget   *vbox;
  GtkWidget   *label;
  GtkWidget   *scrolled_window;

  dbbrowser = g_new0 (dbbrowser_t, 1);
  
  dbbrowser->apply_callback = apply_callback;
  
  /* the dialog box */

  if (apply_callback)
    {
      dbbrowser->dialog =
	gimp_dialog_new (_("DB Browser"), "dbbrowser",
			 gimp_standard_help_func, "filters/dbbrowser.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("Search by _Name"), dialog_search_callback,
			 dbbrowser, NULL, &dbbrowser->name_button, TRUE, FALSE,
			 _("Search by _Blurb"), dialog_search_callback,
			 dbbrowser, NULL, &dbbrowser->blurb_button, FALSE, FALSE,
			 GTK_STOCK_APPLY, dialog_apply_callback,
			 dbbrowser, NULL, NULL, FALSE, FALSE,
			 GTK_STOCK_CLOSE, dialog_close_callback,
			 dbbrowser, NULL, NULL, FALSE, TRUE,

			 NULL);
    }
  else
    {
      dbbrowser->dialog =
	gimp_dialog_new (_("DB Browser"), "dbbrowser",
			 gimp_standard_help_func, "filters/dbbrowser.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("Search by _Name"), dialog_search_callback,
			 dbbrowser, NULL, &dbbrowser->name_button, TRUE, FALSE,
			 _("Search by _Blurb"), dialog_search_callback,
			 dbbrowser, NULL, &dbbrowser->blurb_button, FALSE, FALSE,
			 GTK_STOCK_CLOSE, dialog_close_callback,
			 dbbrowser, NULL, NULL, FALSE, TRUE,

			 NULL);
    }

  g_signal_connect (dbbrowser->dialog, "destroy",
                    G_CALLBACK (dialog_close_callback), dbbrowser);
  
  /* hpaned : left=list ; right=description */

  hpaned = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dialog)->vbox), 
		      hpaned, TRUE, TRUE, 0);
  gtk_widget_show (hpaned);

  /* left = vbox : the list and the search entry */
  
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_paned_pack1 (GTK_PANED (hpaned), vbox, FALSE, TRUE);
  gtk_widget_show (vbox);

  /* list : list in a scrolled_win */
  
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
				       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  dbbrowser->tv = gtk_tree_view_new ();

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (dbbrowser->tv),
					       -1, NULL,
					       gtk_cell_renderer_text_new (),
					       "text", 0, NULL);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dbbrowser->tv), FALSE);

  gtk_widget_set_size_request (dbbrowser->tv, DBL_LIST_WIDTH, DBL_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_window), dbbrowser->tv);
  gtk_widget_show (dbbrowser->tv);

  dbbrowser->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (dbbrowser->tv));
  g_signal_connect (dbbrowser->sel, "changed",
		    G_CALLBACK (procedure_select_callback), dbbrowser);

  /* search entry */

  searchhbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), searchhbox, FALSE, FALSE, 2);
  gtk_widget_show (searchhbox);

  label = gtk_label_new_with_mnemonic (_("_Search:"));
  gtk_box_pack_start (GTK_BOX (searchhbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  dbbrowser->search_entry = gtk_entry_new ();
  g_signal_connect_swapped (dbbrowser->search_entry, "activate",
			    G_CALLBACK (gtk_window_activate_default),
			    dbbrowser->dialog);
  gtk_box_pack_start (GTK_BOX (searchhbox), dbbrowser->search_entry,
		      TRUE, TRUE, 0);
  gtk_widget_show (dbbrowser->search_entry);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dbbrowser->search_entry);

  /* right = description */

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrolled_window, DBL_WIDTH - DBL_LIST_WIDTH, -1);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_ALWAYS);
  gtk_paned_pack2 (GTK_PANED (hpaned), scrolled_window, TRUE, TRUE);
  gtk_widget_show (scrolled_window);

  dbbrowser->descr_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (dbbrowser->descr_vbox), 4);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
					 dbbrowser->descr_vbox);
  gtk_widget_show (dbbrowser->descr_vbox);

  /* now build the list */

  gtk_widget_show (dbbrowser->dialog);

  /* initialize the "return" value (for "apply") */

  dbbrowser->description               = NULL;
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
  dialog_search_callback (NULL, dbbrowser);

  return dbbrowser->dialog;
}

static gint
procedure_select_callback (GtkTreeSelection *sel,
			   dbbrowser_t      *dbbrowser)
{
  GtkTreeIter  iter;
  gchar       *func;

  g_return_val_if_fail (sel != NULL, FALSE);
  g_return_val_if_fail (dbbrowser != NULL, FALSE);

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dbbrowser->store), &iter,
			  1, &func,
			  -1);
      dialog_select (dbbrowser, func);
      g_free (func);
    }

  return FALSE;
}

/* update the description box (right) */
static void 
dialog_select (dbbrowser_t *dbbrowser, 
	       gchar       *proc_name)
{
  GtkWidget   *old_description;
  GtkWidget   *label;
  GtkWidget   *sep;
  gint         i;
  gint         row = 0;
  const gchar *type;

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
  old_description = dbbrowser->description;

  dbbrowser->description = gtk_table_new (10 +
					  dbbrowser->selected_nparams +
					  dbbrowser->selected_nreturn_vals,
					  5, FALSE);

  gtk_table_set_col_spacings (GTK_TABLE (dbbrowser->description), 6);
  gtk_table_set_row_spacing (GTK_TABLE (dbbrowser->description), 0, 2);

  /* show the name */

  label = gtk_label_new (dbbrowser->selected_scheme_proc_name);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);

  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
			     _("Name:"), 1.0, 0.5,
			     label, 3, FALSE);

  /* show the description */

  label = gtk_label_new (dbbrowser->selected_proc_blurb);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
			     _("Blurb:"), 1.0, 0.5,
			     label, 3, FALSE);

  sep = gtk_hseparator_new ();
  gtk_table_attach (GTK_TABLE (dbbrowser->description), sep,
		    0, 4, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
  gtk_widget_show (sep);
  row++;

  /* in parameters */
  if (dbbrowser->selected_nparams) 
    {
      label = gtk_label_new (_("In:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			0, 1, row, row + (dbbrowser->selected_nparams), 
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      for (i = 0; i < dbbrowser->selected_nparams; i++) 
	{
	  /* name */
	  label = gtk_label_new ((dbbrowser->selected_params[i]).name);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			    1, 2, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  /* type */
	  type = GParamType2char ((dbbrowser->selected_params[i]).type);
	  label = gtk_label_new (type);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			    2, 3, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  /* description */
	  label = gtk_label_new ((dbbrowser->selected_params[i]).description);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	  gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			    3, 4, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  row++;
	}
    }

  if ((dbbrowser->selected_nparams) && 
      (dbbrowser->selected_nreturn_vals))
    {
      sep = gtk_hseparator_new ();
      gtk_table_attach (GTK_TABLE (dbbrowser->description), sep,
			0, 4, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
      gtk_widget_show (sep);
      row++;
    }

  /* out parameters */
  if (dbbrowser->selected_nreturn_vals)
    {
      label = gtk_label_new (_("Out:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			0, 1, row, row + (dbbrowser->selected_nreturn_vals), 
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      for (i = 0; i < dbbrowser->selected_nreturn_vals; i++) 
	{
	  /* name */
	  label = gtk_label_new ((dbbrowser->selected_return_vals[i]).name);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			    1, 2, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  /* type */
	  type = GParamType2char (dbbrowser->selected_return_vals[i].type);
	  label = gtk_label_new (type);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			    2, 3, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  /* description */
	  label = gtk_label_new (dbbrowser->selected_return_vals[i].description);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			    3, 4, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  row++;
	}
    }

  if (dbbrowser->selected_nparams || 
      dbbrowser->selected_nreturn_vals)
    {
      sep = gtk_hseparator_new ();
      gtk_table_attach (GTK_TABLE (dbbrowser->description), sep,
			0, 4, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
      gtk_widget_show (sep);
      row++;
    }

  /* show the help */
  if (dbbrowser->selected_proc_help &&
      (strlen (dbbrowser->selected_proc_help) > 1))
    {
      label = gtk_label_new (dbbrowser->selected_proc_help);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

      gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
				 _("Help:"), 1.0, 0.5,
				 label, 3, FALSE);

      sep = gtk_hseparator_new ();
      gtk_table_attach (GTK_TABLE (dbbrowser->description), sep,
			0, 4, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
      gtk_widget_show (sep);

      row++;
    }

  /* show the author & the copyright */

  label = gtk_label_new (dbbrowser->selected_proc_author);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gtk_table_set_row_spacing (GTK_TABLE (dbbrowser->description), row, 2);
  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
			     _("Author:"), 1.0, 0.5,
			     label, 3, FALSE);

  label = gtk_label_new (dbbrowser->selected_proc_date);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gtk_table_set_row_spacing (GTK_TABLE (dbbrowser->description), row, 2);
  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
			     _("Date:"), 1.0, 0.5,
			     label, 3, FALSE);

  label = gtk_label_new (dbbrowser->selected_proc_copyright);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gtk_table_set_row_spacing (GTK_TABLE (dbbrowser->description), row, 2);
  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
			     _("Copyright:"), 1.0, 0.5,
			     label, 3, FALSE);

  if (old_description)
    gtk_container_remove (GTK_CONTAINER (dbbrowser->descr_vbox),
                          old_description);

  gtk_box_pack_start (GTK_BOX (dbbrowser->descr_vbox),
                      dbbrowser->description, FALSE, FALSE, 0);
  gtk_widget_show (dbbrowser->description);
}

static void
dialog_show_message (dbbrowser_t *dbbrowser,
                     const gchar *message)
{
  if (dbbrowser->description && GTK_IS_LABEL (dbbrowser->description))
    {
      gtk_label_set_text (GTK_LABEL (dbbrowser->description), message);
    }
  else
    {
      if (dbbrowser->description)
        gtk_container_remove (GTK_CONTAINER (dbbrowser->descr_vbox),
                              dbbrowser->description);
      
      dbbrowser->description = gtk_label_new (message);
      gtk_box_pack_start (GTK_BOX (dbbrowser->descr_vbox),
                          dbbrowser->description, FALSE, FALSE, 0);
      gtk_widget_show (dbbrowser->description);
    }

  while (gtk_events_pending ())
    gtk_main_iteration ();
}

/* end of the dialog */
static void
dialog_close_callback (GtkWidget   *widget, 
		       dbbrowser_t *dbbrowser)
{
  if (dbbrowser->apply_callback)
    {
      /* we are called by another application : just destroy the dialog box */
      gtk_widget_destroy (dbbrowser->dialog);
    }
  else
    {
      /* we are in the plug_in : kill the gtk application */
      gtk_main_quit ();
    }
}

/* end of the dialog */
static void 
dialog_apply_callback (GtkWidget   *widget, 
		       dbbrowser_t *dbbrowser)
{
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
dialog_search_callback (GtkWidget   *widget, 
		        dbbrowser_t *dbbrowser)
{
  gchar       **proc_list;
  gint          num_procs;
  gint          i;
  gchar        *label;
  const gchar  *query_text;
  GString      *query;
  GtkTreeIter   iter;

  if (dbbrowser->store)
    {
      gtk_list_store_clear (dbbrowser->store);

      /* Perhaps I'm too stupid but I can't find a proper way of
         keeping the list store from sorting itself while new items
         are added. Since this _slow_, we unset the store here to
         force creation of a new one that doesn't sort and activate
         sorting later.
      */
      dbbrowser->store = NULL;
    }

  /* search */

  if (widget == (dbbrowser->name_button))
    {
      dialog_show_message (dbbrowser, _("Searching by name - please wait"));

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
      dialog_show_message (dbbrowser, _("Searching by blurb - please wait"));

      gimp_procedural_db_query (".*", 
			        (gchar *) gtk_entry_get_text
				  (GTK_ENTRY (dbbrowser->search_entry)),
			        ".*", ".*", ".*", ".*", ".*",
			        &num_procs, &proc_list);
    }
  else
    {
      dialog_show_message (dbbrowser, _("Searching - please wait"));
      gimp_procedural_db_query (".*", ".*", ".*", ".*", ".*", ".*", ".*", 
			        &num_procs, &proc_list);
    }

  if (!dbbrowser->store)
    {
      dbbrowser->store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
      gtk_tree_view_set_model (GTK_TREE_VIEW (dbbrowser->tv),
                               GTK_TREE_MODEL (dbbrowser->store));
      g_object_unref (dbbrowser->store);
    }

  for (i = 0; i < num_procs; i++)
    {
      label = g_strdup (proc_list[i]);
      convert_string (label);
      gtk_list_store_append (dbbrowser->store, &iter);
      gtk_list_store_set (dbbrowser->store, &iter,
			  0, label,
			  1, proc_list[i],
			  -1);
      g_free (label);
      g_free (proc_list[i]);
    }

  g_free (proc_list);

  /* now sort the store */ 
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (dbbrowser->store),
                                        0, GTK_SORT_ASCENDING);
  
  if (num_procs > 0)
    {
      gtk_tree_model_get_iter_root (GTK_TREE_MODEL (dbbrowser->store), &iter);
      gtk_tree_selection_select_iter (dbbrowser->sel, &iter);
    }
  else
    {
      dialog_show_message (dbbrowser, _("No matches"));
    }
}

/* utils ... */

static void 
convert_string (gchar *str)
{
  while (*str)
    {
      if (*str == '_')
	*str = '-';

      str++;
    }
}

static const gchar * 
GParamType2char (GimpPDBArgType t)
{
  switch (t)
    {
    case GIMP_PDB_INT32:       return "INT32";
    case GIMP_PDB_INT16:       return "INT16";
    case GIMP_PDB_INT8:        return "INT8";
    case GIMP_PDB_FLOAT:       return "FLOAT";
    case GIMP_PDB_STRING:      return "STRING";
    case GIMP_PDB_INT32ARRAY:  return "INT32ARRAY";
    case GIMP_PDB_INT16ARRAY:  return "INT16ARRAY";
    case GIMP_PDB_INT8ARRAY:   return "INT8ARRAY";
    case GIMP_PDB_FLOATARRAY:  return "FLOATARRAY";
    case GIMP_PDB_STRINGARRAY: return "STRINGARRAY";
    case GIMP_PDB_COLOR:       return "COLOR";
    case GIMP_PDB_REGION:      return "REGION";
    case GIMP_PDB_DISPLAY:     return "DISPLAY";
    case GIMP_PDB_IMAGE:       return "IMAGE";
    case GIMP_PDB_LAYER:       return "LAYER";
    case GIMP_PDB_CHANNEL:     return "CHANNEL";
    case GIMP_PDB_DRAWABLE:    return "DRAWABLE";
    case GIMP_PDB_SELECTION:   return "SELECTION";
    case GIMP_PDB_BOUNDARY:    return "BOUNDARY";
    case GIMP_PDB_PATH:        return "PATH";
    case GIMP_PDB_PARASITE:    return "PARASITE";
    case GIMP_PDB_STATUS:      return "STATUS";
    case GIMP_PDB_END:         return "END";
    default:                   return "UNKNOWN?";
    }
}
