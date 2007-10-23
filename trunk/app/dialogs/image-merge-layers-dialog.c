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

#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "image-merge-layers-dialog.h"

#include "gimp-intl.h"


static void  image_merge_layers_dialog_free (ImageMergeLayersDialog *dialog);


ImageMergeLayersDialog *
image_merge_layers_dialog_new (GimpImage     *image,
                               GimpContext   *context,
                               GtkWidget     *parent,
                               GimpMergeType  merge_type,
                               gboolean       discard_invisible)
{
  ImageMergeLayersDialog *dialog;
  GtkWidget              *vbox;
  GtkWidget              *frame;
  GtkWidget              *button;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  dialog = g_slice_new0 (ImageMergeLayersDialog);

  dialog->image      = image;
  dialog->context    = context;
  dialog->merge_type = GIMP_EXPAND_AS_NECESSARY;

  dialog->dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                              _("Merge Layers"), "gimp-image-merge-layers",
                              GIMP_STOCK_MERGE_DOWN,
                              _("Layers Merge Options"),
                              parent,
                              gimp_standard_help_func,
                              GIMP_HELP_IMAGE_MERGE_LAYERS,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              _("_Merge"),      GTK_RESPONSE_OK,

                              NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog->dialog),
                     (GWeakNotify) image_merge_layers_dialog_free, dialog);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog->dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  frame = gimp_int_radio_group_new (TRUE, _("Final, Merged Layer should be:"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &dialog->merge_type, dialog->merge_type,

                                    _("Expanded as necessary"),
                                    GIMP_EXPAND_AS_NECESSARY, NULL,

                                    _("Clipped to image"),
                                    GIMP_CLIP_TO_IMAGE, NULL,

                                    _("Clipped to bottom layer"),
                                    GIMP_CLIP_TO_BOTTOM_LAYER, NULL,

                                    NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  button = gtk_check_button_new_with_mnemonic (_("_Discard invisible layers"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                dialog->discard_invisible);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &dialog->discard_invisible);


  return dialog;
}

static void
image_merge_layers_dialog_free (ImageMergeLayersDialog *dialog)
{
  g_slice_free (ImageMergeLayersDialog, dialog);
}
