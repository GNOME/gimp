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

#include "dialogs-types.h"

#include "core/gimpimage.h"

#include "widgets/gimpwidgets-utils.h"

#include "path-export-dialog.h"

#include "gimp-intl.h"


typedef struct _PathExportDialog PathExportDialog;

struct _PathExportDialog
{
  GimpImage              *image;
  gboolean                active_only;
  GimpPathExportCallback  callback;
  gpointer                user_data;
};


/*  local function prototypes  */
static void   path_export_dialog_free     (PathExportDialog *private);
static void   path_export_dialog_response (GtkNativeDialog  *widget,
                                           gint              response_id,
                                           PathExportDialog *private);


/*  public function  */

GtkNativeDialog *
path_export_dialog_new (GimpImage              *image,
                        GtkWidget              *parent,
                        GFile                  *export_folder,
                        gboolean                active_only,
                        GimpPathExportCallback  callback,
                        gpointer                user_data)
{
  PathExportDialog     *private;
  GtkFileChooserNative *dialog;
  GtkFileFilter        *filter;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (export_folder == NULL || G_IS_FILE (export_folder),
                        NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (PathExportDialog);

  private->image       = image;
  private->active_only = active_only;
  private->callback    = callback;
  private->user_data   = user_data;

  dialog = gtk_file_chooser_native_new (_("Export Path to SVG"), NULL,
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        _("_Save"), _("_Cancel"));

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                  TRUE);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "SVG (*.svg)");
  gtk_file_filter_add_pattern (filter, "*.svg");
  gtk_file_filter_add_pattern (filter, "*.SVG");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  if (export_folder)
    gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                              export_folder, NULL);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) path_export_dialog_free, private);

  g_signal_connect_object (image, "disconnect",
                           G_CALLBACK (gtk_native_dialog_destroy),
                           dialog, 0);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (path_export_dialog_response),
                    private);

  /* Add dropdown option for which path(s) to export */
  if (dialog)
    {
      const gchar *options[3] = { "selected", "all", NULL };
      const gchar *labels[3]  = { _("Export the selected paths"),
                                  _("Export all paths from this image"),
                                  NULL };

      gtk_file_chooser_add_choice (GTK_FILE_CHOOSER (dialog), "export-paths",
                                   _("Options"), options, labels);
      gtk_file_chooser_set_choice (GTK_FILE_CHOOSER (dialog), "export-paths",
                                   private->active_only ? "selected" : "all");
    }

  return GTK_NATIVE_DIALOG (dialog);
}


/*  private functions  */

static void
path_export_dialog_free (PathExportDialog *private)
{
  g_slice_free (PathExportDialog, private);
}

static void
path_export_dialog_response (GtkNativeDialog  *dialog,
                             gint              response_id,
                             PathExportDialog *private)
{
  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
      GFile          *file;
      const gchar    *export_paths;

      file         = gtk_file_chooser_get_file (chooser);
      export_paths = gtk_file_chooser_get_choice (chooser, "export-paths");

      private->active_only = (! g_strcmp0 (export_paths, "selected"));

      if (file)
        {
          GFile *folder;

          folder = gtk_file_chooser_get_current_folder_file (chooser);

          private->callback (dialog,
                             private->image,
                             file,
                             folder,
                             private->active_only,
                             private->user_data);

          if (folder)
            g_object_unref (folder);

          g_object_unref (file);
        }
    }
  else
    {
      gtk_native_dialog_destroy (dialog);
    }
}
