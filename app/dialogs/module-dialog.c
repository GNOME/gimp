/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * module-dialog.c
 * (C) 1999 Austin Donnelly <austin@gimp.org>
 * (C) 2008 Sven Neumann <sven@gimp.org>
 *
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
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

enum
{
  COLUMN_NAME,
  COLUMN_ENABLED,
  COLUMN_MODULE,
  N_COLUMNS
};

enum
{
  INFO_AUTHOR,
  INFO_VERSION,
  INFO_DATE,
  INFO_COPYRIGHT,
  INFO_LOCATION,
  N_INFOS
};

typedef struct
{
  Gimp         *gimp;

  GimpModule   *selected;
  GtkListStore *list;

  GtkWidget    *hint;
  GtkWidget    *table;
  GtkWidget    *label[N_INFOS];
  GtkWidget    *error_box;
  GtkWidget    *error_label;
} ModuleDialog;


/*  local function prototypes  */

static void   dialog_response         (GtkWidget             *widget,
                                       gint                   response_id,
                                       ModuleDialog          *dialog);
static void   dialog_destroy_callback (GtkWidget             *widget,
                                       ModuleDialog          *dialog);
static void   dialog_select_callback  (GtkTreeSelection      *sel,
                                       ModuleDialog          *dialog);
static void   dialog_enabled_toggled  (GtkCellRendererToggle *celltoggle,
                                       const gchar           *path_string,
                                       ModuleDialog          *dialog);
static void   make_list_item          (gpointer               data,
                                       gpointer               user_data);
static void   dialog_info_add         (GimpModuleDB          *db,
                                       GimpModule            *module,
                                       ModuleDialog          *dialog);
static void   dialog_info_remove      (GimpModuleDB          *db,
                                       GimpModule            *module,
                                       ModuleDialog          *dialog);
static void   dialog_info_update      (GimpModuleDB          *db,
                                       GimpModule            *module,
                                       ModuleDialog          *dialog);
static void   dialog_info_init        (ModuleDialog          *dialog,
                                       GtkWidget             *table);


/*  public functions  */

GtkWidget *
module_dialog_new (Gimp *gimp)
{
  GtkWidget         *shell;
  GtkWidget         *vbox;
  GtkWidget         *sw;
  GtkWidget         *view;
  GtkWidget         *image;
  ModuleDialog      *dialog;
  GtkTreeSelection  *sel;
  GtkTreeIter        iter;
  GtkTreeViewColumn *col;
  GtkCellRenderer   *rend;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  dialog = g_slice_new0 (ModuleDialog);

  dialog->gimp = gimp;

  shell = gimp_dialog_new (_("Module Manager"),
                           "gimp-modules", NULL, 0,
                           gimp_standard_help_func, GIMP_HELP_MODULE_DIALOG,

                           GTK_STOCK_REFRESH, RESPONSE_REFRESH,
                           GTK_STOCK_CLOSE,   GTK_RESPONSE_CLOSE,

                           NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (shell),
                                           GTK_RESPONSE_CLOSE,
                                           RESPONSE_REFRESH,
                                           -1);

  g_signal_connect (shell, "response",
                    G_CALLBACK (dialog_response),
                    dialog);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (shell))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  dialog->hint = gimp_hint_box_new (_("You will have to restart GIMP "
                                      "for the changes to take effect."));
  gtk_box_pack_start (GTK_BOX (vbox), dialog->hint, FALSE, FALSE, 0);

  if (gimp->write_modulerc)
    gtk_widget_show (dialog->hint);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
  gtk_widget_set_size_request (sw, 124, 100);
  gtk_widget_show (sw);

  dialog->list = gtk_list_store_new (N_COLUMNS,
                                     G_TYPE_STRING,
                                     G_TYPE_BOOLEAN,
                                     GIMP_TYPE_MODULE);
  view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (dialog->list));
  g_object_unref (dialog->list);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

  g_list_foreach (gimp->module_db->modules, make_list_item, dialog);

  rend = gtk_cell_renderer_toggle_new ();

  g_signal_connect (rend, "toggled",
                    G_CALLBACK (dialog_enabled_toggled),
                    dialog);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (col, rend, FALSE);
  gtk_tree_view_column_add_attribute (col, rend, "active", COLUMN_ENABLED);

  gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), 1,
                                               _("Module"),
                                               gtk_cell_renderer_text_new (),
                                               "text", COLUMN_NAME,
                                               NULL);

  gtk_container_add (GTK_CONTAINER (sw), view);
  gtk_widget_show (view);

  dialog->table = gtk_table_new (2, N_INFOS, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (dialog->table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->table, FALSE, FALSE, 0);
  gtk_widget_show (dialog->table);

  dialog->error_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->error_box, FALSE, FALSE, 0);

  image = gtk_image_new_from_icon_name (GIMP_STOCK_WARNING, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (dialog->error_box), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  dialog->error_label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (dialog->error_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (dialog->error_box),
                      dialog->error_label, TRUE, TRUE, 0);
  gtk_widget_show (dialog->error_label);

  dialog_info_init (dialog, dialog->table);

  dialog_info_update (gimp->module_db, dialog->selected, dialog);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

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
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      GimpModule *module;

      gtk_tree_model_get (GTK_TREE_MODEL (dialog->list), &iter,
                          COLUMN_MODULE, &module, -1);

      if (module)
        g_object_unref (module);

      if (dialog->selected == module)
        return;

      dialog->selected = module;

      dialog_info_update (dialog->gimp->module_db, dialog->selected, dialog);
    }
}

static void
dialog_enabled_toggled (GtkCellRendererToggle *celltoggle,
                        const gchar           *path_string,
                        ModuleDialog          *dialog)
{
  GtkTreePath *path;
  GtkTreeIter  iter;
  GimpModule  *module = NULL;

  path = gtk_tree_path_new_from_string (path_string);

  if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (dialog->list), &iter, path))
    {
      g_warning ("%s: bad tree path?", G_STRFUNC);
      return;
    }

  gtk_tree_path_free (path);

  gtk_tree_model_get (GTK_TREE_MODEL (dialog->list), &iter,
                      COLUMN_MODULE, &module,
                      -1);

  if (module)
    {
      gimp_module_set_load_inhibit (module, ! module->load_inhibit);
      g_object_unref (module);

      dialog->gimp->write_modulerc = TRUE;
      gtk_widget_show (dialog->hint);
   }
}

static void
dialog_list_item_update (ModuleDialog *dialog,
                         GtkTreeIter  *iter,
                         GimpModule   *module)
{
  gtk_list_store_set (dialog->list, iter,
                      COLUMN_NAME,   (module->info ?
                                      gettext (module->info->purpose) :
                                      gimp_filename_to_utf8 (module->filename)),
                      COLUMN_ENABLED, ! module->load_inhibit,
                      COLUMN_MODULE,  module,
                      -1);
}

static void
make_list_item (gpointer data,
                gpointer user_data)
{
  GimpModule   *module = data;
  ModuleDialog *dialog = user_data;
  GtkTreeIter   iter;

  if (! dialog->selected)
    dialog->selected = module;

  gtk_list_store_append (dialog->list, &iter);

  dialog_list_item_update (dialog, &iter, module);
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
                    GimpModule   *module,
                    ModuleDialog *dialog)
{
  GtkTreeIter  iter;

  /* FIXME: Use gtk_list_store_foreach_remove when it becomes available */

  if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->list), &iter))
    return;

  do
    {
      GimpModule  *this;

      gtk_tree_model_get (GTK_TREE_MODEL (dialog->list), &iter,
                          COLUMN_MODULE, &this,
                          -1);

      if (this)
        g_object_unref (this);

      if (this == module)
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
  GtkTreeModel *model           = GTK_TREE_MODEL (dialog->list);
  GtkTreeIter   iter;
  const gchar  *text[N_INFOS] = { NULL, };
  gchar        *location        = NULL;
  gboolean      iter_valid;
  gint          i;
  gboolean      show_error;

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      GimpModule *this;

      gtk_tree_model_get (model, &iter,
                          COLUMN_MODULE, &this,
                          -1);
      if (this)
        g_object_unref (this);

      if (this == module)
        break;
    }

  if (iter_valid)
    dialog_list_item_update (dialog, &iter, module);

  /* only update the info if we're actually showing it */
  if (module != dialog->selected)
    return;

  if (! module)
    {
      for (i = 0; i < N_INFOS; i++)
        gtk_label_set_text (GTK_LABEL (dialog->label[i]), NULL);

      gtk_label_set_text (GTK_LABEL (dialog->error_label), NULL);
      gtk_widget_hide (dialog->error_box);

      return;
    }

  if (module->on_disk)
    location = g_filename_display_name (module->filename);

  if (module->info)
    {
      text[INFO_AUTHOR]    = module->info->author;
      text[INFO_VERSION]   = module->info->version;
      text[INFO_DATE]      = module->info->date;
      text[INFO_COPYRIGHT] = module->info->copyright;
      text[INFO_LOCATION]  = module->on_disk ? location : _("Only in memory");
    }
  else
    {
      text[INFO_LOCATION]  = (module->on_disk ?
                              location : _("No longer available"));
    }

  for (i = 0; i < N_INFOS; i++)
    gtk_label_set_text (GTK_LABEL (dialog->label[i]),
                        text[i] ? text[i] : "--");
  g_free (location);

  /* Show errors */
  show_error = (module->state == GIMP_MODULE_STATE_ERROR &&
                module->last_module_error);
  gtk_label_set_text (GTK_LABEL (dialog->error_label),
                      show_error ? module->last_module_error : NULL);
  gtk_widget_set_visible (dialog->error_box, show_error);
}

static void
dialog_info_init (ModuleDialog *dialog,
                  GtkWidget    *table)
{
  GtkWidget *label;
  gint       i;

  const gchar * const text[] =
  {
    N_("Author:"),
    N_("Version:"),
    N_("Date:"),
    N_("Copyright:"),
    N_("Location:")
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
