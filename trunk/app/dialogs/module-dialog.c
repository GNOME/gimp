/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * module-dialog.c
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

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimp-modules.h"

#include "widgets/gimphelp-ids.h"

#include "module-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_REFRESH  1
#define NUM_INFO_LINES    9

enum
{
  PATH_COLUMN,
  AUTO_COLUMN,
  MODULE_COLUMN,
  NUM_COLUMNS
};

typedef struct
{
  Gimp         *gimp;

  GimpModule   *last_update;
  GtkListStore *list;

  GtkWidget    *table;
  GtkWidget    *label[NUM_INFO_LINES];
  GtkWidget    *button;
  GtkWidget    *button_label;
} ModuleDialog;


/*  local function prototypes  */

static void   dialog_response              (GtkWidget             *widget,
                                            gint                   response_id,
                                            ModuleDialog          *dialog);
static void   dialog_destroy_callback      (GtkWidget             *widget,
                                            ModuleDialog          *dialog);
static void   dialog_select_callback       (GtkTreeSelection      *sel,
                                            ModuleDialog          *dialog);
static void   dialog_autoload_toggled      (GtkCellRendererToggle *celltoggle,
                                            gchar                 *path_string,
                                            ModuleDialog          *dialog);
static void   dialog_load_unload_callback  (GtkWidget             *widget,
                                            ModuleDialog          *dialog);
static void   make_list_item               (gpointer               data,
                                            gpointer               user_data);
static void   dialog_info_add              (GimpModuleDB          *db,
                                            GimpModule            *module,
                                            ModuleDialog          *dialog);
static void   dialog_info_remove           (GimpModuleDB          *db,
                                            GimpModule            *module,
                                            ModuleDialog          *dialog);
static void   dialog_info_update           (GimpModuleDB          *db,
                                            GimpModule            *module,
                                            ModuleDialog          *dialog);
static void   dialog_info_init             (ModuleDialog          *dialog,
                                            GtkWidget             *table);


/*  public functions  */

GtkWidget *
module_dialog_new (Gimp *gimp)
{
  GtkWidget         *shell;
  GtkWidget         *hbox;
  GtkWidget         *vbox;
  GtkWidget         *listbox;
  GtkWidget         *tv;
  ModuleDialog      *dialog;
  GtkTreeSelection  *sel;
  GtkTreeIter        iter;
  GtkTreeViewColumn *col;
  GtkCellRenderer   *rend;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  dialog = g_slice_new0 (ModuleDialog);

  dialog->gimp = gimp;

  shell = gimp_dialog_new (_("Manage Loadable Modules"),
                           "gimp-modules", NULL, 0,
                           gimp_standard_help_func, GIMP_HELP_MODULE_DIALOG,

                           GTK_STOCK_REFRESH, RESPONSE_REFRESH,
                           GTK_STOCK_CLOSE,   GTK_STOCK_CLOSE,

                           NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (shell),
                                           GTK_RESPONSE_CLOSE,
                                           RESPONSE_REFRESH,
                                           -1);

  g_signal_connect (shell, "response",
                    G_CALLBACK (dialog_response),
                    dialog);

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

  dialog->list = gtk_list_store_new (NUM_COLUMNS,
                                     G_TYPE_STRING,
                                     G_TYPE_BOOLEAN,
                                     GIMP_TYPE_MODULE);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (dialog->list));
  g_object_unref (dialog->list);

  g_list_foreach (gimp->module_db->modules, make_list_item, dialog);

  rend = gtk_cell_renderer_toggle_new ();

  g_signal_connect (rend, "toggled",
                    G_CALLBACK (dialog_autoload_toggled),
                    dialog);

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

  dialog->table = gtk_table_new (2, NUM_INFO_LINES, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (dialog->table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->table, FALSE, FALSE, 0);
  gtk_widget_show (dialog->table);

  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_END);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  dialog->button = gtk_button_new_with_label ("");
  dialog->button_label = gtk_bin_get_child (GTK_BIN (dialog->button));
  gtk_container_add (GTK_CONTAINER (hbox), dialog->button);
  gtk_widget_show (dialog->button);

  g_signal_connect (dialog->button, "clicked",
                    G_CALLBACK (dialog_load_unload_callback),
                    dialog);

  dialog_info_init (dialog, dialog->table);

  dialog_info_update (gimp->module_db, dialog->last_update, dialog);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));

  g_signal_connect (sel, "changed",
                    G_CALLBACK (dialog_select_callback),
                    dialog);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->list), &iter))
    gtk_tree_selection_select_iter (sel, &iter);

  /* hook the GimpModuleDB signals so we can refresh the display
   * appropriately.
   */
  g_signal_connect (gimp->module_db, "add",
                    G_CALLBACK (dialog_info_add),
                    dialog);
  g_signal_connect (gimp->module_db, "remove",
                    G_CALLBACK (dialog_info_remove),
                    dialog);
  g_signal_connect (gimp->module_db, "module-modified",
                    G_CALLBACK (dialog_info_update),
                    dialog);

  g_signal_connect (shell, "destroy",
                    G_CALLBACK (dialog_destroy_callback),
                    dialog);

  return shell;
}


/*  private functions  */

static void
dialog_response (GtkWidget    *widget,
                 gint          response_id,
                 ModuleDialog *dialog)
{
  if (response_id == RESPONSE_REFRESH)
    gimp_modules_refresh (dialog->gimp);
  else
    gtk_widget_destroy (widget);
}

static void
dialog_destroy_callback (GtkWidget    *widget,
                         ModuleDialog *dialog)
{
  g_signal_handlers_disconnect_by_func (dialog->gimp->module_db,
                                        dialog_info_add,
                                        dialog);
  g_signal_handlers_disconnect_by_func (dialog->gimp->module_db,
                                        dialog_info_remove,
                                        dialog);
  g_signal_handlers_disconnect_by_func (dialog->gimp->module_db,
                                        dialog_info_update,
                                        dialog);

  g_slice_free (ModuleDialog, dialog);
}

static void
dialog_select_callback (GtkTreeSelection *sel,
                        ModuleDialog     *dialog)
{
  GimpModule  *module;
  GtkTreeIter  iter;

  gtk_tree_selection_get_selected (sel, NULL, &iter);
  gtk_tree_model_get (GTK_TREE_MODEL (dialog->list), &iter,
                      MODULE_COLUMN, &module, -1);

  if (module)
    g_object_unref (module);

  if (dialog->last_update == module)
    return;

  dialog->last_update = module;

  dialog_info_update (dialog->gimp->module_db, dialog->last_update, dialog);
}

static void
dialog_autoload_toggled (GtkCellRendererToggle *celltoggle,
                         gchar                 *path_string,
                         ModuleDialog          *dialog)
{
  GtkTreePath *path;
  GtkTreeIter  iter;
  gboolean     active = FALSE;
  GimpModule  *module = NULL;

  path = gtk_tree_path_new_from_string (path_string);
  if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (dialog->list), &iter, path))
    {
      g_warning ("%s: bad tree path?", G_STRFUNC);
      return;
    }
  gtk_tree_path_free (path);

  gtk_tree_model_get (GTK_TREE_MODEL (dialog->list), &iter,
                      AUTO_COLUMN,   &active,
                      MODULE_COLUMN, &module,
                      -1);

  if (module)
    {
      g_object_unref (module);

      gimp_module_set_load_inhibit (module, active);

      dialog->gimp->write_modulerc = TRUE;

      gtk_list_store_set (GTK_LIST_STORE (dialog->list), &iter,
                          AUTO_COLUMN, ! active,
                          -1);
    }
}

static void
dialog_load_unload_callback (GtkWidget    *widget,
                             ModuleDialog *dialog)
{
  if (dialog->last_update->state != GIMP_MODULE_STATE_LOADED)
    {
      if (dialog->last_update->info)
        {
          if (g_type_module_use (G_TYPE_MODULE (dialog->last_update)))
            g_type_module_unuse (G_TYPE_MODULE (dialog->last_update));
        }
      else
        {
          gimp_module_query_module (dialog->last_update);
        }
    }

  gimp_module_modified (dialog->last_update);
}

static void
make_list_item (gpointer data,
                gpointer user_data)
{
  GimpModule   *module  = data;
  ModuleDialog *dialog = user_data;
  GtkTreeIter   iter;

  if (! dialog->last_update)
    dialog->last_update = module;

  gtk_list_store_append (dialog->list, &iter);
  gtk_list_store_set (dialog->list, &iter,
                      PATH_COLUMN, module->filename,
                      AUTO_COLUMN, ! module->load_inhibit,
                      MODULE_COLUMN, module,
                      -1);
}

static void
dialog_info_add (GimpModuleDB *db,
                 GimpModule   *module,
                 ModuleDialog *dialog)
{
  make_list_item (module, dialog);
}

static void
dialog_info_remove (GimpModuleDB *db,
                    GimpModule   *mod,
                    ModuleDialog *dialog)
{
  GtkTreeIter  iter;
  GimpModule  *module;

  /* FIXME: Use gtk_list_store_foreach_remove when it becomes available */

  if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->list), &iter))
    return;

  do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dialog->list), &iter,
                          MODULE_COLUMN, &module,
                          -1);

      if (module)
        g_object_unref (module);

      if (module == mod)
        {
          gtk_list_store_remove (dialog->list, &iter);
          return;
        }
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->list), &iter));

  g_warning ("%s: Tried to remove a module not in the dialog's list.",
             G_STRFUNC);
}

static void
dialog_info_update (GimpModuleDB *db,
                    GimpModule   *module,
                    ModuleDialog *dialog)
{
  GTypeModule *g_type_module;
  const gchar *text[NUM_INFO_LINES];
  gint         i;

  g_type_module = G_TYPE_MODULE (module);

  /* only update the info if we're actually showing it */
  if (module != dialog->last_update)
    return;

  if (! module)
    {
      for (i = 0; i < NUM_INFO_LINES; i++)
        gtk_label_set_text (GTK_LABEL (dialog->label[i]), "");
      gtk_label_set_text (GTK_LABEL (dialog->button_label), _("<No modules>"));
      gtk_widget_set_sensitive (GTK_WIDGET (dialog->button), FALSE);
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
      gtk_label_set_text (GTK_LABEL (dialog->label[NUM_INFO_LINES - 1]), str);
      g_free (str);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (dialog->label[NUM_INFO_LINES - 1]),
                          "---");
    }

  for (i = 0; i < NUM_INFO_LINES - 1; i++)
    gtk_label_set_text (GTK_LABEL (dialog->label[i]), gettext (text[i]));

  /* work out what the button should do (if anything) */
  switch (module->state)
    {
    case GIMP_MODULE_STATE_ERROR:
    case GIMP_MODULE_STATE_LOAD_FAILED:
    case GIMP_MODULE_STATE_NOT_LOADED:
      if (module->info)
        gtk_label_set_text (GTK_LABEL (dialog->button_label), _("Load"));
      else
        gtk_label_set_text (GTK_LABEL (dialog->button_label), _("Query"));

      gtk_widget_set_sensitive (GTK_WIDGET (dialog->button),
                                module->on_disk);
      break;

    case GIMP_MODULE_STATE_LOADED:
      gtk_label_set_text (GTK_LABEL (dialog->button_label), _("Unload"));
      gtk_widget_set_sensitive (GTK_WIDGET (dialog->button), FALSE);
      break;
    }
}

static void
dialog_info_init (ModuleDialog *dialog,
                  GtkWidget    *table)
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
    N_("Last error:"),
    N_("Available types:")
  };

  for (i = 0; i < G_N_ELEMENTS (text); i++)
    {
      label = gtk_label_new (gettext (text[i]));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, i, i + 1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
      gtk_widget_show (label);

      dialog->label[i] = gtk_label_new ("");
      gtk_misc_set_alignment (GTK_MISC (dialog->label[i]), 0.0, 0.5);
      gtk_label_set_ellipsize (GTK_LABEL (dialog->label[i]),
                               PANGO_ELLIPSIZE_END);
      gtk_table_attach (GTK_TABLE (dialog->table), dialog->label[i],
                        1, 2, i, i + 1,
                        GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
      gtk_widget_show (dialog->label[i]);
    }
}
