/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * module-browser.c
 * (C) 1999 Austin Donnelly <austin@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimp-modules.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "module-browser.h"

#include "gimp-intl.h"


#define MODULES_RESPONSE_REFRESH 1
#define NUM_INFO_LINES           9

enum
{
  PATH_COLUMN,
  AUTO_COLUMN,
  MODULE_COLUMN,
  NUM_COLUMNS
};

typedef struct _ModuleBrowser ModuleBrowser;

struct _ModuleBrowser
{
  Gimp         *gimp;

  GimpModule   *last_update;
  GtkListStore *list;

  GtkWidget    *table;
  GtkWidget    *label[NUM_INFO_LINES];
  GtkWidget    *button;
  GtkWidget    *button_label;
};


/*  local function prototypes  */

static void   browser_response              (GtkWidget             *widget,
                                             gint                   response_id,
                                             ModuleBrowser         *browser);
static void   browser_destroy_callback      (GtkWidget             *widget,
                                             ModuleBrowser         *browser);
static void   browser_select_callback       (GtkTreeSelection      *sel,
                                             ModuleBrowser         *browser);
static void   browser_autoload_toggled      (GtkCellRendererToggle *celltoggle,
                                             gchar                 *path_string,
                                             ModuleBrowser         *browser);
static void   browser_load_unload_callback  (GtkWidget             *widget,
                                             ModuleBrowser         *browser);
static void   make_list_item                (gpointer               data,
                                             gpointer               user_data);
static void   browser_info_add              (GimpModuleDB          *db,
                                             GimpModule            *module,
                                             ModuleBrowser         *browser);
static void   browser_info_remove           (GimpModuleDB          *db,
                                             GimpModule            *module,
                                             ModuleBrowser         *browser);
static void   browser_info_update           (GimpModuleDB          *db,
                                             GimpModule            *module,
                                             ModuleBrowser         *browser);
static void   browser_info_init             (ModuleBrowser         *browser,
                                             GtkWidget             *table);


/*  public functions  */

GtkWidget *
module_browser_new (Gimp *gimp)
{
  GtkWidget         *shell;
  GtkWidget         *hbox;
  GtkWidget         *vbox;
  GtkWidget         *listbox;
  GtkWidget         *tv;
  ModuleBrowser     *browser;
  GtkTreeSelection  *sel;
  GtkTreeIter        iter;
  GtkTreeViewColumn *col;
  GtkCellRenderer   *rend;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  browser = g_new0 (ModuleBrowser, 1);

  browser->gimp = gimp;

  shell = gimp_viewable_dialog_new (NULL,
                                    _("Module Manager"), "gimp-modules",
                                    GTK_STOCK_EXECUTE,
                                    _("Manage Loadable Modules"),
                                    NULL,
                                    gimp_standard_help_func,
                                    GIMP_HELP_MODULE_DIALOG,

                                    GTK_STOCK_REFRESH, MODULES_RESPONSE_REFRESH,
                                    GTK_STOCK_CLOSE,   GTK_STOCK_CLOSE,

                                    NULL);

  g_signal_connect (shell, "response",
                    G_CALLBACK (browser_response),
                    browser);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell)->vbox), vbox);
  gtk_widget_show (vbox);

  listbox = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (listbox),
				       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (listbox),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 0);
  gtk_widget_set_size_request (listbox, 125, 100);
  gtk_widget_show (listbox);

  browser->list = gtk_list_store_new (NUM_COLUMNS,
                                      G_TYPE_STRING, G_TYPE_BOOLEAN,
                                      G_TYPE_POINTER);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (browser->list));
  g_object_unref (browser->list);

  g_list_foreach (gimp->module_db->modules, make_list_item, browser);

  rend = gtk_cell_renderer_toggle_new ();

  g_signal_connect (rend, "toggled",
                    G_CALLBACK (browser_autoload_toggled),
                    browser);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (col, _("Autoload"));
  gtk_tree_view_column_pack_start (col, rend, FALSE);
  gtk_tree_view_column_add_attribute (col, rend, "active", AUTO_COLUMN);

  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv), 1,
                                               _("Module Path"),
                                               gtk_cell_renderer_text_new (),
                                               "text", PATH_COLUMN,
                                               NULL);

  gtk_container_add (GTK_CONTAINER (listbox), tv);
  gtk_widget_show (tv);

  browser->table = gtk_table_new (2, NUM_INFO_LINES, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (browser->table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), browser->table, FALSE, FALSE, 0);
  gtk_widget_show (browser->table);

  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_SPREAD);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);


  browser->button = gtk_button_new_with_label ("");
  browser->button_label = gtk_bin_get_child (GTK_BIN (browser->button));
  gtk_box_pack_start (GTK_BOX (hbox), browser->button, TRUE, TRUE, 0);
  gtk_widget_show (browser->button);

  g_signal_connect (browser->button, "clicked",
                    G_CALLBACK (browser_load_unload_callback),
                    browser);

  browser_info_init (browser, browser->table);

  browser_info_update (gimp->module_db, browser->last_update, browser);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));

  g_signal_connect (sel, "changed",
                    G_CALLBACK (browser_select_callback),
                    browser);

  if (gtk_tree_model_get_iter_root (GTK_TREE_MODEL (browser->list), &iter))
    gtk_tree_selection_select_iter (sel, &iter);

  /* hook the GimpModuleDB signals so we can refresh the display
   * appropriately.
   */
  g_signal_connect (gimp->module_db, "add",
                    G_CALLBACK (browser_info_add),
                    browser);
  g_signal_connect (gimp->module_db, "remove",
                    G_CALLBACK (browser_info_remove),
                    browser);
  g_signal_connect (gimp->module_db, "module_modified",
                    G_CALLBACK (browser_info_update),
                    browser);

  g_signal_connect (shell, "destroy",
                    G_CALLBACK (browser_destroy_callback),
                    browser);

  return shell;
}


/*  private functions  */

static void
browser_response (GtkWidget     *widget,
                  gint           response_id,
                  ModuleBrowser *browser)
{
  if (response_id == MODULES_RESPONSE_REFRESH)
    gimp_modules_refresh (browser->gimp);
  else
    gtk_widget_destroy (widget);
}

static void
browser_destroy_callback (GtkWidget     *widget,
			  ModuleBrowser *browser)
{
  g_signal_handlers_disconnect_by_func (browser->gimp->module_db,
                                        browser_info_add,
                                        browser);
  g_signal_handlers_disconnect_by_func (browser->gimp->module_db,
                                        browser_info_remove,
                                        browser);
  g_signal_handlers_disconnect_by_func (browser->gimp->module_db,
                                        browser_info_update,
                                        browser);

  g_free (browser);
}

static void
browser_select_callback (GtkTreeSelection *sel,
			 ModuleBrowser    *browser)
{
  GimpModule  *module;
  GtkTreeIter  iter;

  gtk_tree_selection_get_selected (sel, NULL, &iter);
  gtk_tree_model_get (GTK_TREE_MODEL (browser->list), &iter,
                      MODULE_COLUMN, &module, -1);

  if (browser->last_update == module)
    return;

  browser->last_update = module;

  browser_info_update (browser->gimp->module_db, browser->last_update, browser);
}

static void
browser_autoload_toggled (GtkCellRendererToggle *celltoggle,
                          gchar                 *path_string,
                          ModuleBrowser         *browser)
{
  GtkTreePath *path;
  GtkTreeIter  iter;
  gboolean     active = FALSE;
  GimpModule  *module = NULL;

  path = gtk_tree_path_new_from_string (path_string);
  if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (browser->list), &iter, path))
    {
      g_warning ("%s: bad tree path?", G_STRLOC);
      return;
    }
  gtk_tree_path_free (path);

  gtk_tree_model_get (GTK_TREE_MODEL (browser->list), &iter,
                      AUTO_COLUMN,   &active,
                      MODULE_COLUMN, &module,
                      -1);

  if (module)
    {
      gimp_module_set_load_inhibit (module, active);

      browser->gimp->write_modulerc = TRUE;

      gtk_list_store_set (GTK_LIST_STORE (browser->list), &iter,
                          AUTO_COLUMN, ! active,
                          -1);
    }
}

static void
browser_load_unload_callback (GtkWidget     *widget,
			      ModuleBrowser *browser)
{
  if (browser->last_update->state != GIMP_MODULE_STATE_LOADED)
    {
      if (browser->last_update->info)
        {
          if (g_type_module_use (G_TYPE_MODULE (browser->last_update)))
            g_type_module_unuse (G_TYPE_MODULE (browser->last_update));
        }
      else
        {
          gimp_module_query_module (browser->last_update);
        }
    }

  gimp_module_modified (browser->last_update);
}

static void
make_list_item (gpointer data,
		gpointer user_data)
{
  GimpModule    *module  = data;
  ModuleBrowser *browser = user_data;
  GtkTreeIter    iter;

  if (! browser->last_update)
    browser->last_update = module;

  gtk_list_store_append (browser->list, &iter);
  gtk_list_store_set (browser->list, &iter,
		      PATH_COLUMN, module->filename,
                      AUTO_COLUMN, ! module->load_inhibit,
		      MODULE_COLUMN, module,
		      -1);
}

static void
browser_info_add (GimpModuleDB *db,
		  GimpModule   *module,
		  ModuleBrowser*browser)
{
  make_list_item (module, browser);
}

static void
browser_info_remove (GimpModuleDB  *db,
		     GimpModule    *mod,
		     ModuleBrowser *browser)
{
  GtkTreeIter  iter;
  GimpModule  *module;

  /* FIXME: Use gtk_list_store_foreach_remove when it becomes available */

  if (! gtk_tree_model_get_iter_root (GTK_TREE_MODEL (browser->list), &iter))
    return;

  do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (browser->list), &iter,
			  MODULE_COLUMN, &module,
			  -1);

      if (module == mod)
	{
	  gtk_list_store_remove (browser->list, &iter);
	  return;
	}
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (browser->list), &iter));

  g_warning ("%s: Tried to remove a module not in the browser's list.",
	     G_STRLOC);
}

static void
browser_info_update (GimpModuleDB  *db,
                     GimpModule    *module,
		     ModuleBrowser *browser)
{
  GTypeModule *g_type_module;
  const gchar *text[NUM_INFO_LINES];
  gint         i;

  g_type_module = G_TYPE_MODULE (module);

  /* only update the info if we're actually showing it */
  if (module != browser->last_update)
    return;

  if (! module)
    {
      for (i = 0; i < NUM_INFO_LINES; i++)
	gtk_label_set_text (GTK_LABEL (browser->label[i]), "");
      gtk_label_set_text (GTK_LABEL (browser->button_label), _("<No modules>"));
      gtk_widget_set_sensitive (GTK_WIDGET (browser->button), FALSE);
      return;
    }

  if (module->info)
    {
      text[0] = module->info->purpose;
      text[1] = module->info->author;
      text[2] = module->info->version;
      text[3] = module->info->copyright;
      text[4] = module->info->date;
      text[5] = module->on_disk ? _("On disk") : _("Only in memory");
    }
  else
    {
      text[0] = "--";
      text[1] = "--";
      text[2] = "--";
      text[3] = "--";
      text[4] = "--";
      text[5] = module->on_disk ? _("On disk") : _("No longer available");
    }

  text[6] = gimp_module_state_name (module->state);

  if (module->state == GIMP_MODULE_STATE_ERROR && module->last_module_error)
    text[7] = module->last_module_error;
  else
    text[7] = "--";

  if (g_type_module->type_infos || g_type_module->interface_infos)
    {
      gchar *str;

      str = g_strdup_printf ("%d Types, %d Interfaces",
                             g_slist_length (g_type_module->type_infos),
                             g_slist_length (g_type_module->interface_infos));
      gtk_label_set_text (GTK_LABEL (browser->label[NUM_INFO_LINES - 1]), str);
      g_free (str);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (browser->label[NUM_INFO_LINES - 1]),
                          "---");
    }

  for (i = 0; i < NUM_INFO_LINES - 1; i++)
    gtk_label_set_text (GTK_LABEL (browser->label[i]), gettext (text[i]));

  /* work out what the button should do (if anything) */
  switch (module->state)
    {
    case GIMP_MODULE_STATE_ERROR:
    case GIMP_MODULE_STATE_LOAD_FAILED:
    case GIMP_MODULE_STATE_NOT_LOADED:
      if (module->info)
        gtk_label_set_text (GTK_LABEL (browser->button_label), _("Load"));
      else
        gtk_label_set_text (GTK_LABEL (browser->button_label), _("Query"));

      gtk_widget_set_sensitive (GTK_WIDGET (browser->button),
                                module->on_disk);
      break;

    case GIMP_MODULE_STATE_LOADED:
      gtk_label_set_text (GTK_LABEL (browser->button_label), _("Unload"));
      gtk_widget_set_sensitive (GTK_WIDGET (browser->button), FALSE);
      break;
    }
}

static void
browser_info_init (ModuleBrowser *browser,
		   GtkWidget     *table)
{
  GtkWidget *label;
  gint       i;

  static const gchar * const text[] =
  {
    N_("Purpose:"),
    N_("Author:"),
    N_("Version:"),
    N_("Copyright:"),
    N_("Date:"),
    N_("Location:"),
    N_("State:"),
    N_("Last Error:"),
    N_("Available Types:")
  };

  for (i = 0; i < G_N_ELEMENTS (text); i++)
    {
      label = gtk_label_new (gettext (text[i]));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, i, i + 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
      gtk_widget_show (label);

      browser->label[i] = gtk_label_new ("");
      gtk_misc_set_alignment (GTK_MISC (browser->label[i]), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (browser->table), browser->label[i],
                        1, 2, i, i + 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
      gtk_widget_show (browser->label[i]);
    }
}
