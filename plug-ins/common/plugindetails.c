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

#include "libgimp/stdplugins-intl.h"


#define DBL_LIST_WIDTH  250
#define DBL_WIDTH       (DBL_LIST_WIDTH + 300)
#define DBL_HEIGHT      200

typedef struct
{
  GtkTreeView *list_view;
  GtkTreeView *tree_view;
  GtkWidget   *dlg;
  GtkWidget   *search_entry;
  GtkWidget   *descr_scroll;
  GtkWidget   *info_table;
  GtkWidget   *paned;
  gint         num_plugins;
  gboolean     details_showing;
} PDesc;

typedef struct
{
  gchar *menu;
  gchar *accel;
  gchar *prog;
  gchar *types;
  gchar *realname;
  gint  instime;
} PInfo;

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

/* Declare some local functions.
 */
static void   query      (void);
static void   run        (const gchar      *name,
                          gint              nparams,
                          const GimpParam  *param,
                          gint             *nreturn_vals,
                          GimpParam       **return_vals);


static GtkWidget * gimp_plugin_desc           (void);

static gboolean    find_existing_mpath        (GtkTreeModel     *model,
                                               gchar            *mpath,
                                               GtkTreeIter      *return_iter);

static void        list_store_select_callback (GtkTreeSelection *selection,
                                               PDesc            *pdesc);
static void        tree_store_select_callback (GtkTreeSelection *selection,
                                               PDesc            *pdesc);

static void        procedure_general_select_callback (PDesc *pdesc,
                                                      PInfo *pinfo);


static gchar *proc_type_str[] =
{
  N_("Internal GIMP procedure"),
  N_("GIMP Plug-In"),
  N_("GIMP Extension"),
  N_("Temporary Procedure")
};

static PDesc *plugindesc = NULL;

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
                          "Displays plugin details",
                          "Helps browse the plugin menus system. You can "
                          "search for plugin names, sort by name or menu "
                          "location and you can view a tree representation "
                          "of the plugin menus. Can also be of help to find "
                          "where new plugins have installed themselves in "
                          "the menuing system",
                          "Andy Thomas",
                          "Andy Thomas",
                          "1999",
                          N_("_Plugin Details"),
                          "",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register ("plug_in_plug_in_details",
                             N_("<Toolbox>/Xtns/Extensions"));
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

      plugin_dialog = gimp_plugin_desc ();

      gtk_main ();
    }
}

/* Bit of a fiddle but sorta has the effect I want... */

static void
details_callback (GtkWidget *widget,
                  PDesc     *pdesc)
{
  GtkLabel         *lab = GTK_LABEL (GTK_BIN (widget)->child);
  GtkTreeSelection *list_selection;
  GtkTreeIter       iter;

  /* This is a lame hack:
     We add the description on the right on the first details_callback.
     Otherwise the window reacts quite weird on resizes */
  if (pdesc->descr_scroll == NULL)
    {
      pdesc->descr_scroll = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pdesc->descr_scroll),
                                      GTK_POLICY_ALWAYS,
                                      GTK_POLICY_ALWAYS);
      gtk_widget_set_size_request (pdesc->descr_scroll,
                                   DBL_WIDTH - DBL_LIST_WIDTH, -1);
      gtk_paned_pack2 (GTK_PANED (pdesc->paned), pdesc->descr_scroll,
                       FALSE, TRUE);
      list_selection = gtk_tree_view_get_selection (pdesc->list_view);
      if (gtk_tree_selection_get_selected (list_selection, NULL, &iter))
        list_store_select_callback (list_selection, pdesc);
    }

  if (pdesc->details_showing == FALSE)
    {
      GTK_PANED (pdesc->paned)->child1_resize = FALSE;
      gtk_label_set_text (lab, _("Details <<"));
      gtk_widget_show (pdesc->descr_scroll);
      pdesc->details_showing = TRUE;
    }
  else
    {
      GTK_PANED (pdesc->paned)->child1_resize = TRUE;
      GTK_PANED (pdesc->paned)->child2_resize = TRUE;

      gtk_label_set_text (lab, _("Details >>"));
      gtk_widget_hide (pdesc->descr_scroll);
      gtk_paned_set_position (GTK_PANED (pdesc->paned),
                              GTK_PANED (pdesc->paned)->child1->allocation.width);
      pdesc->details_showing = FALSE;
    }
}

static gchar *
format_menu_path (gchar *s)
{
  gchar **str_array;
  gchar  *newstr = NULL;

  if (!s)
    return s;

  str_array = g_strsplit (s, "/", 0);

  newstr = g_strjoinv ("->", str_array);

  g_strfreev (str_array);

  return newstr;
}

static void
procedure_general_select_callback (PDesc *pdesc,
                                   PInfo *pinfo)
{
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
  GtkWidget       *label;
  GtkWidget       *help;
  GtkWidget       *text_view;
  GtkTextBuffer   *text_buffer;
  GtkWidget       *old_table;
  gint             table_row = 0;
  gchar           *str;
  GtkWidget       *separator;

#define ADD_SEPARATOR                                                         \
G_STMT_START                                                                  \
{                                                                             \
  separator = gtk_hseparator_new ();                                          \
  gtk_table_attach (GTK_TABLE (pdesc->info_table), separator,                 \
                    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);  \
  gtk_widget_show (separator);                                                \
  table_row++;                                                                \
}                                                                             \
G_STMT_END

  g_return_if_fail (pdesc != NULL);
  g_return_if_fail (pinfo != NULL);

  if (pdesc->descr_scroll == NULL)
    return;

  selected_proc_blurb     = NULL;
  selected_proc_help      = NULL;
  selected_proc_author    = NULL;
  selected_proc_copyright = NULL;
  selected_proc_date      = NULL;
  selected_proc_type      = 0;
  selected_nparams        = 0;
  selected_nreturn_vals   = 0;
  selected_params         = NULL;
  selected_return_vals    = NULL;

  gimp_procedural_db_proc_info (pinfo->realname,
                                &selected_proc_blurb,
                                &selected_proc_help,
                                &selected_proc_author,
                                &selected_proc_copyright,
                                &selected_proc_date,
                                &selected_proc_type,
                                &selected_nparams, &selected_nreturn_vals,
                                &selected_params,  &selected_return_vals);

  old_table = pdesc->info_table;

  pdesc->info_table = gtk_table_new (9, 5, FALSE);

  gtk_container_set_border_width (GTK_CONTAINER (pdesc->info_table), 12);
  gtk_table_set_col_spacings (GTK_TABLE (pdesc->info_table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (pdesc->info_table), 6);


  /* Number of plugins */

  str = g_strdup_printf (_("Number of Plugin Interfaces: %d"),
                         pdesc->num_plugins);
  label = gtk_label_new (str);
  g_free (str);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
                    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  table_row++;

  ADD_SEPARATOR;

  /* menu path */

  label = gtk_label_new (format_menu_path (pinfo->menu));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gimp_table_attach_aligned (GTK_TABLE (pdesc->info_table), 0, table_row,
                             _("Menu Path:"), 0.0, 0.0,
                             label, 3, FALSE);
  table_row++;

  ADD_SEPARATOR;

  /* show the name */

  label = gtk_label_new (pinfo->realname);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gimp_table_attach_aligned (GTK_TABLE (pdesc->info_table), 0, table_row,
                             _("Name:"), 0.0, 0.0,
                             label, 3, FALSE);
  table_row++;

  ADD_SEPARATOR;

  /* show the description */

  label = gtk_label_new (selected_proc_blurb);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gimp_table_attach_aligned (GTK_TABLE (pdesc->info_table), 0, table_row,
                             _("Blurb:"), 0.0, 0.0,
                             label, 3, FALSE);
  table_row++;

  ADD_SEPARATOR;

  /* show the help */
  if (selected_proc_help && (strlen (selected_proc_help) > 1))
    {
      help = gtk_table_new (2, 2, FALSE);
      gtk_table_set_row_spacing (GTK_TABLE (help), 0, 2);
      gtk_table_set_col_spacing (GTK_TABLE (help), 0, 2);
      gimp_table_attach_aligned (GTK_TABLE (pdesc->info_table), 0, table_row,
                                 _("Help:"), 0.0, 0.0,
                                 help, 3, FALSE);
      table_row++;

      text_buffer = gtk_text_buffer_new  (NULL);
      gtk_text_buffer_set_text (text_buffer, selected_proc_help, -1);

      text_view = gtk_text_view_new_with_buffer (text_buffer);
      g_object_unref (text_buffer);

      gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
      gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);

      gtk_widget_set_size_request (text_view, -1, 60);
      gtk_table_attach (GTK_TABLE (help), text_view, 0, 1, 0, 1,
                        GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                        GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (text_view);

      ADD_SEPARATOR;
    }

  /* show the type */

  label = gtk_label_new (gettext (proc_type_str[selected_proc_type]));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gimp_table_attach_aligned (GTK_TABLE (pdesc->info_table), 0, table_row,
                             _("Type:"), 0.0, 0.0,
                             label, 3, FALSE);
  table_row++;

  /* Remove old and replace with new */

  if (old_table)
    gtk_widget_destroy (old_table);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (pdesc->descr_scroll),
                                         pdesc->info_table);

  gtk_widget_show (pdesc->info_table);

  if (selected_proc_blurb)
    g_free (selected_proc_blurb);
  if (selected_proc_help)
    g_free (selected_proc_help);
  if (selected_proc_author)
    g_free (selected_proc_author);
  if (selected_proc_copyright)
    g_free (selected_proc_copyright);
  if (selected_proc_date)
    g_free (selected_proc_date);
  if (selected_params)
    g_free (selected_params);
  if (selected_return_vals)
    g_free (selected_return_vals);
}

static void
list_store_select_callback (GtkTreeSelection *selection,
                            PDesc            *pdesc)
{
  PInfo        *pinfo = NULL;
  GtkTreeIter   iter;
  GtkTreeModel *model;
  gchar        *mpath = NULL;

  g_return_if_fail (pdesc != NULL);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter,
                          LIST_PINFO_COLUMN, &pinfo,
                          LIST_PATH_COLUMN,  &mpath,
                          -1);
    }

  if (!pinfo || !mpath)
    return;

  model = gtk_tree_view_get_model (pdesc->tree_view);
  if (find_existing_mpath (model, mpath, &iter))
    {
      GtkTreeSelection *tree_selection;
      GtkTreePath *tree_path;

      tree_path = gtk_tree_model_get_path (model, &iter);
      gtk_tree_view_expand_to_path (pdesc->tree_view, tree_path);
      tree_selection = gtk_tree_view_get_selection (pdesc->tree_view);
      g_signal_handlers_block_by_func (tree_selection,
                                       G_CALLBACK (tree_store_select_callback),
                                       pdesc);
      gtk_tree_selection_select_iter (tree_selection, &iter);
      g_signal_handlers_unblock_by_func (tree_selection,
                                         G_CALLBACK (tree_store_select_callback),
                                         pdesc);
      gtk_tree_view_scroll_to_cell (pdesc->tree_view,
                                    tree_path, NULL,
                                    TRUE, 0.5, 0.0);
    }
  else
    {
      g_warning ("Failed to find node in tree");
    }
  g_free (mpath);
  procedure_general_select_callback (pdesc, pinfo);
}

static void
tree_store_select_callback (GtkTreeSelection *selection,
                            PDesc            *pdesc)
{
  PInfo        *pinfo = NULL;
  GtkTreeIter   iter;
  GtkTreeModel *model;
  gchar        *mpath = NULL;
  gboolean      valid, found;

  g_return_if_fail (pdesc != NULL);

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
  model = gtk_tree_view_get_model (pdesc->list_view);
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
      list_selection = gtk_tree_view_get_selection (pdesc->list_view);
      g_signal_handlers_block_by_func (list_selection,
                                       G_CALLBACK (list_store_select_callback),
                                       pdesc);
      gtk_tree_selection_select_iter (list_selection, &iter);
      g_signal_handlers_unblock_by_func (list_selection,
                                         G_CALLBACK (list_store_select_callback),
                                         pdesc);
      gtk_tree_view_scroll_to_cell (pdesc->list_view,
                                    tree_path, NULL,
                                    TRUE, 0.5, 0.0);
    }
  else
    {
      g_warning ("Failed to find node in list");
    }

  procedure_general_select_callback (pdesc, pinfo);
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
      if (!strcmp(mpath, picked_mpath))
      {
        *return_iter = *iter;
        g_free (picked_mpath);
        return TRUE;
      }

      if (gtk_tree_model_iter_children (model, &child, iter))
        {
          gtk_tree_path_down (path);
          if (find_existing_mpath_helper (model, &child, path,
                                          mpath, return_iter)  )
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
get_parent (PDesc       *pdesc,
            gchar       *mpath,
            GtkTreeIter *parent)
{
  GtkTreeIter   last_parent;
  gchar        *tmp_ptr;
  gchar        *str_ptr;
  gchar        *leaf_ptr;
  GtkTreeView  *tree_view;
  GtkTreeStore *tree_store;

  if (mpath == NULL)
    return;

  tree_view  = pdesc->tree_view;
  tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (tree_view));

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

      get_parent (pdesc, tmp_ptr, &last_parent);
      gtk_tree_store_append (tree_store, parent, &last_parent);
      gtk_tree_store_set (tree_store, parent,
                          TREE_MPATH_COLUMN, mpath,
                          TREE_PATH_NAME_COLUMN, leaf_ptr,
                          -1);
    }
}

static void
insert_into_tree_view (PDesc *pdesc,
                       gchar *name,
                       gchar *xtimestr,
                       gchar *menu_str,
                       gchar *types_str,
                       PInfo *pinfo)
{
  gchar        *labels[3];
  gchar        *str_ptr;
  gchar        *tmp_ptr;
  gchar        *leaf_ptr;
  GtkTreeIter   parent, iter;
  GtkTreeView  *tree_view;
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

  get_parent (pdesc, tmp_ptr, &parent);

  /* Last was a leaf */
  /*   printf("found leaf %s parent = %p\n",leaf_ptr,parent); */
  labels[0] = g_strdup (name);
  labels[1] = g_strdup (xtimestr);
  labels[2] = g_strdup (types_str);

  tree_view  = pdesc->tree_view;
  tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (tree_view));
  gtk_tree_store_append (tree_store, &iter, &parent);
  gtk_tree_store_set (tree_store, &iter,
                      TREE_MPATH_COLUMN, menu_str,
                      TREE_PATH_NAME_COLUMN, name,
                      TREE_PINFO_COLUMN, pinfo,
                      TREE_IMAGE_TYPES_COLUMN, types_str,
                      TREE_DATE_COLUMN, xtimestr,
                      -1);
}

static void
get_plugin_info (PDesc       *pdesc,
                 const gchar *search_text)
{
  GimpParam    *return_vals;
  gint          nreturn_vals;
  gchar       **menu_strs;
  gchar       **accel_strs;
  gchar       **prog_strs;
  gchar       **types_strs;
  gchar       **realname_strs;
  gint         *time_ints;
  GtkTreeView  *list_view;
  GtkListStore *list_store;
  GtkTreeView  *tree_view;
  GtkTreeStore *tree_store;
  GtkTreeIter   iter;

  if (!search_text)
    search_text = "";

  return_vals = gimp_run_procedure ("gimp_plugins_query",
                                    &nreturn_vals,
                                    GIMP_PDB_STRING, search_text,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    {
      int loop;
      pdesc->num_plugins = return_vals[1].data.d_int32;
      menu_strs          = return_vals[2].data.d_stringarray;
      accel_strs         = return_vals[4].data.d_stringarray;
      prog_strs          = return_vals[6].data.d_stringarray;
      types_strs         = return_vals[8].data.d_stringarray;
      time_ints          = return_vals[10].data.d_int32array;
      realname_strs      = return_vals[12].data.d_stringarray;

      list_view  = pdesc->list_view;
      list_store = GTK_LIST_STORE (gtk_tree_view_get_model (list_view));
      gtk_list_store_clear (list_store);

      tree_view  = pdesc->tree_view;
      tree_store = GTK_TREE_STORE (gtk_tree_view_get_model (tree_view));
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
              gchar *utf8;

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
          insert_into_tree_view (pdesc,
                                 name,
                                 xtimestr,
                                 menu_strs[loop],
                                 types_strs[loop],
                                 pinfo);
        }
      gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (list_store),
                                            LIST_NAME_COLUMN,
                                            GTK_SORT_ASCENDING);
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

#if 0
static gint
date_sort (GtkCList      *clist,
           gconstpointer  ptr1,
           gconstpointer  ptr2)
{
  GtkCListRow *row1 = (GtkCListRow *) ptr1;
  GtkCListRow *row2 = (GtkCListRow *) ptr2;

  /* Get the data for the row */
  PInfo *row1_pinfo = row1->data;
  PInfo *row2_pinfo = row2->data;

  /* Want to sort on the date field */

  if (row2_pinfo->instime < row1_pinfo->instime)
    {
      return -1;
    }

  if (row2_pinfo->instime > row1_pinfo->instime)
    {
      return 1;
    }
  return 0;
}
#endif

#if 0
static void
clist_click_column (GtkCList *clist,
                    gint      column,
                    gpointer  data)
{
  if (column == 1)
    {
      gtk_clist_set_compare_func (clist, date_sort);
    }
  else
    {
      gtk_clist_set_compare_func (clist, NULL); /* Set back to default */
    }

  if (column == clist->sort_column)
    {
      if (clist->sort_type == GTK_SORT_ASCENDING)
        clist->sort_type = GTK_SORT_DESCENDING;
      else
        clist->sort_type = GTK_SORT_ASCENDING;
    }
  else
    gtk_clist_set_sort_column (clist, column);

  gtk_clist_sort (clist);
}
#endif

static void
dialog_response (GtkWidget *widget,
                 gint       response_id,
                 PDesc     *pdesc)
{
  const gchar *search_text = NULL;
  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      if (widget != NULL)
        {
          /* The result of a button press... read entry data */
          search_text =
            gtk_entry_get_text (GTK_ENTRY (plugindesc->search_entry));
        }

        get_plugin_info (pdesc, search_text);
      break;

    default:
      gtk_widget_destroy (pdesc->dlg);
      gtk_main_quit ();
      break;
    }
}

static GtkWidget *
gimp_plugin_desc (void)
{
  GtkWidget         *button;
  GtkWidget         *hbox, *searchhbox, *vbox;
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

  plugindesc = g_new0 (PDesc, 1);

  /* the dialog box */
  plugindesc->dlg =
    gimp_dialog_new (_("Plugin Descriptions"), "plugindetails",
                     NULL, 0,
                     gimp_standard_help_func, "plug-in-plug-in-details",

                     GTK_STOCK_CLOSE,     GTK_RESPONSE_CLOSE,
                     _("Search by Name"), GTK_RESPONSE_OK,

                     NULL);

  plugindesc->details_showing = FALSE;

  g_signal_connect (plugindesc->dlg, "response",
                    G_CALLBACK (dialog_response),
                    plugindesc);

  /* hbox : left=notebook ; right=description */

  plugindesc->paned = hbox = gtk_hpaned_new ();
  gtk_container_set_border_width (GTK_CONTAINER (plugindesc->paned), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (plugindesc->dlg)->vbox),
                      hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /* left = vbox : the list and the search entry */

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_paned_pack1 (GTK_PANED (hbox), vbox, FALSE, FALSE);
  gtk_widget_show (vbox);

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

  plugindesc->list_view = GTK_TREE_VIEW (list_view);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                     renderer,
                                                     "text",
                                                     LIST_NAME_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Ins date"),
                                                     renderer,
                                                     "text",
                                                     LIST_DATE_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Menu path"),
                                                     renderer,
                                                     "text",
                                                     LIST_PATH_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Image types"),
                                                     renderer,
                                                     "text",
                                                     LIST_IMAGE_TYPES_COLUMN,
                                                     NULL);
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
                    G_CALLBACK (list_store_select_callback),
                    plugindesc);

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

  plugindesc->tree_view = GTK_TREE_VIEW (tree_view);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Menu path/name"),
                                                     renderer,
                                                     "text",
                                                     TREE_PATH_NAME_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Ins date"),
                                                     renderer,
                                                     "text",
                                                     TREE_DATE_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Image types"),
                                                     renderer,
                                                     "text",
                                                     TREE_IMAGE_TYPES_COLUMN,
                                                     NULL);
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
                    G_CALLBACK (tree_store_select_callback),
                    plugindesc);

  label = gtk_label_new (_("Tree view"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), swindow, label);
  gtk_container_add (GTK_CONTAINER (swindow), tree_view);

  gtk_widget_show (tree_view);
  gtk_widget_show (swindow);
  gtk_widget_show (notebook);

  /* search entry & details button */

  searchhbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), searchhbox, FALSE, FALSE, 0);
  gtk_widget_show (searchhbox);

  label = gtk_label_new (_("Search:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (searchhbox),
                      label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  plugindesc->search_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (searchhbox),
                      plugindesc->search_entry, TRUE, TRUE, 0);
  gtk_widget_show (plugindesc->search_entry);

  button = gtk_button_new_with_label (_("Details >>"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (details_callback),
                    plugindesc);
  gtk_box_pack_start (GTK_BOX (searchhbox), button,
                      FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* right = description */
  /* the right description is build on first click of the Details button */

  /* now build the list */
  dialog_response (NULL, GTK_RESPONSE_OK, plugindesc);

  gtk_widget_show (plugindesc->dlg);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
   gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view)),
                                   &iter);

  GTK_PANED (plugindesc->paned)->child1_resize = TRUE;

  return plugindesc->dlg;
}
