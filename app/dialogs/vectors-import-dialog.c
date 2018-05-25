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

#include "vectors-import-dialog.h"

#include "gimp-intl.h"


typedef struct _VectorsImportDialog VectorsImportDialog;

struct _VectorsImportDialog
{
  GimpImage                 *image;
  gboolean                   merge_vectors;
  gboolean                   scale_vectors;
  GimpVectorsImportCallback  callback;
  gpointer                   user_data;
};


/*  local function prototypes  */

static void   vectors_import_dialog_free     (VectorsImportDialog *private);
static void   vectors_import_dialog_response (GtkWidget           *dialog,
                                              gint                 response_id,
                                              VectorsImportDialog *private);


/*  public function  */

GtkWidget *
vectors_import_dialog_new (GimpImage                 *image,
                           GtkWidget                 *parent,
                           GFile                     *import_folder,
                           gboolean                   merge_vectors,
                           gboolean                   scale_vectors,
                           GimpVectorsImportCallback  callback,
                           gpointer                   user_data)
{
  VectorsImportDialog *private;
  GtkWidget           *dialog;
  GtkWidget           *vbox;
  GtkWidget           *button;
  GtkFileFilter       *filter;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (import_folder == NULL || G_IS_FILE (import_folder),
                        NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (VectorsImportDialog);

  private->image         = image;
  private->merge_vectors = merge_vectors;
  private->scale_vectors = scale_vectors;
  private->callback      = callback;
  private->user_data     = user_data;

  dialog = gtk_file_chooser_dialog_new (_("Import Paths from SVG"), NULL,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,

                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Open"),   GTK_RESPONSE_OK,

                                        NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_role (GTK_WINDOW (dialog), "gimp-vectors-import");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_screen (GTK_WINDOW (dialog),
                         gtk_widget_get_screen (parent));

  if (import_folder)
    gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                              import_folder, NULL);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) vectors_import_dialog_free, private);

  g_signal_connect_object (image, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, 0);

  g_signal_connect (dialog, "delete-event",
                    G_CALLBACK (gtk_true),
                    NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (vectors_import_dialog_response),
                    private);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All files (*.*)"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Scalable SVG image (*.svg)"));
  gtk_file_filter_add_pattern (filter, "*.[Ss][Vv][Gg]");
  gtk_file_filter_add_mime_type (filter, "image/svg+xml");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), vbox);
  gtk_widget_show (vbox);

  button = gtk_check_button_new_with_mnemonic (_("_Merge imported paths"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->merge_vectors);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->merge_vectors);

  button = gtk_check_button_new_with_mnemonic (_("_Scale imported paths "
                                                 "to fit image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->scale_vectors);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->scale_vectors);

  return dialog;
}


/*  private functions  */

static void
vectors_import_dialog_free (VectorsImportDialog *private)
{
  g_slice_free (VectorsImportDialog, private);
}

static void
vectors_import_dialog_response (GtkWidget           *dialog,
                                gint                 response_id,
                                VectorsImportDialog *private)
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
                             private->merge_vectors,
                             private->scale_vectors,
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
