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

#include "gui-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"

#include "display/gimpdisplay-foreach.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpcontainertreeview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"

#include "quit-dialog.h"

#include "gimp-intl.h"


static void  quit_dialog_response          (GtkWidget      *dialog,
                                            gint            response_id,
                                            Gimp           *gimp);
static void  quit_dialog_container_changed (GimpContainer  *images,
                                            GimpObject     *image,
                                            GimpMessageBox *box);


GtkWidget *
quit_dialog_new (Gimp *gimp)
{
  GimpContainer *images;
  GtkWidget     *dialog;
  GtkWidget     *box;
  GtkWidget     *label;
  GtkWidget     *view;
  GList         *list;
  gint           rows;
  gint           preview_size;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  images = gimp_displays_get_dirty_images (gimp);

  g_return_val_if_fail (images != NULL, NULL);

  for (list = gimp_action_groups_from_name ("file");
       list;
       list = g_list_next (list))
    {
      gimp_action_group_set_action_sensitive (list->data, "file-quit", FALSE);
    }

  dialog = gimp_dialog_new (_("Quit The GIMP?"), "gimp-quit",
                            NULL, 0,
                            gimp_standard_help_func,
                            GIMP_HELP_FILE_QUIT_CONFIRM,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_QUIT,   GTK_RESPONSE_OK,

                            NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (quit_dialog_response),
                    gimp);

  box = gimp_message_box_new (GIMP_STOCK_WILBER_EEK);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), box);
  gtk_widget_show (box);

  g_signal_connect_object (images, "remove",
                           G_CALLBACK (quit_dialog_container_changed),
                           box, 0);
  quit_dialog_container_changed (images, NULL, GIMP_MESSAGE_BOX (box));

  preview_size = gimp->config->layer_preview_size;

  view = gimp_container_tree_view_new (images, NULL, preview_size, 1);
  gtk_box_pack_start (GTK_BOX (box), view, TRUE, TRUE, 0);
  gtk_widget_show (view);

  rows = CLAMP (gimp_container_num_children (images), 3, 6);
  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (view),
                                       -1,
                                       rows * (preview_size + 2));

  label = gtk_label_new (_("If you quit GIMP now, "
                           "these changes will be lost."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  return dialog;
}

static void
quit_dialog_response (GtkWidget *dialog,
                      gint       response_id,
                      Gimp      *gimp)
{
  GList *list;

  gtk_widget_destroy (dialog);

  for (list = gimp_action_groups_from_name ("file");
       list;
       list = g_list_next (list))
    {
      gimp_action_group_set_action_sensitive (list->data, "file-quit", TRUE);
    }

  if (response_id == GTK_RESPONSE_OK)
    gimp_exit (gimp, TRUE);
}

static void
quit_dialog_container_changed (GimpContainer  *images,
                               GimpObject     *image,
                               GimpMessageBox *box)
{
  gint  num_images = gimp_container_num_children (images);

  if (num_images == 1)
    gimp_message_box_set_primary_text (box,
                                       _("There is one image with unsaved changes:"));
  else
    gimp_message_box_set_primary_text (box,
                                       _("There are %d images with unsaved changes:"),
                                       num_images);
}

