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
  gchar *procedure;
  gint   instime;
} PInfo;


typedef struct _Browser      Browser;
typedef struct _BrowserClass BrowserClass;

struct _Browser
{
  GimpPlugIn parent_instance;
};

struct _BrowserClass
{
  GimpPlugInClass parent_class;
};


/* Declare local functions.
 */

#define BROWSER_TYPE  (browser_get_type ())
#define BROWSER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BROWSER_TYPE, Browser))

GType                   browser_get_type         (void) G_GNUC_CONST;

static GList          * browser_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * browser_create_procedure (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * browser_run              (GimpProcedure        *procedure,
                                                  GimpProcedureConfig  *config,
                                                  gpointer              run_data);

static GtkWidget * browser_dialog_new             (void);
static void        browser_dialog_response        (GtkWidget        *widget,
                                                   gint              response_id,
                                                   PluginBrowser    *browser);
static void        browser_list_selection_changed (GtkTreeSelection *selection,
                                                   PluginBrowser    *browser);
static void        browser_tree_selection_changed (GtkTreeSelection *selection,
                                                   PluginBrowser    *browser);

static gboolean    find_existing_mpath            (GtkTreeModel     *model,
                                                   const gchar      *mpath,
                                                   GtkTreeIter      *return_iter);


G_DEFINE_TYPE (Browser, browser, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (BROWSER_TYPE)
DEFINE_STD_SET_I18N


static void
browser_class_init (BrowserClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = browser_query_procedures;
  plug_in_class->create_procedure = browser_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
browser_init (Browser *browser)
{
}

static GList *
browser_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
browser_create_procedure (GimpPlugIn  *plug_in,
                          const gchar *procedure_name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (procedure_name, PLUG_IN_PROC))
    {
      procedure = gimp_procedure_new (plug_in, procedure_name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      browser_run, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("_Plug-In Browser"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_PLUGIN);
      gimp_procedure_add_menu_path (procedure, "<Image>/Help/[Programming]");

      gimp_procedure_set_documentation (procedure,
                                        _("Display information about plug-ins"),
                                        _("Allows one to browse the plug-in "
                                          "menus system. You can search for "
                                          "plug-in names, sort by name or menu "
                                          "location and you can view a tree "
                                          "representation of the plug-in menus. "
                                          "Can also be of help to find where "
                                          "new plug-ins have installed "
                                          "themselves in the menus."),
                                        PLUG_IN_PROC);
      gimp_procedure_set_attribution (procedure,
                                      "Andy Thomas",
                                      "Andy Thomas",
                                      "1999");

      gimp_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        GIMP_TYPE_RUN_MODE,
                                        GIMP_RUN_INTERACTIVE,
                                        G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
browser_run (GimpProcedure        *procedure,
             GimpProcedureConfig  *config,
             gpointer              run_data)
{
  browser_dialog_new ();
  gtk_main ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
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
                       const gchar   *menu_path,
                       const gchar   *types_str,
                       PInfo         *pinfo)
{
  GtkTreeStore *tree_store;
  GtkTreeIter   parent, iter;

  get_parent (browser, menu_path, &parent);

  tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (browser->tree_view));
  gtk_tree_store_append (tree_store, &iter, &parent);
  gtk_tree_store_set (tree_store, &iter,
                      TREE_COLUMN_MPATH,       menu_path,
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
  GimpProcedure  *procedure;
  GimpValueArray *return_vals;
  const gchar   **procedure_strs;
  gint            num_plugins = 0;
  gchar          *str;
  GtkListStore   *list_store;
  GtkTreeStore   *tree_store;

  gimp_browser_show_message (GIMP_BROWSER (browser->browser),
                             _("Searching by name"));

  procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (),
                                           "gimp-plug-ins-query");
  return_vals = gimp_procedure_run (procedure,
                                    "search-string", search_text,
                                    NULL);

  if (GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS)
    {
      procedure_strs = GIMP_VALUES_GET_STRV (return_vals, 1);
      num_plugins = g_strv_length ((gchar **) procedure_strs);
    }

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

  gimp_browser_set_search_summary (gimp_browser, str);
  g_free (str);

  list_store = GTK_LIST_STORE (gtk_tree_view_get_model (browser->list_view));
  gtk_list_store_clear (list_store);

  tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (browser->tree_view));
  gtk_tree_store_clear (tree_store);

  if (num_plugins > 0)
    {
      GtkTreeSelection  *sel;
      GtkTreeIter        iter;
      const gchar      **accel_strs;
      const gchar      **prog_strs;
      const gint        *time_ints;
      gint               i;

      accel_strs     = GIMP_VALUES_GET_STRV (return_vals, 2);
      prog_strs      = GIMP_VALUES_GET_STRV (return_vals, 3);
      time_ints      = GIMP_VALUES_GET_INT32_ARRAY  (return_vals, 5);

      for (i = 0; i < num_plugins; i++)
        {
          GimpProcedure *procedure;
          const gchar   *types;
          PInfo         *pinfo;
          gchar         *menu_label;
          gchar         *tmp;
          GList         *menu_paths;
          const gchar   *menu_path;
          gchar          xtimestr[50];
          struct tm     *x;
          time_t         tx;
          gint           ret;

          procedure = gimp_pdb_lookup_procedure (gimp_get_pdb (),
                                                 procedure_strs[i]);

          types      = gimp_procedure_get_image_types (procedure);
          menu_label = g_strdup (gimp_procedure_get_menu_label (procedure));
          menu_paths = gimp_procedure_get_menu_paths (procedure);

          menu_path = menu_paths->data;

          /* Strip off trailing ellipsis */
          tmp = strstr (menu_label, "...");
          if (tmp && tmp == (menu_label + strlen (menu_label) - 3))
            *tmp = '\0';

          tmp = gimp_strip_uline (menu_label);
          g_free (menu_label);
          menu_label = tmp;

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
                  g_strlcpy (xtimestr, utf8, sizeof (xtimestr));
                  g_free (utf8);
                }
            }
          else
            {
              strcpy (xtimestr, "");
            }

          pinfo = g_new0 (PInfo, 1);

          pinfo->menu      = g_strdup (menu_path);
          pinfo->accel     = g_strdup (accel_strs[i]);
          pinfo->prog      = g_strdup (prog_strs[i]);
          pinfo->instime   = time_ints[i];
          pinfo->procedure = g_strdup (procedure_strs[i]);

          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (list_store, &iter,
                              LIST_COLUMN_NAME,        menu_label,
                              LIST_COLUMN_DATE,        (gint64) tx,
                              LIST_COLUMN_DATE_STRING, xtimestr,
                              LIST_COLUMN_PATH,        menu_path,
                              LIST_COLUMN_IMAGE_TYPES, types,
                              LIST_COLUMN_PINFO,       pinfo,
                              -1);

          /* Now do the tree view.... */
          insert_into_tree_view (browser,
                                 menu_label,
                                 (gint64) tx,
                                 xtimestr,
                                 menu_path,
                                 types,
                                 pinfo);

          g_free (menu_label);
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

  gimp_value_array_unref (return_vals);
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

  gimp_ui_init (PLUG_IN_BINARY);

  browser = g_new0 (PluginBrowser, 1);

  browser->dialog = gimp_dialog_new (_("Plug-in Browser"), PLUG_IN_ROLE,
                                     NULL, 0,
                                     gimp_standard_help_func, PLUG_IN_PROC,

                                     _("_Close"), GTK_RESPONSE_CLOSE,

                                     NULL);
  gtk_window_set_default_size (GTK_WINDOW (browser->dialog), DBL_WIDTH,
                               DBL_WIDTH - DBL_LIST_WIDTH);

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
  gtk_box_pack_start (GTK_BOX (gimp_browser_get_left_vbox (GIMP_BROWSER (browser->browser))),
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

  parent = gtk_widget_get_parent (gimp_browser_get_right_vbox (GIMP_BROWSER (browser->browser)));
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

  gimp_browser_set_widget (GIMP_BROWSER (browser->browser),
                           gimp_proc_view_new (pinfo->procedure));
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

  gimp_browser_set_widget (GIMP_BROWSER (browser->browser),
                           gimp_proc_view_new (pinfo->procedure));
}
