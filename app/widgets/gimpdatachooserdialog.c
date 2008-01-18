/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatachooserdialog.c
 * Copyright (C) 2008 Bill Skaggs <weskaggs@primate.ucdavis.edu>
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

#include <sys/stat.h>
#include <string.h>

#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include <libgimpbase/gimpbase.h>
#include <libgimpwidgets/gimpwidgets.h>
#include <libgimpconfig/gimpconfig.h>

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdata.h"
#include "core/gimpdatafactory.h"

#include "config/gimpguiconfig.h"

#include "file/file-utils.h"

#include "dialogs/dialogs.h"

#include "gimpcontainereditor.h"
#include "gimpdatafactoryview.h"
#include "gimpdatachooserdialog.h"
#include "gimpdialogfactory.h"
#include "gimpdocked.h"
#include "gimpeditor.h"
#include "gimphelp-ids.h"

#include "gimp-intl.h"

static void        gimp_data_chooser_dialog_init          (GimpDataChooserDialog *dialog);

static gchar *     gimp_data_chooser_dialog_get_filename  (GimpDataChooserDialog *dialog);

static GtkWidget * gimp_data_chooser_dialog_path_view     (GimpDataChooserDialog *dialog,
                                                           GList                 *dirlist);

static void        path_view_selection_changed            (GtkTreeSelection      *selection,
                                                           gpointer               data);

static gboolean    gimp_data_chooser_dialog_activate_item (GtkWidget             *widget,
                                                           GimpData              *data,
                                                           gpointer               insert_data,
                                                           GimpDataChooserDialog *dialog);

static gboolean    gimp_data_chooser_dialog_select_item   (GtkWidget             *widget,
                                                           GimpData              *data,
                                                           gpointer               insert_data,
                                                           GimpDataChooserDialog *dialog);


G_DEFINE_TYPE (GimpDataChooserDialog, gimp_data_chooser_dialog, GIMP_TYPE_DIALOG)

#define parent_class gimp_data_chooser_dialog_parent_class


static void
gimp_data_chooser_dialog_class_init (GimpDataChooserDialogClass *klass)
{
}

static void
gimp_data_chooser_dialog_init (GimpDataChooserDialog *dialog)
{
  dialog->filename        = NULL;
  dialog->factory_view    = NULL;
  dialog->working_factory = NULL;
}

/*  public functions  */

gchar *
gimp_data_chooser_dialog_new (GimpDataFactory *working_factory,
                              GimpViewType      view_type)
{
  GtkWidget       *dialog;
  GtkWidget       *factory_view;
  GimpDataFactory *factory;
  gchar           *filename = NULL;
  GType            data_type;
  gint             view_size = 32;
  gchar           *path      = NULL;
  gchar           *writable_path = NULL;
  gchar           *tmp;
  GimpContext     *context;
  gchar           *dirname;
  GList           *dirlist;
  GtkWidget       *path_view;
  GtkWidget       *hbox;
  GimpDocked      *docked;

  GimpContainerEditor *container_editor;

  g_object_get (working_factory->gimp->config,
                working_factory->path_property_name,     &path,
                working_factory->writable_property_name, &writable_path,
                NULL);

  g_print ("In data chooser dialog, for working factory:\n");
  g_print ("Path is '%s'\n", path);
  g_print ("Writable path is '%s'\n", writable_path);

  tmp = gimp_config_path_expand (path, TRUE, NULL);
  g_free (path);
  path = tmp;

  dirlist = gimp_path_parse (path, 256, FALSE, NULL);
  dirname = g_strdup (dirlist->data);

  data_type = working_factory->container->children_type;

  /* create a context */
  context = gimp_context_new (working_factory->gimp, "tmp", NULL);

  /* create the data factory */
  factory = gimp_data_factory_new (working_factory->gimp,
                                   data_type,
                                   working_factory->path_property_name,
                                   working_factory->path_property_name,
                                   working_factory->loader_entries,
                                   working_factory->n_loader_entries,
                                   NULL,
                                   NULL);

  /* load the data */
  gimp_data_factory_data_load_path (factory, dirname);

  /* create the data factory view */
  factory_view = gimp_data_factory_view_new (view_type, factory, context,
                                             view_size, 1,
                                             global_dock_factory->menu_factory,
                                             "<Brushes>",
                                             "/brushes-popup",
                                             "brushes");

  /* suppress the button bar */
  container_editor = GIMP_CONTAINER_EDITOR (factory_view);
  docked = GIMP_DOCKED (container_editor->view);
  gimp_docked_set_show_button_bar (docked, FALSE);

  /* kludge to prevent activation from opening an editor */
  gtk_widget_set_sensitive (GIMP_DATA_FACTORY_VIEW (factory_view)->edit_button,
                            FALSE);

  /* create the dialog */
  dialog = g_object_new (GIMP_TYPE_DATA_CHOOSER_DIALOG,
                         "title",                     _("Load Data"),
                         "role",                      "gimp-data-load",
                         NULL);

  GIMP_DATA_CHOOSER_DIALOG (dialog)->working_factory = working_factory;

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OPEN,   GTK_RESPONSE_OK,
                          NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_help_connect (GTK_WIDGET (dialog),
                     gimp_standard_help_func, GIMP_HELP_HELP, dialog);

  /* put an hbox in the main vbox */
  hbox = gtk_hbox_new (FALSE, 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  path_view = gimp_data_chooser_dialog_path_view (GIMP_DATA_CHOOSER_DIALOG (dialog),
                                                  dirlist);
  gtk_box_pack_start (GTK_BOX (hbox), path_view, FALSE, FALSE, 0);
  gtk_widget_show (path_view);

  gtk_box_pack_start (GTK_BOX (hbox), factory_view, FALSE, FALSE, 0);
  GIMP_DATA_CHOOSER_DIALOG (dialog)->factory_view = GIMP_DATA_FACTORY_VIEW (factory_view);
  gtk_widget_set_size_request (factory_view, 300, 400);
  gtk_widget_show (factory_view);

  gtk_widget_show (dialog);

  g_signal_connect_object (container_editor->view, "activate-item",
                           G_CALLBACK (gimp_data_chooser_dialog_activate_item),
                           dialog, 0);
  g_signal_connect_object (container_editor->view, "select-item",
                           G_CALLBACK (gimp_data_chooser_dialog_select_item),
                           dialog, 0);

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    filename = gimp_data_chooser_dialog_get_filename (GIMP_DATA_CHOOSER_DIALOG (dialog));

  gtk_widget_destroy (dialog);
  g_object_unref (context);
  return filename;
}

static gchar *
gimp_data_chooser_dialog_get_filename (GimpDataChooserDialog *dialog)
{
  return dialog->filename;
}

static GtkWidget *
gimp_data_chooser_dialog_path_view (GimpDataChooserDialog *dialog,
                                    GList                 *dirlist)
{
  GtkListStore      *list_store;
  GtkTreeIter        iter;
  GtkWidget         *view;
  GtkCellRenderer   *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection  *select;
  GList             *list;

  list_store = gtk_list_store_new (1, G_TYPE_STRING);

  for (list = dirlist; list; list = g_list_next (list))
    {
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter, 0, list->data, -1);
    }

  view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Folder", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (select), "changed",
                    G_CALLBACK (path_view_selection_changed),
                    dialog);
  return view;
}

static void
path_view_selection_changed (GtkTreeSelection *selection,
                             gpointer          data)
{
  GimpDataChooserDialog *dialog = GIMP_DATA_CHOOSER_DIALOG (data);
  GimpDataFactoryView   *factory_view;
  GtkTreeIter            iter;
  GtkTreeModel          *model;
  gchar                 *dirname;

  factory_view = dialog->factory_view;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      GimpContainerEditor *container_editor;

      container_editor = GIMP_CONTAINER_EDITOR (factory_view);

      gtk_tree_model_get (model, &iter, 0, &dirname, -1);

      gimp_data_factory_data_free (factory_view->factory);

      gimp_data_factory_data_load_path (factory_view->factory,
                                        dirname);
    }
}

static gboolean
gimp_data_chooser_dialog_select_item (GtkWidget             *widget,
                                      GimpData              *data,
                                      gpointer               insert_data,
                                      GimpDataChooserDialog *dialog)
{
  GimpDataFactoryView *factory_view;

  factory_view = dialog->factory_view;

  /* kludge to prevent activation from opening an editor */
  gtk_widget_set_sensitive (factory_view->edit_button,
                            FALSE);

  return FALSE;
}

static gboolean
gimp_data_chooser_dialog_activate_item (GtkWidget             *widget,
                                        GimpData              *data,
                                        gpointer               insert_data,
                                        GimpDataChooserDialog *dialog)
{
  GimpDataFactory     *factory;
  GimpDataFactory     *working_factory;
  GimpDataFactoryView *factory_view;
  gchar               *filename;
  gchar               *contents;
  gsize                length;
  GError              *error       = NULL;
  gchar               *writable_path;
  gchar               *tmp;
  gchar               *basename;
  gchar               *new_filename;
  struct stat          filestat;
  GimpDatafileData     file_data;
  GimpDataLoadContext  load_context;
  GimpContainerEditor *container_editor;

  g_return_val_if_fail (GIMP_IS_DATA_CHOOSER_DIALOG (dialog), FALSE);

  factory_view = dialog->factory_view;
  factory = factory_view->factory;
  working_factory = dialog->working_factory;

  container_editor = GIMP_CONTAINER_EDITOR (factory_view);

  if (! GIMP_IS_DATA (data))
    return FALSE;

  filename = data->filename;

  if (! filename)
    return FALSE;

  g_file_get_contents (filename, &contents, &length, &error);
  if (error)
    {
      g_print ("error getting contents of %s\n", filename);
      /* handle error */
      return FALSE;
    }

  g_object_get (working_factory->gimp->config,
                working_factory->writable_property_name, &writable_path,
                NULL);

  g_print ("writable path before expansion is '%s'\n", writable_path);

  tmp = gimp_config_path_expand (writable_path, TRUE, NULL);
  g_free (writable_path);
  writable_path = tmp;

  basename = g_path_get_basename (filename);

  new_filename = g_build_filename (writable_path, basename, NULL);

  if (g_file_test (new_filename, G_FILE_TEST_EXISTS))
    {
      g_print ("error setting contents of %s: file already exists\n", new_filename);
      g_free (contents);
      return FALSE;
      /* come up with a new name */
    }

  g_file_set_contents (new_filename, contents, length, &error);

  g_free (contents);

  if (error)
    {
      /* handle error */
      return FALSE;
    }

  g_stat (new_filename, &filestat);
  g_print ("loading new file %s\n", new_filename);

  file_data.filename = new_filename;
  file_data.dirname  = writable_path;
  file_data.basename = basename;
  file_data.atime    = filestat.st_atime;
  file_data.mtime    = filestat.st_mtime;
  file_data.ctime    = filestat.st_ctime;

  load_context.factory = working_factory;
  load_context.cache   = NULL;

  gimp_data_factory_load_data (&file_data, &load_context);

  g_free (new_filename);
  g_free (writable_path);
  g_free (basename);

/*   if (data) */
/*     { */
/*       gimp_context_set_by_type (context, */
/*                                 factory->container->children_type, */
/*                                 GIMP_OBJECT (data)); */
/*     } */

  return FALSE;
}
