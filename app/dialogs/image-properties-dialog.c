/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * image-properties-dialog.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpimagecommenteditor.h"
#include "widgets/gimpimagepropview.h"
#include "widgets/gimpimageprofileview.h"
#include "widgets/gimpviewabledialog.h"

#include "image-properties-dialog.h"

#include "gimp-intl.h"


GtkWidget *
image_properties_dialog_new (GimpImage   *image,
                             GimpContext *context,
                             GtkWidget   *parent)
{
  GtkWidget *dialog;
  GtkWidget *notebook;
  GtkWidget *view;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                                     _("Image Properties"),
                                     "gimp-image-properties",
                                     "dialog-information",
                                     _("Image Properties"),
                                     parent,
                                     gimp_standard_help_func,
                                     GIMP_HELP_IMAGE_PROPERTIES,

                                     _("_Close"), GTK_RESPONSE_OK,

                                     NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  view = gimp_image_prop_view_new (image);
  gtk_container_set_border_width (GTK_CONTAINER (view), 12);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            view, gtk_label_new_with_mnemonic (_("_Properties")));
  gtk_widget_show (view);

  view = gimp_image_profile_view_new (image);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            view, gtk_label_new_with_mnemonic (_("C_olor Profile")));
  gtk_widget_show (view);

  view = gimp_image_comment_editor_new (image);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            view, gtk_label_new_with_mnemonic (_("Co_mment")));
  gtk_widget_show (view);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);

  return dialog;
}
