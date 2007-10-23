/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimpimage.h"

#include "vectors-export-dialog.h"

#include "gimp-intl.h"


static void  vectors_export_dialog_free (VectorsExportDialog *dialog);


VectorsExportDialog *
vectors_export_dialog_new (GimpImage *image,
                           GtkWidget *parent,
                           gboolean   active_only)
{
  VectorsExportDialog *dialog;
  GtkWidget           *combo;
  GtkFileChooser      *chooser;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  dialog = g_slice_new0 (VectorsExportDialog);

  dialog->image       = image;
  dialog->active_only = active_only;

  dialog->dialog =
    gtk_file_chooser_dialog_new (_("Export Path to SVG"), NULL,
                                 GTK_FILE_CHOOSER_ACTION_SAVE,

                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                 GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                                 NULL);

  chooser = GTK_FILE_CHOOSER (dialog->dialog);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_screen (GTK_WINDOW (dialog->dialog),
                         gtk_widget_get_screen (parent));

  gtk_window_set_role (GTK_WINDOW (dialog->dialog), "gimp-vectors-export");
  gtk_window_set_position (GTK_WINDOW (dialog->dialog), GTK_WIN_POS_MOUSE);

  gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_OK);
  gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

  g_object_weak_ref (G_OBJECT (dialog->dialog),
                     (GWeakNotify) vectors_export_dialog_free, dialog);

  g_signal_connect_object (image, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog->dialog, 0);

  g_signal_connect (dialog->dialog, "delete-event",
                    G_CALLBACK (gtk_true),
                    NULL);

  combo = gimp_int_combo_box_new (_("Export the active path"),           TRUE,
                                  _("Export all paths from this image"), FALSE,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 dialog->active_only);
  gtk_file_chooser_set_extra_widget (chooser, combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &dialog->active_only);

  return dialog;
}

static void
vectors_export_dialog_free (VectorsExportDialog *dialog)
{
  g_slice_free (VectorsExportDialog, dialog);
}
