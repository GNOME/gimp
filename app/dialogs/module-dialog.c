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
#include "core/gimpcontainer.h"
#include "core/gimpmodules.h"

#include "module-browser.h"

#include "libgimp/gimpintl.h"


#define NUM_INFO_LINES 8

enum
{
  PATH_COLUMN,
  MODULE_COLUMN,
  NUM_COLUMNS
};

typedef struct _ModuleBrowser ModuleBrowser;

struct _ModuleBrowser
{
  GtkWidget    *table;
  GtkWidget    *label[NUM_INFO_LINES];
  GtkWidget    *button_label;
  GimpModule   *last_update;
  GtkWidget    *button;
  GtkListStore *list;
  GtkWidget    *load_inhibit_check;

  GQuark        modules_handler_id;
  Gimp         *gimp;
};


/*  local function prototypes  */

static void   browser_popdown_callback      (GtkWidget        *widget,
                                             gpointer          data);
static void   browser_destroy_callback      (GtkWidget        *widget,
                                             ModuleBrowser    *browser);
static void   browser_load_inhibit_callback (GtkWidget        *widget,
                                             ModuleBrowser    *browser);
static void   browser_select_callback       (GtkTreeSelection *sel,
                                             ModuleBrowser    *browser);
static void   browser_load_unload_callback  (GtkWidget        *widget,
                                             ModuleBrowser    *browser);
static void   browser_refresh_callback      (GtkWidget        *widget,
                                             ModuleBrowser    *browser);
static void   make_list_item                (gpointer          data,
                                             gpointer          user_data);
static void   browser_info_add              (GimpContainer    *container,
                                             GimpModule       *module,
                                             ModuleBrowser    *browser);
static void   browser_info_remove           (GimpContainer    *container,
                                             GimpModule       *module,
                                             ModuleBrowser    *browser);
static void   browser_info_update           (GimpModule       *module,
                                             ModuleBrowser    *browser);
static void   browser_info_init             (ModuleBrowser    *browser,
                                             GtkWidget        *table);


/*  public functions  */

GtkWidget *
module_browser_new (Gimp *gimp)
{
  GtkWidget        *shell;
  GtkWidget        *hbox;
  GtkWidget        *vbox;
  GtkWidget        *listbox;
  GtkWidget        *button;
  GtkWidget        *tv;
  ModuleBrowser    *browser;
  GtkTreeSelection *sel;
  GtkTreeIter       iter;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  shell = gimp_dialog_new (_("Module DB"), "module_db_dialog",
			   gimp_standard_help_func,
			   "dialogs/module_browser.html",
			   GTK_WIN_POS_NONE,
			   FALSE, TRUE, FALSE,

			   GTK_STOCK_CLOSE, browser_popdown_callback,
			   NULL, NULL, NULL, TRUE, TRUE,

			   NULL);

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
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

  browser = g_new0 (ModuleBrowser, 1);

  browser->gimp = gimp;

  browser->list = gtk_list_store_new (NUM_COLUMNS,
                                      G_TYPE_STRING, G_TYPE_POINTER);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (browser->list));
  g_object_unref (browser->list);

  gimp_container_foreach (gimp->modules, make_list_item, browser);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv),
					       -1, NULL,
					       gtk_cell_renderer_text_new (),
					       "text", PATH_COLUMN,
					       NULL);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);

  gtk_container_add (GTK_CONTAINER (listbox), tv);
  gtk_widget_show (tv);

  browser->table = gtk_table_new (5, NUM_INFO_LINES + 1, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (browser->table), 4);  
  gtk_box_pack_start (GTK_BOX (vbox), browser->table, FALSE, FALSE, 0);
  gtk_widget_show (browser->table);

  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_SPREAD);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 5);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label (_("Refresh"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (browser_refresh_callback),
                    browser);

  browser->button = gtk_button_new_with_label ("");
  browser->button_label = gtk_bin_get_child (GTK_BIN (browser->button));
  gtk_box_pack_start (GTK_BOX (hbox), browser->button, TRUE, TRUE, 0);
  gtk_widget_show (browser->button);

  g_signal_connect (G_OBJECT (browser->button), "clicked",
                    G_CALLBACK (browser_load_unload_callback),
                    browser);

  browser_info_init (browser, browser->table);
  browser_info_update (browser->last_update, browser);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));

  g_signal_connect (G_OBJECT (sel), "changed",
                    G_CALLBACK (browser_select_callback),
                    browser);

  if (gtk_tree_model_get_iter_root (GTK_TREE_MODEL (browser->list), &iter))
    gtk_tree_selection_select_iter (sel, &iter);

  /* hook the GimpContainer signals so we can refresh the display
   * appropriately.
   */
  browser->modules_handler_id =
    gimp_container_add_handler (gimp->modules, "modified", 
                                G_CALLBACK (browser_info_update),
                                browser);

  g_signal_connect (G_OBJECT (gimp->modules), "add", 
                    G_CALLBACK (browser_info_add), 
                    browser);
  g_signal_connect (G_OBJECT (gimp->modules), "remove", 
                    G_CALLBACK (browser_info_remove), 
                    browser);

  g_signal_connect (G_OBJECT (shell), "destroy",
                    G_CALLBACK (browser_destroy_callback), 
                    browser);

  return shell;
}


/*  private functions  */

static void
browser_popdown_callback (GtkWidget *widget,
			  gpointer   data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
browser_destroy_callback (GtkWidget     *widget,
			  ModuleBrowser *browser)
{
  g_signal_handlers_disconnect_by_func (G_OBJECT (browser->gimp->modules),
                                        browser_info_add,
                                        browser);
  g_signal_handlers_disconnect_by_func (G_OBJECT (browser->gimp->modules), 
                                        browser_info_remove,
                                        browser);
  gimp_container_remove_handler (browser->gimp->modules,
                                 browser->modules_handler_id);
  g_free (browser);
}

static void
browser_load_inhibit_callback (GtkWidget     *widget,
			       ModuleBrowser *browser)
{
  gboolean new_value;

  g_return_if_fail (browser->last_update != NULL);

  new_value = ! GTK_TOGGLE_BUTTON (widget)->active;

  if (new_value == browser->last_update->load_inhibit)
    return;

  browser->last_update->load_inhibit = new_value;
  gimp_module_modified (browser->last_update);

  browser->gimp->write_modulerc = TRUE;
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

  browser_info_update (browser->last_update, browser);
}

static void
browser_load_unload_callback (GtkWidget     *widget, 
			      ModuleBrowser *browser)
{
  if (browser->last_update->state != GIMP_MODULE_STATE_LOADED_OK)
    {
      if (g_type_module_use (G_TYPE_MODULE (browser->last_update)))
        g_type_module_unuse (G_TYPE_MODULE (browser->last_update));
    }

  gimp_module_modified (browser->last_update);
}

static void
browser_refresh_callback (GtkWidget     *widget, 
			  ModuleBrowser *browser)
{
  gimp_modules_refresh (browser->gimp);
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
		      MODULE_COLUMN, module,
		      -1);
}

static void
browser_info_add (GimpContainer *container,
		  GimpModule    *module, 
		  ModuleBrowser *browser)
{
  make_list_item (module, browser);
}

static void
browser_info_remove (GimpContainer *container,
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
browser_info_update (GimpModule    *module, 
		     ModuleBrowser *browser)
{
  const gchar *text[NUM_INFO_LINES];
  gint         i;

  static const gchar * const statename[] =
  {
    N_("Module error"),
    N_("Loaded OK"),
    N_("Load failed"),
    N_("Unloaded OK")
  };

  /* only update the info if we're actually showing it */
  if (module != browser->last_update)
    return;

  if (! module)
    {
      for (i = 0; i < NUM_INFO_LINES; i++)
	gtk_label_set_text (GTK_LABEL (browser->label[i]), "");
      gtk_label_set_text (GTK_LABEL (browser->button_label), _("<No modules>"));
      gtk_widget_set_sensitive (GTK_WIDGET (browser->button), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (browser->load_inhibit_check), FALSE);
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

  text[6] = gettext (statename[module->state]);

  if (module->state == GIMP_MODULE_STATE_ERROR && module->last_module_error)
    text[7] = module->last_module_error;
  else
    text[7] = "--";

  for (i = 0; i < NUM_INFO_LINES; i++)
    gtk_label_set_text (GTK_LABEL (browser->label[i]), gettext (text[i]));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (browser->load_inhibit_check),
                                ! module->load_inhibit);
  gtk_widget_set_sensitive (GTK_WIDGET (browser->load_inhibit_check), TRUE);

  /* work out what the button should do (if anything) */
  switch (module->state)
    {
    case GIMP_MODULE_STATE_ERROR:
    case GIMP_MODULE_STATE_LOAD_FAILED:
    case GIMP_MODULE_STATE_UNLOADED_OK:
      gtk_label_set_text (GTK_LABEL (browser->button_label), _("Load"));
      gtk_widget_set_sensitive (GTK_WIDGET (browser->button), module->on_disk);
      break;

    case GIMP_MODULE_STATE_LOADED_OK:
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

  static const gchar *text[] =
  {
    N_("Purpose:"),
    N_("Author:"),
    N_("Version:"),
    N_("Copyright:"),
    N_("Date:"),
    N_("Location:"),
    N_("State:"),
    N_("Last Error:")
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

  browser->load_inhibit_check =
    gtk_check_button_new_with_label (_("Autoload during start-up"));
  gtk_table_attach (GTK_TABLE (table), browser->load_inhibit_check,
		    0, 2, i, i + 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  gtk_widget_show (browser->load_inhibit_check);

  g_signal_connect (G_OBJECT (browser->load_inhibit_check), "toggled",
                    G_CALLBACK (browser_load_inhibit_callback),
                    browser);
}
