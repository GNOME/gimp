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

#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsizebox.h"
#include "widgets/gimpviewabledialog.h"

#include "print-size-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1


typedef struct _PrintSizeDialog PrintSizeDialog;

struct _PrintSizeDialog
{
  GimpImage              *image;
  GtkWidget              *box;
  GimpResolutionCallback  callback;
  gpointer                user_data;
};


static void   print_size_dialog_response (GtkWidget       *dialog,
                                          gint             response_id,
                                          PrintSizeDialog *private);
static void   print_size_dialog_reset    (PrintSizeDialog *private);


GtkWidget *
print_size_dialog_new (GimpImage              *image,
                       const gchar            *title,
                       const gchar            *role,
                       GtkWidget              *parent,
                       GimpHelpFunc            help_func,
                       const gchar            *help_id,
                       GimpResolutionCallback  callback,
                       gpointer                user_data)
{
  GtkWidget       *dialog;
  GtkWidget       *vbox;
  GtkWidget       *frame;
  PrintSizeDialog *private;
  gint             width, height;
  gdouble          xres, yres;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  width  = gimp_image_get_width (image);
  height = gimp_image_get_height (image);

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (image),
                                     title, role,
                                     GIMP_STOCK_PRINT_RESOLUTION, title,
                                     parent,
                                     help_func, help_id,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GIMP_STOCK_RESET, RESPONSE_RESET,
                                     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                     NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  private = g_new0 (PrintSizeDialog, 1);

  g_signal_connect_swapped (dialog, "destroy",
                            G_CALLBACK (g_free),
                            private);

  private->image     = image;
  private->callback  = callback;
  private->user_data = user_data;

  gimp_image_get_resolution (image, &xres, &yres);

  private->box = g_object_new (GIMP_TYPE_SIZE_BOX,
                               "width",           width,
                               "height",          height,
                               "unit",            gimp_image_get_unit (image),
                               "xresolution",     xres,
                               "yresolution",     yres,
                               "resolution-unit", gimp_image_get_unit (image),
                               "keep-aspect",     TRUE,
                               "edit-resolution", TRUE,
                               NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (print_size_dialog_response),
                    private);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  frame = gimp_frame_new (_("Print Size"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_container_add (GTK_CONTAINER (frame), private->box);
  gtk_widget_show (private->box);

  return dialog;
}

static void
print_size_dialog_response (GtkWidget       *dialog,
                            gint             response_id,
                            PrintSizeDialog *private)
{
  GimpUnit  resolution_unit;
  gdouble   xres, yres;

  switch (response_id)
    {
    case RESPONSE_RESET:
      print_size_dialog_reset (private);
      break;

    case GTK_RESPONSE_OK:
      g_object_get (private->box,
                    "xresolution",     &xres,
                    "yresolution",     &yres,
                    "resolution-unit", &resolution_unit,
                    NULL);

      private->callback (dialog,
                         private->image,
                         xres, yres, resolution_unit,
                         private->user_data);
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}

static void
print_size_dialog_reset (PrintSizeDialog *private)
{
  gdouble  xres, yres;

  gimp_image_get_resolution (private->image, &xres, &yres);

  g_object_set (private->box,
                "keep-aspect",     FALSE,
                "xresolution",     xres,
                "yresolution",     yres,
                "resolution-unit", gimp_image_get_unit (private->image),
                "keep-aspect",     TRUE,
                NULL);
}
