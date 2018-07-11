/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Copyright (C) 1999 Andy Thomas  alt@picnic.demon.co.uk
 *
 * Note some portions of the UI comes from the dbbrowser plugin.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <time.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-plug-in-details"
#define PLUG_IN_BINARY  "plugin-browser"
#define PLUG_IN_ROLE    "gimp-plugin-browser"
#define DBL_LIST_WIDTH  250
#define DBL_WIDTH       (DBL_LIST_WIDTH + 400)
#define DBL_HEIGHT      250


enum
{
  LIST_COLUMN_NAME,
  LIST_COLUMN_DATE,
  LIST_COLUMN_DATE_STRING,
  LIST_COLUMN_PATH,
  LIST_COLUMN_IMAGE_TYPES,
  LIST_COLUMN_PINFO,
  N_LIST_COLUMNS
};

enum
{
  TREE_COLUMN_PATH_NAME,
  TREE_COLUMN_DATE,
  TREE_COLUMN_DATE_STRING,
  TREE_COLUMN_IMAGE_TYPES,
  TREE_COLUMN_MPATH,
  TREE_COLUMN_PINFO,
  N_TREE_OLUMNS
};

typedef struct
{
  GtkWidget   *dialog;

  GtkWidget   *browser;

  GtkTreeView *list_view;
  GtkTreeView *tree_view;
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
static void        query (void);
static void        run   (const gchar      *name,
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
                                                   const gchar      *mpath,
                                                   GtkTreeIter      *return_iter);


const GimpPlugInInfo PLUG_IN_INFO =
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
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run-mode", "The run mode { RUN-INTERACTIVE (0) }" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Display information about plug-ins"),
                          "Allows one to browse the plug-in menus system. You "
                          "can search for plug-in names, sort by name or menu "
                          "location and you can view a tree representation "
                          "of the plug-in menus. Can also be of help to find "
                          "where new plug-ins have installed themselves in "
                          "the menus.",
                          "Andy Thomas",
                          "Andy Thomas",
                          "1999",
                          N_("_Plug-in Browser"),
                          "",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Help/Programming");
  gimp_plugin_icon_register (PLUG_IN_PROC, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) GIMP_ICON_PLUGIN);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;

  INIT_I18N ();

  if (strcmp (name, PLUG_IN_PROC) == 0)
    {
      *nreturn_vals = 1;

      values[0].data.d_status = GIMP_PDB_SUCCESS;

      browser_dialog_new ();
      gtk_main ();
    }
}

static gboolean
find_existing_mpath_helper (GtkTreeModel *model,
                            GtkTreeIter  *iter,
                            GtkTreePath  *path,
                            const gchar  *mpath,
                            GtkTreeIter  *return_iter)
{
  do
    {
      GtkTreeIter  child;
      gchar       *picked_mpath;

      gtk_tree_model_get (model, iter,
                          TREE_COLUMN_MPATH, &picked_mpath,
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
                     const gchar  *mpath,
                     GtkTreeIter  *return_iter)
{
  GtkTreePath *path = gtk_tree_path_new_first ();
  GtkTreeIter  parent;
  gboolean     found;

  if (! gtk_tree_model_get_iter (model, &parent, path))
    {
      gtk_tree_path_free (path);
      return FALSE;
    }

  found = find_existing_mpath_helper (model, &parent, path, mpath, return_iter);

  gtk_tree_path_free (path);

  return found;
}

static void
get_parent (PluginBrowser *browser,
            const gchar   *mpath,
            GtkTreeIter   *parent)
{
  GtkTreeIter   last_parent;
  gchar        *tmp_ptr;
  gchar        *str_ptr;
  GtkTreeStore *tree_store;

  if (! mpath)
    return;

  tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (browser->tree_view));

  /* Lookup for existing mpath */
  if (find_existing_mpath (GTK_TREE_MODEL (tree_store), mpath, parent))
    return;

  tmp_ptr = g_strdup (mpath);

  /* Strip off trailing ellipsis */
  str_ptr = strstr (mpath, "...");
  if (str_ptr && str_ptr == (mpath + strlen (mpath) - 3))
    *str_ptr = '\0';

  str_ptr = strrchr (tmp_ptr, '/');

  if (str_ptr == NULL)
    {
      gtk_tree_store_append (tree_store, parent, NULL);
      gtk_tree_store_set (tree_store, parent,
                          TREE_COLUMN_MPATH,     mpath,
                          TREE_COLUMN_PATH_NAME, mpath,
                          -1);
    }
  else
    {
      gchar  *leaf_ptr;

      leaf_ptr = g_strdup (str_ptr + 1);
      *str_ptr = '\0';

      get_parent (browser, tmp_ptr, &last_parent);
      gtk_tree_store_append (tree_store, parent, &last_parent);
      gtk_tree_store_set (tree_store, parent,
                          TREE_COLUMN_MPATH,     mpath,
                          TREE_COLUMN_PATH_NAME, leaf_ptr,
                          -1);

      g_free (leaf_ptr);
    }
}

static void
insert_into_tree_view (PluginBrowser *browser,
                       const gchar   *name,
                       gint64         xtime,
                       const gchar   *xtimestr,
                       const gchar   *menu_str,
                       const gchar   *types_str,
                       PInfo         *pinfo)
{
  gchar        *str_ptr;
  gchar        *tmp_ptr;
  GtkTreeIter   parent, iter;
  GtkTreeStore *tree_store;

  /* Find all nodes */
  /* Last one is the leaf part */

  tmp_ptr = g_strdup (menu_str);

  str_ptr = strrchr (tmp_ptr, '/');

  if (str_ptr == NULL)
  {
    g_free (tmp_ptr);
    return; /* No node */
  }

  *str_ptr = '\0';

  /*   printf("inserting %s...\n",menu_str); */

  get_parent (browser, tmp_ptr, &parent);

  tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (browser->tree_view));
  gtk_tree_store_append (tree_store, &iter, &parent);
  gtk_tree_store_set (tree_store, &iter,
                      TREE_COLUMN_MPATH,       menu_str,
                      TREE_COLUMN_PATH_NAME,   name,
                      TREE_COLUMN_IMAGE_TYPES, types_str,
                      TREE_COLUMN_DATE,        xtime,
                      TREE_COLUMN_DATE_STRING, xtimestr,
                      TREE_COLUMN_PINFO,       pinfo,
                      -1);
}

static void
browser_search (GimpBrowser   *gimp_browser,
                const gchar   *search_text,
                gint           search_type,
                PluginBrowser *browser)
{
  GimpParam    *return_vals;
  gint          nreturn_vals;
  gint          num_plugins;
  gchar        *str;
  GtkListStore *list_store;
  GtkTreeStore *tree_store;

  gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                             _("Searching by name"));

  return_vals = gimp_run_procedure ("gimp-plugins-query",
                                    &nreturn_vals,
                                    GIMP_PDB_STRING, search_text,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    num_plugins = return_vals[1].data.d_int32;
  else
    num_plugins = 0;

  if (! search_text || strlen (search_text) == 0)
    {
      str = g_strdup_printf (ngettext ("%d plug-in", "%d plug-ins",
                                       num_plugins),
                             num_plugins);
    }
  else
    {
      switch (num_plugins)
        {
        case 0:
          str = g_strdup (_("No matches for your query"));
          break;
        default:
          str = g_strdup_printf (ngettext ("%d plug-in matches your query",
                                           "%d plug-ins match your query",
                                           num_plugins), num_plugins);
          break;
        }
    }

  gtk_label_set_text (GTK_LABEL (gimp_browser->count_label), str);
  g_free (str);

  list_store = GTK_LIST_STORE (gtk_tree_view_get_model (browser->list_view));
  gtk_list_store_clear (list_store);

  tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (browser->tree_view));
  gtk_tree_store_clear (tree_store);

  if (num_plugins > 0)
    {
      GtkTreeSelection  *sel;
      GtkTreeIter        iter;
      gchar            **menu_strs;
      gchar            **accel_strs;
      gchar            **prog_strs;
      gchar            **types_strs;
      gchar            **realname_strs;
      gint              *time_ints;
      gint               i;

      menu_strs     = return_vals[2].data.d_stringarray;
      accel_strs    = return_vals[4].data.d_stringarray;
      prog_strs     = return_vals[6].data.d_stringarray;
      types_strs    = return_vals[8].data.d_stringarray;
      time_ints     = return_vals[10].data.d_int32array;
      realname_strs = return_vals[12].data.d_stringarray;

      for (i = 0; i < num_plugins; i++)
        {
          PInfo     *pinfo;
          gchar     *name;
          gchar      xtimestr[50];
          struct tm *x;
          time_t     tx;
          gint       ret;

          /* Strip off trailing ellipsis */
          name = strstr (menu_strs[i], "...");
          if (name && name == (menu_strs[i] + strlen (menu_strs[i]) - 3))
            *name = '\0';

          name = strrchr (menu_strs[i], '/');

          if (name)
            {
              *name = '\0';
              name = name + 1;
            }
          else
            {
              name = menu_strs[i];
            }

          tx = time_ints[i];
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
            {
              strcpy (xtimestr, "");
            }

          pinfo = g_new0 (PInfo, 1);

          pinfo->menu     = g_strdup (menu_strs[i]);
          pinfo->accel    = g_strdup (accel_strs[i]);
          pinfo->prog     = g_strdup (prog_strs[i]);
          pinfo->types    = g_strdup (types_strs[i]);
          pinfo->instime  = time_ints[i];
          pinfo->realname = g_strdup (realname_strs[i]);

          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (list_store, &iter,
                              LIST_COLUMN_NAME,        name,
                              LIST_COLUMN_DATE,        (gint64) tx,
                              LIST_COLUMN_DATE_STRING, xtimestr,
                              LIST_COLUMN_PATH,        menu_strs[i],
                              LIST_COLUMN_IMAGE_TYPES, types_strs[i],
                              LIST_COLUMN_PINFO,       pinfo,
                              -1);

          /* Now do the tree view.... */
          insert_into_tree_view (browser,
                                 name,
                                 (gint64) tx,
                                 xtimestr,
                                 menu_strs[i],
                                 types_strs[i],
                                 pinfo);
        }

      gtk_tree_view_columns_autosize (GTK_TREE_VIEW (browser->list_view));
      gtk_tree_view_columns_autosize (GTK_TREE_VIEW (browser->tree_view));

      gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (list_store),
                                            LIST_COLUMN_NAME,
                                            GTK_SORT_ASCENDING);
      gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (tree_store),
                                            TREE_COLUMN_PATH_NAME,
                                            GTK_SORT_ASCENDING);

      sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser->list_view));

      gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store),
                                     &iter);
      gtk_tree_selection_select_iter (sel, &iter);
    }
  else
    {
      gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                                 _("No matches"));
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

static GtkWidget *
browser_dialog_new (void)
{
  PluginBrowser     *browser;
  GtkWidget         *label, *notebook;
  GtkWidget         *scrolled_window;
  GtkListStore      *list_store;
  GtkTreeStore      *tree_store;
  GtkWidget         *list_view;
  GtkWidget         *tree_view;
  GtkWidget         *parent;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *renderer;
  GtkTreeSelection  *selection;
  GtkTreeIter        iter;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  browser = g_new0 (PluginBrowser, 1);

  browser->dialog = gimp_dialog_new (_("Plug-in Browser"), PLUG_IN_ROLE,
                                     NULL, 0,
                                     gimp_standard_help_func, PLUG_IN_PROC,

                                     _("_Close"), GTK_RESPONSE_CLOSE,

                                     NULL);

  g_signal_connect (browser->dialog, "response",
                    G_CALLBACK (browser_dialog_response),
                    browser);

  browser->browser = gimp_browser_new ();
  gtk_container_set_border_width (GTK_CONTAINER (browser->browser), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (browser->dialog))),
                      browser->browser, TRUE, TRUE, 0);
  gtk_widget_show (browser->browser);

  g_signal_connect (browser->browser, "search",
                    G_CALLBACK (browser_search),
                    browser);

  /* left = notebook */

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (GIMP_BROWSER (browser->browser)->left_vbox),
                      notebook, TRUE, TRUE, 0);

  /* list : list in a scrolled_win */
  list_store = gtk_list_store_new (N_LIST_COLUMNS,
                                   G_TYPE_STRING,
                                   G_TYPE_INT64,
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
                                                     "text", LIST_COLUMN_NAME,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, LIST_COLUMN_NAME);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Menu Path"),
                                                     renderer,
                                                     "text", LIST_COLUMN_PATH,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, LIST_COLUMN_PATH);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Image Types"),
                                                     renderer,
                                                     "text",
                                                     LIST_COLUMN_IMAGE_TYPES,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, LIST_COLUMN_IMAGE_TYPES);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  renderer = gtk_cell_renderer_text_new ();

  column = gtk_tree_view_column_new_with_attributes (_("Installation Date"),
                                                     renderer,
                                                     "text",
                                                     LIST_COLUMN_DATE_STRING,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, LIST_COLUMN_DATE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_widget_set_size_request (list_view, DBL_LIST_WIDTH, DBL_HEIGHT);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

  g_signal_connect (selection, "changed",
                    G_CALLBACK (browser_list_selection_changed),
                    browser);

  label = gtk_label_new (_("List View"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrolled_window, label);
  gtk_container_add (GTK_CONTAINER (scrolled_window), list_view);
  gtk_widget_show (list_view);
  gtk_widget_show (scrolled_window);

  /* notebook->ctree */
  tree_store = gtk_tree_store_new (N_LIST_COLUMNS,
                                   G_TYPE_STRING,
                                   G_TYPE_INT64,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_POINTER);

  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (tree_store));
  g_object_unref (tree_store);

  browser->tree_view = GTK_TREE_VIEW (tree_view);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Menu Path"),
                                                     renderer,
                                                     "text",
                                                     TREE_COLUMN_PATH_NAME,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, TREE_COLUMN_PATH_NAME);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Image Types"),
                                                     renderer,
                                                     "text",
                                                     TREE_COLUMN_IMAGE_TYPES,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, TREE_COLUMN_IMAGE_TYPES);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Installation Date"),
                                                     renderer,
                                                     "text",
                                                     TREE_COLUMN_DATE_STRING,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id  (column, TREE_COLUMN_DATE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (tree_view, DBL_LIST_WIDTH, DBL_HEIGHT);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

  g_signal_connect (selection, "changed",
                    G_CALLBACK (browser_tree_selection_changed),
                    browser);

  label = gtk_label_new (_("Tree View"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrolled_window, label);
  gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);

  gtk_widget_show (tree_view);
  gtk_widget_show (scrolled_window);
  gtk_widget_show (notebook);

  parent = gtk_widget_get_parent (GIMP_BROWSER (browser->browser)->right_vbox);
  parent = gtk_widget_get_parent (parent);

  gtk_widget_set_size_request (parent, DBL_WIDTH - DBL_LIST_WIDTH, -1);

  /* now build the list */
  browser_search (GIMP_BROWSER (browser->browser), "", 0, browser);

  gtk_widget_show (browser->dialog);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
    gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view)),
                                    &iter);

  return browser->dialog;
}

static void
browser_dialog_response (GtkWidget     *widget,
                         gint           response_id,
                         PluginBrowser *browser)
{
  gtk_widget_destroy (browser->dialog);
  gtk_main_quit ();
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
                          LIST_COLUMN_PINFO, &pinfo,
                          LIST_COLUMN_PATH,  &mpath,
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
                          TREE_COLUMN_PINFO, &pinfo,
                          TREE_COLUMN_MPATH, &mpath,
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
                          LIST_COLUMN_PATH, &picked_mpath,
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

  gimp_browser_set_widget (GIMP_BROWSER (browser->browser),
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
