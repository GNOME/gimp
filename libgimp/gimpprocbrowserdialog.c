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

#include "gimpprocbrowser.h"
#include "gimpprocview.h"

#include "libgimp/stdplugins-intl.h"


#define DBL_LIST_WIDTH 250
#define DBL_WIDTH      (DBL_LIST_WIDTH + 400)
#define DBL_HEIGHT     250


typedef enum
{
  SEARCH_TYPE_ALL,
  SEARCH_TYPE_NAME,
  SEARCH_TYPE_BLURB,
  SEARCH_TYPE_HELP,
  SEARCH_TYPE_AUTHOR,
  SEARCH_TYPE_COPYRIGHT,
  SEARCH_TYPE_DATE,
  SEARCH_TYPE_PROC_TYPE
} SearchType;

enum
{
  COLUMN_LABEL,
  COLUMN_PROC_NAME,
  N_COLUMNS
};

typedef struct
{
  GtkWidget        *dialog;

  GtkWidget        *browser;

  GtkListStore     *store;
  GtkTreeView      *tree_view;

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

static void       browser_selection_changed (GtkTreeSelection  *sel,
                                             GimpDBBrowser     *browser);
static void       browser_row_activated     (GtkTreeView       *treeview,
                                             GtkTreePath       *path,
                                             GtkTreeViewColumn *column,
                                             GimpDBBrowser     *browser);
static void       browser_show_procedure    (GimpDBBrowser     *browser,
                                             gchar             *proc_name);
static void       browser_search            (GimpBrowser       *browser,
                                             const gchar       *query_text,
                                             gint               search_type,
                                             GimpDBBrowser     *db_browser);
static void       browser_response          (GtkWidget         *widget,
                                             gint               response_id,
                                             GimpDBBrowser     *browser);
static void       browser_convert_string    (gchar             *str);


/*  public functions  */

GtkWidget *
gimp_proc_browser_dialog_new (gboolean                     scheme_names,
                              GimpProcBrowserApplyCallback apply_callback,
                              gpointer                     user_data)
{
  GimpDBBrowser    *browser;
  GtkWidget        *scrolled_window;
  GtkCellRenderer  *renderer;
  GtkTreeSelection *selection;

  browser = g_new0 (GimpDBBrowser, 1);

  browser->scheme_names   = scheme_names ? TRUE : FALSE;
  browser->apply_callback = apply_callback;
  browser->user_data      = user_data;

  browser->dialog = gimp_dialog_new (_("Procedure Browser"), "dbbrowser",
                                     NULL, 0,
                                     gimp_standard_help_func,
                                     "plug-in-db-browser",
                                     NULL);

  if (apply_callback)
    {
      gtk_dialog_add_button (GTK_DIALOG (browser->dialog),
                             GTK_STOCK_APPLY, GTK_RESPONSE_APPLY);
      gtk_dialog_set_default_response (GTK_DIALOG (browser->dialog),
                                       GTK_RESPONSE_APPLY);
    }

  gtk_dialog_add_button (GTK_DIALOG (browser->dialog),
                         GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  g_signal_connect (browser->dialog, "response",
                    G_CALLBACK (browser_response),
                    browser);

  browser->browser = gimp_browser_new ();
  gimp_browser_add_search_types (GIMP_BROWSER (browser->browser),
                                 _("by name"),        SEARCH_TYPE_NAME,
                                 _("by description"), SEARCH_TYPE_BLURB,
                                 _("by help"),        SEARCH_TYPE_HELP,
                                 _("by author"),      SEARCH_TYPE_AUTHOR,
                                 _("by copyright"),   SEARCH_TYPE_COPYRIGHT,
                                 _("by date"),        SEARCH_TYPE_DATE,
                                 _("by type"),        SEARCH_TYPE_PROC_TYPE,
                                 NULL);
  gtk_container_set_border_width (GTK_CONTAINER (browser->browser), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (browser->dialog)->vbox),
                     browser->browser);
  gtk_widget_show (browser->browser);

  g_signal_connect (browser->browser, "search",
                    G_CALLBACK (browser_search),
                    browser);

  /* list : list in a scrolled_win */

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (GIMP_BROWSER (browser->browser)->left_vbox),
                      scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  browser->tree_view = GTK_TREE_VIEW (gtk_tree_view_new ());

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_text_set_fixed_height_from_font
    (GTK_CELL_RENDERER_TEXT (renderer), 1);

  gtk_tree_view_insert_column_with_attributes (browser->tree_view,
                                               -1, NULL,
                                               renderer,
                                               "text", 0,
                                               NULL);
  gtk_tree_view_set_headers_visible (browser->tree_view, FALSE);

  g_signal_connect (browser->tree_view, "row_activated",
                    G_CALLBACK (browser_row_activated),
                    browser);

  gtk_widget_set_size_request (GTK_WIDGET (browser->tree_view),
                               DBL_LIST_WIDTH, DBL_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
                     GTK_WIDGET (browser->tree_view));
  gtk_widget_show (GTK_WIDGET (browser->tree_view));

  selection = gtk_tree_view_get_selection (browser->tree_view);

  g_signal_connect (selection, "changed",
                    G_CALLBACK (browser_selection_changed),
                    browser);

  gtk_widget_set_size_request (GIMP_BROWSER (browser->browser)->right_vbox->parent->parent,
                               DBL_WIDTH - DBL_LIST_WIDTH, -1);

  /* now build the list */

  gtk_widget_show (browser->dialog);

  /* initialize the "return" value (for "apply") */

  browser->proc_name      = NULL;
  browser->proc_blurb     = NULL;
  browser->proc_help      = NULL;
  browser->proc_author    = NULL;
  browser->proc_copyright = NULL;
  browser->proc_date      = NULL;
  browser->proc_type      = 0;
  browser->n_params       = 0;
  browser->n_return_vals  = 0;
  browser->params         = NULL;
  browser->return_vals    = NULL;

  /* first search (all procedures) */
  browser_search (GIMP_BROWSER (browser->browser), "", SEARCH_TYPE_ALL,
                  browser);

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

  gimp_browser_set_widget (GIMP_BROWSER (browser->browser),
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
browser_search (GimpBrowser   *gimp_browser,
                const gchar   *query_text,
                gint           search_type,
                GimpDBBrowser *browser)
{
  gchar **proc_list;
  gint    num_procs;
  gchar  *str;

  if (search_type == SEARCH_TYPE_NAME)
    {
      GString *query;

      gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                                 _("Searching by name - please wait"));

      query = g_string_new ("");

      while (*query_text)
        {
          if ((*query_text == '_') || (*query_text == '-'))
            g_string_append (query, "[-_]");
          else
            g_string_append_c (query, *query_text);

          query_text++;
        }

      gimp_procedural_db_query (query->str, ".*", ".*", ".*", ".*", ".*", ".*",
                                &num_procs, &proc_list);

      g_string_free (query, TRUE);
    }
  else if (search_type == SEARCH_TYPE_BLURB)
    {
      gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                                 _("Searching by description - please wait"));

      gimp_procedural_db_query (".*", query_text, ".*", ".*", ".*", ".*", ".*",
                                &num_procs, &proc_list);
    }
  else if (search_type == SEARCH_TYPE_HELP)
    {
      gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                                 _("Searching by help - please wait"));

      gimp_procedural_db_query (".*", ".*", query_text, ".*", ".*", ".*", ".*",
                                &num_procs, &proc_list);
    }
  else if (search_type == SEARCH_TYPE_AUTHOR)
    {
      gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                                 _("Searching by author - please wait"));

      gimp_procedural_db_query (".*", ".*", ".*", query_text, ".*", ".*", ".*",
                                &num_procs, &proc_list);
    }
  else if (search_type == SEARCH_TYPE_COPYRIGHT)
    {
      gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                                 _("Searching by copyright - please wait"));

      gimp_procedural_db_query (".*", ".*", ".*", ".*", query_text, ".*", ".*",
                                &num_procs, &proc_list);
    }
  else if (search_type == SEARCH_TYPE_DATE)
    {
      gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                                 _("Searching by date - please wait"));

      gimp_procedural_db_query (".*", ".*", ".*", ".*", ".*", query_text, ".*",
                                &num_procs, &proc_list);
    }
  else if (search_type == SEARCH_TYPE_PROC_TYPE)
    {
      gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                                 _("Searching by type - please wait"));

      gimp_procedural_db_query (".*", ".*", ".*", ".*", ".*", ".*", query_text,
                                &num_procs, &proc_list);
    }
  else
    {
      gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                                 _("Searching - please wait"));

      gimp_procedural_db_query (".*", ".*", ".*", ".*", ".*", ".*", ".*",
                                &num_procs, &proc_list);
    }

  if (num_procs == 1)
    str = g_strdup (_("1 Procedure"));
  else
    str = g_strdup_printf (_("%d Procedures"), num_procs);

  gtk_label_set_text (GTK_LABEL (gimp_browser->count_label), str);
  g_free (str);

  if (num_procs > 0)
    {
      GtkTreeSelection *selection;
      GtkTreeIter       iter;
      gint              i;

      browser->store = gtk_list_store_new (N_COLUMNS,
                                           G_TYPE_STRING,
                                           G_TYPE_STRING);
      gtk_tree_view_set_model (browser->tree_view,
                               GTK_TREE_MODEL (browser->store));
      g_object_unref (browser->store);

      for (i = 0; i < num_procs; i++)
        {
          str = g_strdup (proc_list[i]);

          if (browser->scheme_names)
            browser_convert_string (str);

          gtk_list_store_append (browser->store, &iter);
          gtk_list_store_set (browser->store, &iter,
                              COLUMN_LABEL,     str,
                              COLUMN_PROC_NAME, proc_list[i],
                              -1);

          g_free (str);
          g_free (proc_list[i]);
        }

      g_free (proc_list);

      gtk_tree_view_columns_autosize (browser->tree_view);

      gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (browser->store),
                                            COLUMN_LABEL, GTK_SORT_ASCENDING);

      gtk_tree_model_get_iter_first (GTK_TREE_MODEL (browser->store), &iter);
      selection = gtk_tree_view_get_selection (browser->tree_view);
      gtk_tree_selection_select_iter (selection, &iter);
    }
  else
    {
      gtk_tree_view_set_model (browser->tree_view, NULL);
      browser->store = NULL;

      gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                                 _("No matches"));
    }
}

static void
browser_response (GtkWidget     *widget,
                  gint           response_id,
                  GimpDBBrowser *browser)
{
  switch (response_id)
    {
    case GTK_RESPONSE_APPLY:
      if (browser->apply_callback)
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
