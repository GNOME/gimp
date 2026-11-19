/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"

#include "dialogs-types.h"

#include "core/gimpimage.h"

#include "path/gimppath.h"
#include "path/gimppath-import.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpview.h"
#include "widgets/gimpwidgets-utils.h"

#include "path-import-dialog.h"

#include "gimp-intl.h"


typedef struct _PathImportDialog PathImportDialog;

struct _PathImportDialog
{
  GimpImage              *image;
  GtkWidget              *file_chooser;
  GtkWidget              *preview;

  gboolean                merge_paths;
  gboolean                scale_paths;
  GimpPathImportCallback  callback;
  gpointer                user_data;
};


/*  local function prototypes  */
static void   path_import_dialog_free     (PathImportDialog *private);
static void   path_import_dialog_response (GtkWidget        *dialog,
                                           gint              response_id,
                                           PathImportDialog *private);
static void   path_import_dialog_file_set (GtkFileChooser   *widget,
                                           PathImportDialog *private);

/*  public function  */

GtkWidget *
path_import_dialog_new (GimpImage              *image,
                        GimpContext            *context,
                        GtkWidget              *parent,
                        GFile                  *import_folder,
                        gboolean                merge_paths,
                        gboolean                scale_paths,
                        GimpPathImportCallback  callback,
                        gpointer                user_data)
{
  PathImportDialog *private;
  GtkWidget        *dialog;
  GtkWidget        *main_hbox;
  GtkWidget        *vbox;
  GtkWidget        *grid;
  GtkWidget        *frame;
  GtkWidget        *button;
  GtkFileFilter    *filter;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (import_folder == NULL || G_IS_FILE (import_folder),
                        NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (PathImportDialog);

  private->image       = image;
  private->merge_paths = merge_paths;
  private->scale_paths = scale_paths;
  private->callback    = callback;
  private->user_data   = user_data;

  dialog = gimp_dialog_new (_("Import Paths from SVG"),
                            "gimp-path-import", NULL, 0,
                            gimp_standard_help_func,
                            GIMP_HELP_PATH_IMPORT,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Import"), GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) path_import_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (path_import_dialog_response),
                    private);

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_hbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (main_hbox, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  /* The "Select Source" frame */
  frame = gimp_frame_new (_("Select Source"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  private->file_chooser =
    gtk_file_chooser_button_new (_("Select SVG File"),
                                 GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_container_add (GTK_CONTAINER (frame), private->file_chooser);
  gtk_widget_set_visible (private->file_chooser, TRUE);

  g_signal_connect (private->file_chooser, "file-set",
                    G_CALLBACK (path_import_dialog_file_set),
                    private);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All files (*.*)"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (private->file_chooser),
                               filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Scalable SVG image (*.svg)"));
  gtk_file_filter_add_pattern (filter, "*.svg");
  gtk_file_filter_add_pattern (filter, "*.SVG");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (private->file_chooser),
                               filter);

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (private->file_chooser),
                               filter);

  /*  The "Import Options" frame  */
  frame = gimp_frame_new (_("Import Options"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_set_visible (grid, TRUE);

  button = gtk_check_button_new_with_mnemonic (_("_Merge imported paths"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->merge_paths);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0, NULL, 0.0, 0.5,
                            button, 2);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->merge_paths);

  button = gtk_check_button_new_with_mnemonic (_("_Scale imported paths "
                                                 "to fit image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->scale_paths);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1, NULL, 0.0, 0.5,
                            button, 2);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->scale_paths);

  /*  The "Preview" frame  */
  frame = gimp_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  private->preview = gimp_view_new_full_by_types (context,
                                                  GIMP_TYPE_VIEW,
                                                  GIMP_TYPE_PATH,
                                                  148, 148, 1,
                                                  TRUE, FALSE, FALSE);
  gtk_widget_set_halign (private->preview, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), private->preview, FALSE, FALSE, 0);
  gtk_widget_set_visible (private->preview, TRUE);

  g_signal_connect_object (image, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, 0);

  return dialog;
}


/*  private functions  */

static void
path_import_dialog_free (PathImportDialog *private)
{
  g_signal_handlers_disconnect_by_func (private->file_chooser,
                                        G_CALLBACK (path_import_dialog_file_set),
                                        private);

  g_slice_free (PathImportDialog, private);
}

static void
path_import_dialog_response (GtkWidget        *dialog,
                             gint              response_id,
                             PathImportDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (private->file_chooser);
      GFile          *file;

      file = gtk_file_chooser_get_file (chooser);

      if (file)
        {
          GFile *folder;

          folder = gtk_file_chooser_get_current_folder_file (chooser);

          private->callback (dialog,
                             private->image,
                             file,
                             folder,
                             private->merge_paths,
                             private->scale_paths,
                             private->user_data);

          if (folder)
            g_object_unref (folder);

          g_object_unref (file);
        }
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

static void
path_import_dialog_file_set (GtkFileChooser   *widget,
                             PathImportDialog *private)
{
  GFile  *file;
  GError *error = NULL;
  GList  *paths = NULL;

  file = gtk_file_chooser_get_file (widget);
  if (file != NULL)
    {
      gboolean success;

      success = gimp_path_import_get_preview (private->image, file,
                                              NULL, 0, private->scale_paths,
                                              &paths, &error);

      gimp_view_set_viewable (GIMP_VIEW (private->preview),
                              success ? GIMP_VIEWABLE (paths->data) : NULL);

      g_list_free (paths);
    }
}
