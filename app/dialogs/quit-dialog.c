/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimplist.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "widgets/gimpcontainertreeview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"

#include "quit-dialog.h"

#include "gimp-intl.h"


static void  quit_dialog_response          (GtkWidget         *dialog,
                                            gint               response_id,
                                            Gimp              *gimp);
static void  quit_dialog_container_changed (GimpContainer     *images,
                                            GimpObject        *image,
                                            GimpMessageBox    *box);
static void  quit_dialog_image_activated   (GimpContainerView *view,
                                            GimpImage         *image,
                                            gpointer           insert_data,
                                            Gimp              *gimp);


/*  public functions  */

GtkWidget *
quit_dialog_new (Gimp *gimp)
{
  GimpContainer  *images;
  GimpMessageBox *box;
  GtkWidget      *dialog;
  GtkWidget      *label;
  GtkWidget      *button;
  GtkWidget      *view;
  gint            rows;
  gint            preview_size;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

#ifdef __GNUC__
#warning FIXME: need container of dirty images
#endif

  images = gimp_displays_get_dirty_images (gimp);

  g_return_val_if_fail (images != NULL, NULL);

  dialog =
    gimp_message_dialog_new (_("Quit The GIMP"), GIMP_STOCK_WILBER_EEK,
                             NULL, 0,
                             gimp_standard_help_func, NULL,

                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,

                             NULL);

  g_object_set_data_full (G_OBJECT (dialog), "dirty-images",
                          images, (GDestroyNotify) g_object_unref);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (quit_dialog_response),
                    gimp);

  box = GIMP_MESSAGE_DIALOG (dialog)->box;

  button = gtk_dialog_add_button (GTK_DIALOG (dialog), "", GTK_RESPONSE_OK);

  g_object_set_data (G_OBJECT (box), "ok-button", button);

  g_signal_connect_object (images, "add",
                           G_CALLBACK (quit_dialog_container_changed),
                           box, 0);
  g_signal_connect_object (images, "remove",
                           G_CALLBACK (quit_dialog_container_changed),
                           box, 0);

  preview_size = gimp->config->layer_preview_size;
  rows         = CLAMP (gimp_container_num_children (images), 3, 6);

  view = gimp_container_tree_view_new (images, NULL, preview_size, 1);
  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (view),
                                       -1,
                                       rows * (preview_size + 2));
  gtk_box_pack_start (GTK_BOX (box), view, TRUE, TRUE, 0);
  gtk_widget_show (view);

  g_signal_connect (view, "activate-item",
                    G_CALLBACK (quit_dialog_image_activated),
                    gimp);

  label = gtk_label_new (_("If you quit GIMP now, "
                           "these changes will be lost."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_object_set_data (G_OBJECT (box), "lost-label", label);

  quit_dialog_container_changed (images, NULL,
                                 GIMP_MESSAGE_DIALOG (dialog)->box);

  return dialog;
}

static void
quit_dialog_response (GtkWidget *dialog,
                      gint       response_id,
                      Gimp      *gimp)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    gimp_exit (gimp, TRUE);
}

static void
quit_dialog_container_changed (GimpContainer  *images,
                               GimpObject     *image,
                               GimpMessageBox *box)
{
  gint       num_images = gimp_container_num_children (images);
  GtkWidget *label      = g_object_get_data (G_OBJECT (box), "lost-label");
  GtkWidget *button     = g_object_get_data (G_OBJECT (box), "ok-button");
  GtkWidget *dialog     = gtk_widget_get_toplevel (button);

  if (num_images == 1)
    gimp_message_box_set_primary_text (box,
                                       _("There is one image with unsaved changes:"));
  else
    gimp_message_box_set_primary_text (box,
                                       _("There are %d images with unsaved changes:"),
                                       num_images);

  if (num_images == 0)
    {
      gtk_widget_hide (label);
      g_object_set (button,
                    "label",     GTK_STOCK_QUIT,
                    "use-stock", TRUE,
                    NULL);
      gtk_widget_grab_default (button);
    }
  else
    {
      gtk_widget_show (label);
      g_object_set (button,
                    "label",     _("_Discard Changes"),
                    "use-stock", FALSE,
                    NULL);
      gtk_window_set_default (GTK_WINDOW (dialog), NULL);
    }
}

static void
quit_dialog_image_activated (GimpContainerView *view,
                             GimpImage         *image,
                             gpointer           insert_data,
                             Gimp              *gimp)
{
  GList *list;

  for (list = GIMP_LIST (gimp->displays)->list;
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;

      if (display->gimage == image)
        gtk_window_present (GTK_WINDOW (display->shell));
    }
}
