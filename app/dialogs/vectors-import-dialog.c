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

#include "vectors-import-dialog.h"

#include "gimp-intl.h"


static void  vectors_import_dialog_free (VectorsImportDialog *dialog);


VectorsImportDialog *
vectors_import_dialog_new (GimpImage *image,
                           GtkWidget *parent,
                           gboolean   merge_vectors,
                           gboolean   scale_vectors)
{
  VectorsImportDialog *dialog;
  GtkWidget           *vbox;
  GtkWidget           *button;
  GtkFileFilter       *filter;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  dialog = g_slice_new0 (VectorsImportDialog);

  dialog->image         = image;
  dialog->merge_vectors = merge_vectors;
  dialog->scale_vectors = scale_vectors;

  dialog->dialog =
    gtk_file_chooser_dialog_new (_("Import Paths from SVG"), NULL,
                                 GTK_FILE_CHOOSER_ACTION_OPEN,

                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                 GTK_STOCK_OPEN,   GTK_RESPONSE_OK,

                                 NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_screen (GTK_WINDOW (dialog->dialog),
                         gtk_widget_get_screen (parent));

  gtk_window_set_role (GTK_WINDOW (dialog->dialog), "gimp-vectors-import");
  gtk_window_set_position (GTK_WINDOW (dialog->dialog), GTK_WIN_POS_MOUSE);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
                                   GTK_RESPONSE_OK);

  g_object_weak_ref (G_OBJECT (dialog->dialog),
                     (GWeakNotify) vectors_import_dialog_free, dialog);

  g_signal_connect_object (image, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog->dialog, 0);

  g_signal_connect (dialog->dialog, "delete-event",
                    G_CALLBACK (gtk_true),
                    NULL);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All files (*.*)"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog->dialog), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Scalable SVG image (*.svg)"));
  gtk_file_filter_add_pattern (filter, "*.[Ss][Vv][Gg]");
  gtk_file_filter_add_mime_type (filter, "image/svg+xml");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog->dialog), filter);

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog->dialog), filter);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog->dialog), vbox);
  gtk_widget_show (vbox);

  button = gtk_check_button_new_with_mnemonic (_("_Merge imported paths"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                dialog->merge_vectors);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &dialog->merge_vectors);

  button = gtk_check_button_new_with_mnemonic (_("_Scale imported paths "
                                                 "to fit image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                dialog->scale_vectors);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &dialog->scale_vectors);

  return dialog;
}

static void
vectors_import_dialog_free (VectorsImportDialog *dialog)
{
  g_slice_free (VectorsImportDialog, dialog);
}
