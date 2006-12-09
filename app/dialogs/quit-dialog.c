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
#include "core/gimpcontext.h"
#include "core/gimplist.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "widgets/gimpcontainertreeview.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdnd.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"

#include "quit-dialog.h"

#include "gimp-intl.h"


static GtkWidget * quit_close_all_dialog_new               (Gimp              *gimp,
                                                            gboolean           do_quit);
static void        quit_close_all_dialog_response          (GtkWidget         *dialog,
                                                            gint               response_id,
                                                            Gimp              *gimp);
static void        quit_close_all_dialog_container_changed (GimpContainer     *images,
                                                            GimpObject        *image,
                                                            GimpMessageBox    *box);
static void        quit_close_all_dialog_image_activated   (GimpContainerView *view,
                                                            GimpImage         *image,
                                                            gpointer           insert_data,
                                                            Gimp              *gimp);


/*  public functions  */

GtkWidget *
quit_dialog_new (Gimp *gimp)
{
  return quit_close_all_dialog_new (gimp, TRUE);
}

GtkWidget *
close_all_dialog_new (Gimp *gimp)
{
  return quit_close_all_dialog_new (gimp, FALSE);
}

static GtkWidget *
quit_close_all_dialog_new (Gimp     *gimp,
                           gboolean  do_quit)
{
  GimpContainer  *images;
  GimpContext    *context;
  GimpMessageBox *box;
  GtkWidget      *dialog;
  GtkWidget      *label;
  GtkWidget      *button;
  GtkWidget      *view;
  GtkWidget      *dnd_widget;
  gint            rows;
  gint            view_size;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

#ifdef __GNUC__
#warning FIXME: need container of dirty images
#endif

  images  = gimp_displays_get_dirty_images (gimp);
  context = gimp_context_new (gimp, "close-all-dialog",
                              gimp_get_user_context (gimp));

  g_return_val_if_fail (images != NULL, NULL);

  dialog =
    gimp_message_dialog_new (do_quit ? _("Quit GIMP") : _("Close All Images"),
                             GIMP_STOCK_WARNING,
                             NULL, 0,
                             gimp_standard_help_func,
                             do_quit ?
                             GIMP_HELP_FILE_QUIT : GIMP_HELP_FILE_CLOSE_ALL,

                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,

                             NULL);

  g_object_set_data_full (G_OBJECT (dialog), "dirty-images",
                          images, (GDestroyNotify) g_object_unref);
  g_object_set_data_full (G_OBJECT (dialog), "dirty-images-context",
                          context, (GDestroyNotify) g_object_unref);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (quit_close_all_dialog_response),
                    gimp);

  box = GIMP_MESSAGE_DIALOG (dialog)->box;

  button = gtk_dialog_add_button (GTK_DIALOG (dialog), "", GTK_RESPONSE_OK);

  g_object_set_data (G_OBJECT (box), "ok-button", button);
  g_object_set_data (G_OBJECT (box), "do-quit", GINT_TO_POINTER (do_quit));

  g_signal_connect_object (images, "add",
                           G_CALLBACK (quit_close_all_dialog_container_changed),
                           box, 0);
  g_signal_connect_object (images, "remove",
                           G_CALLBACK (quit_close_all_dialog_container_changed),
                           box, 0);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  view_size = gimp->config->layer_preview_size;
  rows      = CLAMP (gimp_container_num_children (images), 3, 6);

  view = gimp_container_tree_view_new (images, context, view_size, 1);
  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (view),
                                       -1,
                                       rows * (view_size + 2));
  gtk_box_pack_start (GTK_BOX (box), view, TRUE, TRUE, 0);
  gtk_widget_show (view);

  g_signal_connect (view, "activate-item",
                    G_CALLBACK (quit_close_all_dialog_image_activated),
                    gimp);

  dnd_widget = gimp_container_view_get_dnd_widget (GIMP_CONTAINER_VIEW (view));
  gimp_dnd_xds_source_add (dnd_widget,
                           (GimpDndDragViewableFunc) gimp_dnd_get_drag_data,
                           NULL);

  if (do_quit)
    label = gtk_label_new (_("If you quit GIMP now, "
                             "these changes will be lost."));
  else
    label = gtk_label_new (_("If you close these images now, "
                             "changes will be lost."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_object_set_data (G_OBJECT (box), "lost-label", label);

  quit_close_all_dialog_container_changed (images, NULL,
                                           GIMP_MESSAGE_DIALOG (dialog)->box);

  return dialog;
}

static void
quit_close_all_dialog_response (GtkWidget *dialog,
                                gint       response_id,
                                Gimp      *gimp)
{
  GimpMessageBox *box     = GIMP_MESSAGE_DIALOG (dialog)->box;
  gboolean        do_quit = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (box),
                                                                "do-quit"));

  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      if (do_quit)
        gimp_exit (gimp, TRUE);
      else
        gimp_displays_delete (gimp);
    }
}

static void
quit_close_all_dialog_container_changed (GimpContainer  *images,
                                         GimpObject     *image,
                                         GimpMessageBox *box)
{
  gint       num_images = gimp_container_num_children (images);
  GtkWidget *label      = g_object_get_data (G_OBJECT (box), "lost-label");
  GtkWidget *button     = g_object_get_data (G_OBJECT (box), "ok-button");
  GtkWidget *dialog     = gtk_widget_get_toplevel (button);
  gboolean   do_quit    = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (box),
                                                              "do-quit"));
  gchar     *text;

  text = g_strdup_printf (ngettext ("There is one image with unsaved changes:",
                                    "There are %d images with unsaved changes:",
                                    num_images), num_images);
  gimp_message_box_set_primary_text (box, text);
  g_free (text);

  if (num_images == 0)
    {
      gtk_widget_hide (label);
      g_object_set (button,
                    "label",     do_quit ? GTK_STOCK_QUIT : GTK_STOCK_CLOSE,
                    "use-stock", TRUE,
                    "image",     NULL,
                    NULL);
      gtk_widget_grab_default (button);
    }
  else
    {
      GtkWidget *icon = gtk_image_new_from_stock (GTK_STOCK_DELETE,
                                                  GTK_ICON_SIZE_BUTTON);
      gtk_widget_show (label);
      g_object_set (button,
                    "label",     _("_Discard Changes"),
                    "use-stock", FALSE,
                    "image",     icon,
                    NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                       GTK_RESPONSE_CANCEL);
    }
}

static void
quit_close_all_dialog_image_activated (GimpContainerView *view,
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

      if (display->image == image)
        gtk_window_present (GTK_WINDOW (display->shell));
    }
}
