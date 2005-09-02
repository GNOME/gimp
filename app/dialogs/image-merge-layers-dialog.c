/* The GIMP -- an image manipulation program
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


/*  public functions  */

ImageMergeLayersDialog *
image_merge_layers_dialog_new (GimpImage     *image,
                               GimpContext   *context,
                               GtkWidget     *parent,
                               GimpMergeType  merge_type)
{
  ImageMergeLayersDialog *dialog;
  GtkWidget              *frame;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  dialog = g_new0 (ImageMergeLayersDialog, 1);

  dialog->gimage     = image;
  dialog->context    = context;
  dialog->merge_type = GIMP_EXPAND_AS_NECESSARY;

  dialog->dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (image),
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
                     (GWeakNotify) g_free, dialog);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

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

  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog->dialog)->vbox), frame);
  gtk_widget_show (frame);

  return dialog;
}
