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
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpcontainertreeview.h"

#include "quit-dialog.h"

#include "gimp-intl.h"


static void  quit_dialog_response (GtkWidget *dialog,
                                   gint       response_id,
                                   Gimp      *gimp);


GtkWidget *
quit_dialog_new (Gimp *gimp)
{
  GimpContainer *images;
  GtkWidget     *dialog;
  GtkWidget     *hbox;
  GtkWidget     *vbox;
  GtkWidget     *image;
  GtkWidget     *label;
  GtkWidget     *scrolled_window;
  GtkWidget     *view;
  GList         *list;
  gchar         *msg;
  gint           num_images;
  gint           preview_size;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  images = gimp_displays_get_dirty_images (gimp);
  num_images = gimp_container_num_children (images);

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

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GIMP_STOCK_WILBER_EEK,
                                    GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  if (num_images == 1)
    {
      msg = g_strdup_printf (_("There is one image with unsaved changes:"));
    }
  else
    {
      msg = g_strdup_printf (_("There are %d images with unsaved changes:"),
                             num_images);
    }

  label = gtk_label_new (msg);
  g_free (msg);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  preview_size = gimp->config->layer_preview_size;

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  /* FIXME */
  gtk_widget_set_size_request (scrolled_window,
                               -1,
                               CLAMP (num_images, 3, 6) * (preview_size + 6));

  view = gimp_container_tree_view_new (images,
                                       gimp_get_user_context (gimp),
                                       preview_size, 1);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
                                         view);
  gtk_widget_show (view);

  label = gtk_label_new (_("If you quit GIMP now "
                           "these changes will be lost."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
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
