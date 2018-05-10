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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimpimage.h"

#include "vectors-export-dialog.h"

#include "gimp-intl.h"


typedef struct _VectorsExportDialog VectorsExportDialog;

struct _VectorsExportDialog
{
  GimpImage                 *image;
  gboolean                   active_only;
  GimpVectorsExportCallback  callback;
  gpointer                   user_data;
};


/*  local function prototypes  */

static void   vectors_export_dialog_free     (VectorsExportDialog *private);
static void   vectors_export_dialog_response (GtkWidget           *widget,
                                              gint                 response_id,
                                              VectorsExportDialog *private);


/*  public function  */

GtkWidget *
vectors_export_dialog_new (GimpImage                 *image,
                           GtkWidget                 *parent,
                           GFile                     *export_folder,
                           gboolean                   active_only,
                           GimpVectorsExportCallback  callback,
                           gpointer                   user_data)
{
  VectorsExportDialog *private;
  GtkWidget           *dialog;
  GtkWidget           *combo;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (export_folder == NULL || G_IS_FILE (export_folder),
                        NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (VectorsExportDialog);

  private->image       = image;
  private->active_only = active_only;
  private->callback    = callback;
  private->user_data   = user_data;

  dialog = gtk_file_chooser_dialog_new (_("Export Path to SVG"), NULL,
                                        GTK_FILE_CHOOSER_ACTION_SAVE,

                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Save"),   GTK_RESPONSE_OK,

                                        NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_role (GTK_WINDOW (dialog), "gimp-vectors-export");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_screen (GTK_WINDOW (dialog),
                         gtk_widget_get_screen (parent));

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                  TRUE);

  if (export_folder)
    gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                              export_folder, NULL);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) vectors_export_dialog_free, private);

  g_signal_connect_object (image, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, 0);

  g_signal_connect (dialog, "delete-event",
                    G_CALLBACK (gtk_true),
                    NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (vectors_export_dialog_response),
                    private);

  combo = gimp_int_combo_box_new (_("Export the active path"),           TRUE,
                                  _("Export all paths from this image"), FALSE,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 private->active_only);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &private->active_only);

  return dialog;
}


/*  private functions  */

static void
vectors_export_dialog_free (VectorsExportDialog *private)
{
  g_slice_free (VectorsExportDialog, private);
}

static void
vectors_export_dialog_response (GtkWidget           *dialog,
                                gint                 response_id,
                                VectorsExportDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
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
                             private->active_only,
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
