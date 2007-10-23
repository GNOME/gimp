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
#include "core/gimplayer.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "desaturate-dialog.h"

#include "gimp-intl.h"


static void  desaturate_dialog_free (DesaturateDialog *dialog);


DesaturateDialog *
desaturate_dialog_new (GimpDrawable       *drawable,
                       GimpContext        *context,
                       GtkWidget          *parent,
                       GimpDesaturateMode  mode)
{
  DesaturateDialog *dialog;
  GtkWidget        *vbox;
  GtkWidget        *frame;
  GtkWidget        *button;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  dialog = g_slice_new0 (DesaturateDialog);

  dialog->drawable = drawable;
  dialog->mode     = mode;

  dialog->dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (drawable), context,
                              _("Desaturate"), "gimp-drawable-desaturate",
                              GIMP_STOCK_CONVERT_GRAYSCALE,
                              _("Remove Colors"),
                              parent,
                              gimp_standard_help_func,
                              GIMP_HELP_LAYER_DESATURATE,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,

                              NULL);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog->dialog),
                                  _("_Desaturate"), GTK_RESPONSE_OK);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_stock (GIMP_STOCK_CONVERT_GRAYSCALE,
                                                  GTK_ICON_SIZE_BUTTON));

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog->dialog),
                     (GWeakNotify) desaturate_dialog_free, dialog);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog->dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  frame =
    gimp_enum_radio_frame_new (GIMP_TYPE_DESATURATE_MODE,
                               gtk_label_new (_("Choose shade of gray based on:")),
                               G_CALLBACK (gimp_radio_button_update),
                               &dialog->mode,
                               &button);

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (button), dialog->mode);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return dialog;
}

static void
desaturate_dialog_free (DesaturateDialog *dialog)
{
  g_slice_free (DesaturateDialog, dialog);
}
