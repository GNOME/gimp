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

#include "gimpprocbox.h"
#include "gimpprocbrowser.h"
#include "gimpprocview.h"

#include "libgimp/stdplugins-intl.h"


#define RESPONSE_SEARCH       1
#define RESPONSE_SEARCH_NAME  2
#define RESPONSE_SEARCH_BLURB 3

#define DBL_LIST_WIDTH 250
#define DBL_WIDTH      (DBL_LIST_WIDTH + 400)
#define DBL_HEIGHT     250

enum
{
  COLUMN_LABEL,
  COLUMN_PROC_NAME,
  N_COLUMNS
};

typedef struct
{
  GtkWidget        *dialog;

  GtkWidget        *count_label;
  GtkWidget        *search_entry;
  GtkWidget        *proc_box;

  GtkListStore     *store;
  GtkWidget        *tv;
  GtkTreeSelection *sel;

  /* the currently selected procedure */
  gchar            *proc_name;
  gchar            *proc_blurb;
  gchar            *proc_help;
  gchar            *proc_author;
  gchar            *proc_copyright;
  gchar            *proc_date;
  GimpPDBProcType   proc_type;
  gint              n_params;
  gint              n_return_vals;
  GimpParamDef     *params;
  GimpParamDef     *return_vals;

  gboolean                     scheme_names;
  GimpProcBrowserApplyCallback apply_callback;
  gpointer                     user_data;
} GimpDBBrowser;


/*  local function prototypes  */

static void   browser_response          (GtkWidget         *widget,
                                         gint               response_id,
                                         GimpDBBrowser     *browser);
static void   browser_selection_changed (GtkTreeSelection  *sel,
                                         GimpDBBrowser     *browser);
static void   browser_row_activated     (GtkTreeView       *treeview,
                                         GtkTreePath       *path,
                                         GtkTreeViewColumn *column,
                                         GimpDBBrowser     *browser);
static void   browser_show_procedure    (GimpDBBrowser     *browser,
                                         gchar             *proc_name);
static void   browser_convert_string    (gchar             *str);


/*  public functions  */

GtkWidget *
gimp_proc_browser_dialog_new (gboolean                     scheme_names,
                              GimpProcBrowserApplyCallback apply_callback,
                              gpointer                     user_data)
{
  GimpDBBrowser   *browser;
  GtkWidget       *paned;
  GtkWidget       *hbox;
  GtkWidget       *vbox;
  GtkWidget       *label;
  GtkWidget       *scrolled_window;
  GtkCellRenderer *renderer;

  browser = g_new0 (GimpDBBrowser, 1);

  browser->scheme_names   = scheme_names ? TRUE : FALSE;
  browser->apply_callback = apply_callback;
  browser->user_data      = user_data;

  if (apply_callback)
    {
      browser->dialog =
        gimp_dialog_new (_("Procedure Browser"), "dbbrowser",
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-db-browser",

                         _("Search by _Name"),  RESPONSE_SEARCH_NAME,
                         _("Search by _Blurb"), RESPONSE_SEARCH_BLURB,
                         GTK_STOCK_APPLY,       GTK_RESPONSE_APPLY,
                         GTK_STOCK_CLOSE,       GTK_RESPONSE_CLOSE,

                         NULL);
    }
  else
    {
      browser->dialog =
        gimp_dialog_new (_("Procedure Browser"), "dbbrowser",
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-db-browser",

                         _("Search by _Name"),  RESPONSE_SEARCH_NAME,
                         _("Search by _Blurb"), RESPONSE_SEARCH_BLURB,
                         GTK_STOCK_CLOSE,       GTK_RESPONSE_CLOSE,

                         NULL);
    }

  gtk_dialog_set_default_response (GTK_DIALOG (browser->dialog),
                                   RESPONSE_SEARCH_NAME);

  g_signal_connect (browser->dialog, "response",
                    G_CALLBACK (browser_response),
                    browser);

  /* paned : left=list ; right=description */

  paned = gtk_hpaned_new ();
  gtk_container_set_border_width (GTK_CONTAINER (paned), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (browser->dialog)->vbox),
                     paned);
  gtk_widget_show (paned);

  /* left = vbox : the list and the search entry */

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_paned_pack1 (GTK_PANED (paned), vbox, FALSE, TRUE);
  gtk_widget_show (vbox);

  browser->count_label = gtk_label_new ("0 Procedures");
  gtk_misc_set_alignment (GTK_MISC (browser->count_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), browser->count_label, FALSE, FALSE, 0);
  gtk_widget_show (browser->count_label);

  /* list : list in a scrolled_win */

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  browser->tv = gtk_tree_view_new ();

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_text_set_fixed_height_from_font
    (GTK_CELL_RENDERER_TEXT (renderer), 1);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (browser->tv),
                                               -1, NULL,
                                               renderer,
                                               "text", 0,
                                               NULL);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (browser->tv), FALSE);

  if (apply_callback)
    g_signal_connect (browser->tv, "row_activated",
                      G_CALLBACK (browser_row_activated),
                      browser);

  gtk_widget_set_size_request (browser->tv, DBL_LIST_WIDTH, DBL_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_window), browser->tv);
  gtk_widget_show (browser->tv);

  browser->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser->tv));

  g_signal_connect (browser->sel, "changed",
                    G_CALLBACK (browser_selection_changed),
                    browser);

  /* search entry */

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Search:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  browser->search_entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (browser->search_entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), browser->search_entry, TRUE, TRUE, 0);
  gtk_widget_show (browser->search_entry);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), browser->search_entry);

  /* right = description */

  browser->proc_box = gimp_proc_box_new ();
  gtk_widget_set_size_request (browser->proc_box,
                               DBL_WIDTH - DBL_LIST_WIDTH, -1);
  gtk_paned_pack2 (GTK_PANED (paned), browser->proc_box, TRUE, TRUE);
  gtk_widget_show (browser->proc_box);

  /* now build the list */

  gtk_widget_show (browser->dialog);

  /* initialize the "return" value (for "apply") */

  browser->proc_name        = NULL;
  browser->proc_blurb       = NULL;
  browser->proc_help        = NULL;
  browser->proc_author      = NULL;
  browser->proc_copyright   = NULL;
  browser->proc_date        = NULL;
  browser->proc_type        = 0;
  browser->n_params         = 0;
  browser->n_return_vals    = 0;
  browser->params           = NULL;
  browser->return_vals      = NULL;

  /* first search (all procedures) */
  browser_response (browser->dialog, RESPONSE_SEARCH, browser);

  gtk_widget_grab_focus (browser->search_entry);

  return browser->dialog;
}


/*  private functions  */

static void
browser_selection_changed (GtkTreeSelection *sel,
                           GimpDBBrowser    *browser)
{
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      gchar *proc_name;

      gtk_tree_model_get (GTK_TREE_MODEL (browser->store), &iter,
                          COLUMN_PROC_NAME, &proc_name,
                          -1);
      browser_show_procedure (browser, proc_name);
      g_free (proc_name);
    }
}

static void
browser_row_activated (GtkTreeView       *treeview,
                       GtkTreePath       *path,
                       GtkTreeViewColumn *column,
                       GimpDBBrowser     *browser)
{
  browser_response (browser->dialog, GTK_RESPONSE_APPLY, browser);
}

static void
browser_show_procedure (GimpDBBrowser *browser,
                        gchar         *proc_name)
{
  g_free (browser->proc_name);
  browser->proc_name = g_strdup (proc_name);

  if (browser->scheme_names)
    browser_convert_string (browser->proc_name);

  g_free (browser->proc_blurb);
  g_free (browser->proc_help);
  g_free (browser->proc_author);
  g_free (browser->proc_copyright);
  g_free (browser->proc_date);

  gimp_destroy_paramdefs (browser->params,      browser->n_params);
  gimp_destroy_paramdefs (browser->return_vals, browser->n_return_vals);

  gimp_procedural_db_proc_info (proc_name,
                                &browser->proc_blurb,
                                &browser->proc_help,
                                &browser->proc_author,
                                &browser->proc_copyright,
                                &browser->proc_date,
                                &browser->proc_type,
                                &browser->n_params,
                                &browser->n_return_vals,
                                &browser->params,
                                &browser->return_vals);

  gimp_proc_box_set_widget (browser->proc_box,
                            gimp_proc_view_new (browser->proc_name,
                                                NULL,
                                                browser->proc_blurb,
                                                browser->proc_help,
                                                browser->proc_author,
                                                browser->proc_copyright,
                                                browser->proc_date,
                                                browser->proc_type,
                                                browser->n_params,
                                                browser->n_return_vals,
                                                browser->params,
                                                browser->return_vals));
}

static void
browser_response (GtkWidget     *widget,
                  gint           response_id,
                  GimpDBBrowser *browser)
{
  switch (response_id)
    {
    case GTK_RESPONSE_APPLY:
      browser->apply_callback (browser->proc_name,
                               browser->proc_blurb,
                               browser->proc_help,
                               browser->proc_author,
                               browser->proc_copyright,
                               browser->proc_date,
                               browser->proc_type,
                               browser->n_params,
                               browser->n_return_vals,
                               browser->params,
                               browser->return_vals,
                               browser->user_data);
      break;

    case RESPONSE_SEARCH:
    case RESPONSE_SEARCH_NAME:
    case RESPONSE_SEARCH_BLURB:
      {
        gchar       **proc_list;
        gint          num_procs;
        gchar        *str;
        gint          i;
        const gchar  *query_text;
        GString      *query;
        GtkTreeIter   iter;

        gtk_tree_view_set_model (GTK_TREE_VIEW (browser->tv), NULL);

        /* search */

        if (response_id == RESPONSE_SEARCH_NAME)
          {
            gimp_proc_box_show_message (browser->proc_box,
                                        _("Searching by name - please wait"));

            query = g_string_new ("");
            query_text = gtk_entry_get_text (GTK_ENTRY (browser->search_entry));

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
        else if (response_id == RESPONSE_SEARCH_BLURB)
          {
            gimp_proc_box_show_message (browser->proc_box,
                                        _("Searching by blurb - please wait"));

            gimp_procedural_db_query (".*",
                                      (gchar *) gtk_entry_get_text
                                      (GTK_ENTRY (browser->search_entry)),
                                      ".*", ".*", ".*", ".*", ".*",
                                      &num_procs, &proc_list);
          }
        else
          {
            gimp_proc_box_show_message (browser->proc_box,
                                        _("Searching - please wait"));

            gimp_procedural_db_query (".*", ".*", ".*", ".*", ".*", ".*", ".*",
                                      &num_procs, &proc_list);
          }

        if (num_procs == 1)
          str = g_strdup (_("1 Procedure"));
        else
          str = g_strdup_printf (_("%d Procedures"), num_procs);

        gtk_label_set_text (GTK_LABEL (browser->count_label), str);
        g_free (str);

        browser->store = gtk_list_store_new (N_COLUMNS,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (browser->tv),
                                 GTK_TREE_MODEL (browser->store));
        g_object_unref (browser->store);

        for (i = 0; i < num_procs; i++)
          {
            gchar *label;

            label = g_strdup (proc_list[i]);

            if (browser->scheme_names)
              browser_convert_string (label);

            gtk_list_store_append (browser->store, &iter);
            gtk_list_store_set (browser->store, &iter,
                                COLUMN_LABEL,     label,
                                COLUMN_PROC_NAME, proc_list[i],
                                -1);

            g_free (label);
            g_free (proc_list[i]);
          }

        g_free (proc_list);

        gtk_tree_view_columns_autosize (GTK_TREE_VIEW (browser->tv));

        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (browser->store),
                                              COLUMN_LABEL, GTK_SORT_ASCENDING);

        if (num_procs > 0)
          {
            gtk_tree_model_get_iter_first (GTK_TREE_MODEL (browser->store),
                                           &iter);
            gtk_tree_selection_select_iter (browser->sel, &iter);
          }
        else
          {
            gimp_proc_box_show_message (browser->proc_box, _("No matches"));
          }
      }
      break;

    default:
      if (browser->apply_callback)
        {
          /* we are called by another application:
           * just destroy the dialog box
           */
          gtk_widget_destroy (browser->dialog);
        }
      else
        {
          /* we are in the plug_in:
           * quit the gtk application
           */
          gtk_main_quit ();
        }
      break;
    }
}

static void
browser_convert_string (gchar *str)
{
  while (*str)
    {
      if (*str == '_')
        *str = '-';

      str++;
    }
}
