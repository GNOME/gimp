/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * image-properties-dialog.c
 * Copyright (C) 2005 Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "core/ligmacontext.h"
#include "core/ligmaimage.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaimagecommenteditor.h"
#include "widgets/ligmaimagepropview.h"
#include "widgets/ligmaimageprofileview.h"
#include "widgets/ligmaviewabledialog.h"

#include "image-properties-dialog.h"

#include "ligma-intl.h"


GtkWidget *
image_properties_dialog_new (LigmaImage   *image,
                             LigmaContext *context,
                             GtkWidget   *parent)
{
  GtkWidget *dialog;
  GtkWidget *notebook;
  GtkWidget *view;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  dialog = ligma_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                     _("Image Properties"),
                                     "ligma-image-properties",
                                     "dialog-information",
                                     _("Image Properties"),
                                     parent,
                                     ligma_standard_help_func,
                                     LIGMA_HELP_IMAGE_PROPERTIES,

                                     _("_Close"), GTK_RESPONSE_CLOSE,

                                     NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  view = ligma_image_prop_view_new (image);
  gtk_container_set_border_width (GTK_CONTAINER (view), 12);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            view, gtk_label_new_with_mnemonic (_("_Properties")));
  gtk_widget_show (view);

  view = ligma_image_profile_view_new (image);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            view, gtk_label_new_with_mnemonic (_("C_olor Profile")));
  gtk_widget_show (view);

  view = ligma_image_comment_editor_new (image);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            view, gtk_label_new_with_mnemonic (_("Co_mment")));
  gtk_widget_show (view);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);

  return dialog;
}
