/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpimagepropview.h"
#include "widgets/gimpimageprofileview.h"
#include "widgets/gimpviewabledialog.h"

#include "image-properties-dialog.h"

#include "gimp-intl.h"


static GtkWidget * image_comment_view_new    (GimpImage     *image);
static void        image_comment_view_update (GtkWidget     *page,
                                              GtkTextBuffer *buffer);


/*  public functions  */

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

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), notebook,
                      FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  view = gimp_image_prop_view_new (image);
  gtk_container_set_border_width (GTK_CONTAINER (view), 12);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            view, gtk_label_new (_("Properties")));
  gtk_widget_show (view);

  view = gimp_image_profile_view_new (image);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            view, gtk_label_new (_("Color Profile")));
  gtk_widget_show (view);

  view = image_comment_view_new (image);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            view, gtk_label_new (_("Comment")));

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);

  return dialog;
}

static GtkWidget *
image_comment_view_new (GimpImage *image)
{
  GtkWidget     *view;
  GtkWidget     *scrolled_window;
  GtkWidget     *text_view;
  GtkTextBuffer *buffer;

  view = gimp_image_parasite_view_new (image, "gimp-comment");

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
  gtk_container_add (GTK_CONTAINER (view), scrolled_window);
  gtk_widget_show (scrolled_window);

  text_view = gtk_text_view_new ();
  gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (text_view), 6);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (text_view), 6);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (text_view), 6);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

  g_signal_connect (view, "update",
                    G_CALLBACK (image_comment_view_update),
                    buffer);

  image_comment_view_update (view, buffer);

  return view;
}

static void
image_comment_view_update (GtkWidget     *page,
                           GtkTextBuffer *buffer)
{
  GimpImageParasiteView *view = GIMP_IMAGE_PARASITE_VIEW (page);
  const GimpParasite    *parasite;

  parasite = gimp_image_parasite_view_get_parasite (view);

  if (parasite)
    {
      gchar *text = g_strndup (gimp_parasite_data (parasite),
                               gimp_parasite_data_size (parasite));

      if (! g_utf8_validate (text, -1, NULL))
        {
          gchar *tmp = gimp_any_to_utf8 (text, -1, NULL);

          g_free (text);
          text = tmp;
        }

      gtk_text_buffer_set_text (buffer, text, -1);
      g_free (text);

      gtk_widget_show (page);
    }
  else
    {
      gtk_widget_hide (page);
      gtk_text_buffer_set_text (buffer, "", 0);
    }
}
