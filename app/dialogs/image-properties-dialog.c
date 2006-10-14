/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * image-properties-dialog.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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
#include "widgets/gimpimagepropview.h"
#include "widgets/gimpviewabledialog.h"

#include "image-properties-dialog.h"

#include "gimp-intl.h"


/*  public functions  */

GtkWidget *
image_properties_dialog_new (GimpImage   *image,
                             GimpContext *context,
                             GtkWidget   *parent)
{
  GtkWidget *dialog;
  GtkWidget *view;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (image), context,
                                     _("Image Properties"),
                                     "gimp-image-properties",
                                     GTK_STOCK_INFO,
                                     _("Image Properties"),
                                     parent,
                                     gimp_standard_help_func,
                                     GIMP_HELP_IMAGE_PROPERTIES,

                                     GTK_STOCK_CLOSE, GTK_RESPONSE_OK,

                                     NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  view = gimp_image_prop_view_new (image);
  gtk_container_set_border_width (GTK_CONTAINER (view), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), view,
                      FALSE, FALSE, 0);
  gtk_widget_show (view);

  return dialog;
}
