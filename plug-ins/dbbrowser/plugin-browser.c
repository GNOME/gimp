/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Copyright (C) 1999 Andy Thomas  alt@picnic.demon.co.uk
 *
 * Note some portions of the UI comes from the dbbrowser plugin.
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
#include <time.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpprocbox.h"
#include "gimpprocview.h"

#include "libgimp/stdplugins-intl.h"


enum
{
  LIST_NAME_COLUMN,
  LIST_DATE_COLUMN,
  LIST_PATH_COLUMN,
  LIST_IMAGE_TYPES_COLUMN,
  LIST_PINFO_COLUMN,
  LIST_N_COLUMNS
};

enum
{
  TREE_PATH_NAME_COLUMN,
  TREE_DATE_COLUMN,
  TREE_IMAGE_TYPES_COLUMN,
  TREE_MPATH_COLUMN,
  TREE_PINFO_COLUMN,
  TREE_N_COLUMNS
};

#define DBL_LIST_WIDTH  250
#define DBL_WIDTH       (DBL_LIST_WIDTH + 400)
#define DBL_HEIGHT      250

typedef struct
{
  GtkWidget   *dialog;

  GtkTreeView *list_view;
  GtkTreeView *tree_view;

  GtkWidget   *count_label;
  GtkWidget   *search_entry;
  GtkWidget   *proc_box;

  gint         num_plugins;
} PluginBrowser;

typedef struct
{
  gchar *menu;
  gchar *accel;
  gchar *prog;
  gchar *types;
  gchar *realname;
  gint  instime;
} PInfo;


/* Declare some local functions.
 */
static void   query      (void);
static void   run        (const gchar      *name,
                          gint              nparams,
                          const GimpParam  *param,
                          gint             *nreturn_vals,
                          GimpParam       **return_vals);


static GtkWidget * browser_dialog_new             (void);
static void        browser_dialog_response        (GtkWidget        *widget,
                                                   gint              response_id,
                                                   PluginBrowser    *browser);
static void        browser_list_selection_changed (GtkTreeSelection *selection,
                                                   PluginBrowser    *browser);
static void        browser_tree_selection_changed (GtkTreeSelection *selection,
                                                   PluginBrowser    *browser);
static void        browser_show_plugin            (PluginBrowser    *browser,
                                                   PInfo            *pinfo);

static gboolean    find_existing_mpath            (GtkTreeModel     *model,
                                                   gchar            *mpath,
                                                   GtkTreeIter      *return_iter);



static PluginBrowser *browser = NULL;

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()


static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, [non-interactive]" }
  };

  gimp_install_procedure ("plug_in_plug_in_details",
                          "Displays plug-in details",
                          "Allows to browse the plug-in menus system. You can "
                          "search for plug-in names, sort by name or menu "
                          "location and you can view a tree representation "
                          "of the plug-in menus. Can also be of help to find "
                          "where new plug-ins have installed themselves in "
                          "the menus.",
                          "Andy Thomas",
                          "Andy Thomas",
                          "1999",
                          N_("_Plug-In Browser"),
                          "",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register ("plug_in_plug_in_details",
                             "<Toolbox>/Xtns/Extensions");
  gimp_plugin_icon_register ("plug_in_plug_in_details",
                             GIMP_ICON_TYPE_STOCK_ID, GIMP_STOCK_PLUGIN);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;

  INIT_I18N ();

  if (strcmp (name, "plug_in_plug_in_details") == 0)
    {
      GtkWidget *plugin_dialog;

      *nreturn_vals = 1;

      values[0].data.d_status = GIMP_PDB_SUCCESS;

      plugin_dialog = browser_dialog_new ();

      gtk_main ();
    }
}

#if 0
static void
pinfo_free (gpointer p)
{
  PInfo *pinfo = p;

  g_free (pinfo->menu);
  g_free (pinfo->accel);
  g_free (pinfo->prog);
  g_free (pinfo->types);
  g_free (pinfo->realname);
  g_free (pinfo);
}
#endif

static gboolean
find_existing_mpath_helper (GtkTreeModel *model,
                            GtkTreeIter  *iter,
                            GtkTreePath  *path,
                            gchar        *mpath,
                            GtkTreeIter  *return_iter)
{
  do
    {
      GtkTreeIter  child;
      gchar       *picked_mpath;

      gtk_tree_model_get (model, iter,
                          TREE_MPATH_COLUMN, &picked_mpath,
                          -1);

      if (! strcmp (mpath, picked_mpath))
        {
          *return_iter = *iter;
          g_free (picked_mpath);
          return TRUE;
        }

      if (gtk_tree_model_iter_children (model, &child, iter))
        {
          gtk_tree_path_down (path);

          if (find_existing_mpath_helper (model, &child, path,
                                          mpath, return_iter))
            {
              g_free (picked_mpath);
              return TRUE;
            }

          gtk_tree_path_up (path);
        }

      gtk_tree_path_next (path);
      g_free (picked_mpath);
    }
  while (gtk_tree_model_iter_next (model, iter));

  return FALSE;
}

static gboolean
find_existing_mpath (GtkTreeModel *model,
                     gchar        *mpath,
                     GtkTreeIter  *return_iter)
{
  GtkTreePath *path;
  GtkTreeIter  parent;

  path = gtk_tree_path_new_first ();

  if (gtk_tree_model_get_iter (model, &parent, path) == FALSE)
    {
      gtk_tree_path_free (path);
      return FALSE;
    }

  return find_existing_mpath_helper (model, &parent, path,
                                     mpath, return_iter);
  gtk_tree_path_free (path);
}

static void
get_parent (PluginBrowser *browser,
            gchar         *mpath,
            GtkTreeIter   *parent)
{
  GtkTreeIter   last_parent;
  gchar        *tmp_ptr;
  gchar        *str_ptr;
  gchar        *leaf_ptr;
  GtkTreeStore *tree_store;

  if (mpath == NULL)
    return;

  tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (browser->tree_view));

  /* Lookup for existing mpath */
  if (find_existing_mpath (GTK_TREE_MODEL (tree_store), mpath, parent))
    return;

  /* Next one up */
  tmp_ptr = g_strdup (mpath);

  str_ptr = strrchr (tmp_ptr,'/');

  if (str_ptr == NULL)
    {
      leaf_ptr = mpath;
      gtk_tree_store_append (tree_store, parent, NULL);
      gtk_tree_store_set (tree_store, parent,
                          TREE_MPATH_COLUMN, mpath,
                          TREE_PATH_NAME_COLUMN, mpath,
                          -1);
    }
  else
    {
      leaf_ptr = g_strdup(str_ptr+1);

      *str_ptr = '\000';

      get_parent (browser, tmp_ptr, &last_parent);
      gtk_tree_store_append (tree_store, parent, &last_parent);
      gtk_tree_store_set (tree_store, parent,
                          TREE_MPATH_COLUMN, mpath,
                          TREE_PATH_NAME_COLUMN, leaf_ptr,
                          -1);
    }
}

static void
insert_into_tree_view (PluginBrowser *browser,
                       gchar         *name,
                       gchar         *xtimestr,
                       gchar         *menu_str,
                       gchar         *types_str,
                       PInfo         *pinfo)
{
  gchar        *labels[3];
  gchar        *str_ptr;
  gchar        *tmp_ptr;
  gchar        *leaf_ptr;
  GtkTreeIter   parent, iter;
  GtkTreeStore *tree_store;

  /* Find all nodes */
  /* Last one is the leaf part */

  tmp_ptr = g_strdup (menu_str);

  str_ptr = strrchr (tmp_ptr, '/');

  if (str_ptr == NULL)
    return; /* No node */

  leaf_ptr = g_strdup (str_ptr + 1);

  *str_ptr = '\000';

  /*   printf("inserting %s...\n",menu_str); */

  get_parent (browser, tmp_ptr, &parent);

  /* Last was a leaf */
  /*   printf("found leaf %s parent = %p\n",leaf_ptr,parent); */
  labels[0] = g_strdup (name);
  labels[1] = g_strdup (xtimestr);
  labels[2] = g_strdup (types_str);

  tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (browser->tree_view));
  gtk_tree_store_append (tree_store, &iter, &parent);
  gtk_tree_store_set (tree_store, &iter,
                      TREE_MPATH_COLUMN,       menu_str,
                      TREE_PATH_NAME_COLUMN,   name,
                      TREE_IMAGE_TYPES_COLUMN, types_str,
                      TREE_DATE_COLUMN,        xtimestr,
                      TREE_PINFO_COLUMN,       pinfo,
                      -1);
}

static void
get_plugin_info (PluginBrowser *browser,
                 const gchar   *search_text)
{
  GimpParam    *return_vals;
  gint          nreturn_vals;
  gchar       **menu_strs;
  gchar       **accel_strs;
  gchar       **prog_strs;
  gchar       **types_strs;
  gchar       **realname_strs;
  gint         *time_ints;
  GtkListStore *list_store;
  GtkTreeStore *tree_store;
  GtkTreeIter   iter;

  if (! search_text)
    search_text = "";

  gimp_proc_box_show_message (browser->proc_box,
                              _("Searching by name - please wait"));

  return_vals = gimp_run_procedure ("gimp_plugins_query",
                                    &nreturn_vals,
                                    GIMP_PDB_STRING, search_text,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    {
      gchar *str;
      gint   loop;

      browser->num_plugins = return_vals[1].data.d_int32;
      menu_strs            = return_vals[2].data.d_stringarray;
      accel_strs           = return_vals[4].data.d_stringarray;
      prog_strs            = return_vals[6].data.d_stringarray;
      types_strs           = return_vals[8].data.d_stringarray;
      time_ints            = return_vals[10].data.d_int32array;
      realname_strs        = return_vals[12].data.d_stringarray;

      if (browser->num_plugins == 1)
        str = g_strdup (_("1 Plug-In Interface"));
      else
        str = g_strdup_printf (_("%d Plug-In Interfaces"),
                               browser->num_plugins);

      gtk_label_set_text (GTK_LABEL (browser->count_label), str);
      g_free (str);

      list_store = GTK_LIST_STORE (gtk_tree_view_get_model (browser->list_view));
      gtk_list_store_clear (list_store);

      tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (browser->tree_view));
      gtk_tree_store_clear (tree_store);

      for (loop = 0; loop < return_vals[1].data.d_int32; loop++)
        {
          PInfo     *pinfo;
          gchar     *name;
          gchar      xtimestr[50];
          struct tm *x;
          time_t     tx;
          int        ret;

          name = strrchr (menu_strs[loop], '/');

          if (name)
            name = name + 1;
          else
            name = menu_strs[loop];

          pinfo = g_new0 (PInfo, 1);

          tx = time_ints[loop];
          if (tx)
            {
              const gchar *format = "%c";  /* gcc workaround to avoid warning */
              gchar       *utf8;

              x = localtime (&tx);
              ret = strftime (xtimestr, sizeof (xtimestr), format, x);
              xtimestr[ret] = 0;

              if ((utf8 = g_locale_to_utf8 (xtimestr, -1, NULL, NULL, NULL)))
                {
                  strncpy (xtimestr, utf8, sizeof (xtimestr));
                  xtimestr[sizeof (xtimestr) - 1] = 0;
                  g_free (utf8);
                }
            }
          else
            strcpy (xtimestr,"");

          pinfo->menu     = g_strdup (menu_strs[loop]);
          pinfo->accel    = g_strdup (accel_strs[loop]);
          pinfo->prog     = g_strdup (prog_strs[loop]);
          pinfo->types    = g_strdup (types_strs[loop]);
          pinfo->instime  = time_ints[loop];
          pinfo->realname = g_strdup (realname_strs[loop]);

          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (list_store, &iter,
                              LIST_NAME_COLUMN,        name,
                              LIST_DATE_COLUMN,        xtimestr,
                              LIST_PATH_COLUMN,        menu_strs[loop],
                              LIST_IMAGE_TYPES_COLUMN, types_strs[loop],
                              LIST_PINFO_COLUMN,       pinfo,
                              -1);

          /* Now do the tree view.... */
          insert_into_tree_view (browser,
                                 name,
                                 xtimestr,
                                 menu_strs[loop],
                                 types_strs[loop],
                                 pinfo);
        }

      gtk_tree_view_columns_autosize (GTK_TREE_VIEW (browser->list_view));
      gtk_tree_view_columns_autosize (GTK_TREE_VIEW (browser->tree_view));

      gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (list_store),
                                            LIST_NAME_COLUMN,
                                            GTK_SORT_ASCENDING);
      gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (tree_store),
                                            TREE_PATH_NAME_COLUMN,
                                            GTK_SORT_ASCENDING);

      if (browser->num_plugins)
        {
          GtkTreeSelection *sel =
            gtk_tree_view_get_selection (GTK_TREE_VIEW (browser->list_view));

          gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store),
                                         &iter);
          gtk_tree_selection_select_iter (sel, &iter);
        }
      else
        {
          gimp_proc_box_show_message (browser->proc_box, _("No matches"));
        }
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

static GtkWidget *
browser_dialog_new (void)
{
  GtkWidget         *paned;
  GtkWidget         *hbox, *vbox;
  GtkWidget         *label, *notebook, *swindow;
  GtkListStore      *list_store;
  GtkTreeStore      *tree_store;
  GtkWidget         *list_view;
  GtkWidget         *tree_view;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *renderer;
  GtkTreeSelection  *selection;
  GtkTreeIter        iter;

  gimp_ui_init ("plugindetails", FALSE);

  browser = g_new0 (PluginBrowser, 1);

  /* the dialog box */
  browser->dialog =
    gimp_dialog_new (_("Plug-In Browser"), "plugindetails",
                     NULL, 0,
                     gimp_standard_help_func, "plug-in-plug-in-details",

                     _("Search by _Name"), GTK_RESPONSE_OK,
                     GTK_STOCK_CLOSE,      GTK_RESPONSE_CLOSE,

                     NULL);

  g_signal_connect (browser->dialog, "response",
                    G_CALLBACK (browser_dialog_response),
                    browser);

  /* paned : left=notebook ; right=description */

  paned = gtk_hpaned_new ();
  gtk_container_set_border_width (GTK_CONTAINER (paned), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (browser->dialog)->vbox),
                     paned);
  gtk_widget_show (paned);

  /* left = vbox : the list and the search entry */

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_paned_pack1 (GTK_PANED (paned), vbox, FALSE, TRUE);
  gtk_widget_show (vbox);

  browser->count_label = gtk_label_new ("0 Plug-In Interfaces");
  gtk_misc_set_alignment (GTK_MISC (browser->count_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), browser->count_label, FALSE, FALSE, 0);
  gtk_widget_show (browser->count_label);

  /* left = notebook */

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

  /* list : list in a scrolled_win */
  list_store = gtk_list_store_new (LIST_N_COLUMNS,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_POINTER);

  list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
  g_object_unref (list_store);

  browser->list_view = GTK_TREE_VIEW (list_view);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                     renderer,
                                                     "text", LIST_NAME_COLUMN,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, LIST_NAME_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Insertion Date"),
                                                     renderer,
                                                     "text", LIST_DATE_COLUMN,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, LIST_DATE_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Menu Path"),
                                                     renderer,
                                                     "text", LIST_PATH_COLUMN,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, LIST_PATH_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Image Types"),
                                                     renderer,
                                                     "text",
                                                     LIST_IMAGE_TYPES_COLUMN,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, LIST_IMAGE_TYPES_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  swindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (swindow), 2);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swindow),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_widget_set_size_request (list_view, DBL_LIST_WIDTH, DBL_HEIGHT);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

  g_signal_connect (selection, "changed",
                    G_CALLBACK (browser_list_selection_changed),
                    browser);

  label = gtk_label_new (_("List View"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), swindow, label);
  gtk_container_add (GTK_CONTAINER (swindow), list_view);
  gtk_widget_show (list_view);
  gtk_widget_show (swindow);

  /* notebook->ctree */
  tree_store = gtk_tree_store_new (LIST_N_COLUMNS,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_POINTER);

  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (tree_store));
  g_object_unref (tree_store);

  browser->tree_view = GTK_TREE_VIEW (tree_view);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Menu Path/Name"),
                                                     renderer,
                                                     "text",
                                                     TREE_PATH_NAME_COLUMN,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, TREE_PATH_NAME_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Insertion Date"),
                                                     renderer,
                                                     "text",
                                                     TREE_DATE_COLUMN,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, TREE_DATE_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Image Types"),
                                                     renderer,
                                                     "text",
                                                     TREE_IMAGE_TYPES_COLUMN,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, TREE_IMAGE_TYPES_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  swindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (swindow), 2);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swindow),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (tree_view, DBL_LIST_WIDTH, DBL_HEIGHT);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

  g_signal_connect (selection, "changed",
                    G_CALLBACK (browser_tree_selection_changed),
                    browser);

  label = gtk_label_new (_("Tree View"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), swindow, label);
  gtk_container_add (GTK_CONTAINER (swindow), tree_view);

  gtk_widget_show (tree_view);
  gtk_widget_show (swindow);
  gtk_widget_show (notebook);

  /* search entry & details button */

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
  browser_dialog_response (NULL, GTK_RESPONSE_OK, browser);

  gtk_widget_show (browser->dialog);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
    gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view)),
                                    &iter);

  gtk_widget_grab_focus (browser->search_entry);

  return browser->dialog;
}

static void
browser_dialog_response (GtkWidget     *widget,
                         gint           response_id,
                         PluginBrowser *browser)
{
  const gchar *search_text = NULL;

  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      search_text = gtk_entry_get_text (GTK_ENTRY (browser->search_entry));
      get_plugin_info (browser, search_text);
      break;

    default:
      gtk_widget_destroy (browser->dialog);
      gtk_main_quit ();
      break;
    }
}

static void
browser_list_selection_changed (GtkTreeSelection *selection,
                                PluginBrowser    *browser)
{
  PInfo        *pinfo = NULL;
  GtkTreeIter   iter;
  GtkTreeModel *model;
  gchar        *mpath = NULL;

  g_return_if_fail (browser != NULL);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter,
                          LIST_PINFO_COLUMN, &pinfo,
                          LIST_PATH_COLUMN,  &mpath,
                          -1);
    }

  if (!pinfo || !mpath)
    return;

  model = gtk_tree_view_get_model (browser->tree_view);

  if (find_existing_mpath (model, mpath, &iter))
    {
      GtkTreeSelection *tree_selection;
      GtkTreePath      *tree_path;

      tree_path = gtk_tree_model_get_path (model, &iter);
      gtk_tree_view_expand_to_path (browser->tree_view, tree_path);
      tree_selection = gtk_tree_view_get_selection (browser->tree_view);

      g_signal_handlers_block_by_func (tree_selection,
                                       browser_tree_selection_changed,
                                       browser);
      gtk_tree_selection_select_iter (tree_selection, &iter);
      g_signal_handlers_unblock_by_func (tree_selection,
                                         browser_tree_selection_changed,
                                         browser);

      gtk_tree_view_scroll_to_cell (browser->tree_view,
                                    tree_path, NULL,
                                    TRUE, 0.5, 0.0);
    }
  else
    {
      g_warning ("Failed to find node in tree");
    }

  g_free (mpath);

  browser_show_plugin (browser, pinfo);
}

static void
browser_tree_selection_changed (GtkTreeSelection *selection,
                                PluginBrowser    *browser)
{
  PInfo        *pinfo = NULL;
  GtkTreeIter   iter;
  GtkTreeModel *model;
  gchar        *mpath = NULL;
  gboolean      valid, found;

  g_return_if_fail (browser != NULL);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter,
                          TREE_PINFO_COLUMN, &pinfo,
                          TREE_MPATH_COLUMN, &mpath,
                          -1);
    }

  if (!pinfo || !mpath)
    return;

  /* Get the first iter in the list */
  model = gtk_tree_view_get_model (browser->list_view);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  found = FALSE;

  while (valid)
    {
      /* Walk through the list, reading each row */
      gchar *picked_mpath;

      gtk_tree_model_get (model, &iter,
                          LIST_PATH_COLUMN, &picked_mpath,
                          -1);
      if (picked_mpath && !strcmp (mpath, picked_mpath))
        {
          found = TRUE;
          break;
        }
      g_free (picked_mpath);
      valid = gtk_tree_model_iter_next (model, &iter);
    }
  g_free (mpath);

  if (found)
    {
      GtkTreeSelection *list_selection;
      GtkTreePath      *tree_path;

      tree_path = gtk_tree_model_get_path (model, &iter);
      list_selection = gtk_tree_view_get_selection (browser->list_view);

      g_signal_handlers_block_by_func (list_selection,
                                       browser_list_selection_changed,
                                       browser);
      gtk_tree_selection_select_iter (list_selection, &iter);
      g_signal_handlers_unblock_by_func (list_selection,
                                         browser_list_selection_changed,
                                         browser);

      gtk_tree_view_scroll_to_cell (browser->list_view,
                                    tree_path, NULL,
                                    TRUE, 0.5, 0.0);
    }
  else
    {
      g_warning ("Failed to find node in list");
    }

  browser_show_plugin (browser, pinfo);
}

static void
browser_show_plugin (PluginBrowser *browser,
                     PInfo         *pinfo)
{
  gchar           *blurb         = NULL;
  gchar           *help          = NULL;
  gchar           *author        = NULL;
  gchar           *copyright     = NULL;
  gchar           *date          = NULL;
  GimpPDBProcType  type          = 0;
  gint             n_params      = 0;
  gint             n_return_vals = 0;
  GimpParamDef    *params        = NULL;
  GimpParamDef    *return_vals   = NULL;

  g_return_if_fail (browser != NULL);
  g_return_if_fail (pinfo != NULL);

  gimp_procedural_db_proc_info (pinfo->realname,
                                &blurb,
                                &help,
                                &author,
                                &copyright,
                                &date,
                                &type,
                                &n_params,
                                &n_return_vals,
                                &params,
                                &return_vals);

  gimp_proc_box_set_widget (browser->proc_box,
                            gimp_proc_view_new (pinfo->realname,
                                                pinfo->menu,
                                                blurb,
                                                help,
                                                author,
                                                copyright,
                                                date,
                                                type,
                                                n_params,
                                                n_return_vals,
                                                params,
                                                return_vals));

  g_free (blurb);
  g_free (help);
  g_free (author);
  g_free (copyright);
  g_free (date);

  gimp_destroy_paramdefs (params,      n_params);
  gimp_destroy_paramdefs (return_vals, n_return_vals);
}
