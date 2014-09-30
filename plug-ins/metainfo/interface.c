/* interface.c - user interface for the metadata editor
 *
 * Copyright (C) 2014, Hartmut Kuhse <hatti@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include <glib.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "interface.h"
#include "metainfo.h"

#include "libgimp/stdplugins-intl.h"

static gboolean    save_attributes_to_image;
static GtkWidget  *save_button;

/* show a transient message dialog */
void
metainfo_message_dialog (GtkMessageType  type,
                         GtkWindow      *parent,
                         const gchar    *title,
                         const gchar    *message)
{
  GtkWidget *dlg;

  dlg = gtk_message_dialog_new (parent, 0, type, GTK_BUTTONS_OK, "%s", message);

  if (title)
    gtk_window_set_title (GTK_WINDOW (dlg), title);

  gtk_window_set_role (GTK_WINDOW (dlg), "metainfo-message");
  gtk_dialog_run (GTK_DIALOG (dlg));
  gtk_widget_destroy (dlg);
}

static void
metainfo_dialog_response (GtkWidget *widget,
                          gint       response_id,
                          gpointer   data)
{
  GimpAttributes *attributes = (GimpAttributes *) data;

  switch (response_id)
    {
    case METAINFO_RESPONSE_SAVE:
      page_description_save_to_attributes (attributes);
      page_artwork_save_to_attributes (attributes);
      page_administration_save_to_attributes (attributes);
      page_rights_save_to_attributes (attributes);
      save_attributes_to_image = TRUE;
      break;
    case GTK_RESPONSE_OK:
    default:
      save_attributes_to_image = FALSE;
      break;
    }
  gtk_widget_destroy (widget);
}

gboolean
metainfo_get_save_attributes (void)
{
  return save_attributes_to_image;
}

gboolean
metainfo_dialog (gint32          image_id,
                 GimpAttributes *attributes)
{
  GtkBuilder *builder;
  GtkWidget  *dialog;
  GtkWidget  *attributes_vbox;
  GtkWidget  *content_area;
  GdkPixbuf  *pixbuf;
  GtkWidget  *label_header;
  GtkWidget  *label_info;
  GtkWidget  *thumb_box;

  gchar      *header;
  gchar      *ui_file;
  gchar      *title;
  gchar      *fname;
  gchar      *role;
  GError     *error = NULL;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  fname = g_filename_display_basename (gimp_image_get_uri (image_id));
  header  = g_strdup_printf ("Image");
  role  = g_strdup_printf ("gimp-image-metainfo-dialog");
  pixbuf = gimp_image_get_thumbnail (image_id, THUMB_SIZE, THUMB_SIZE,
                                     GIMP_PIXBUF_SMALL_CHECKS);
  title = g_strdup_printf ("Digital Rights: %s", fname);

  builder = gtk_builder_new ();

  ui_file = g_build_filename (gimp_data_directory (),
                              "ui", "plug-ins", "plug-in-metainfo.ui", NULL);

  if (! gtk_builder_add_from_file (builder, ui_file, &error))
    {
      g_printerr ("Error occured while loading UI file!\n");
      g_printerr ("Message: %s\n", error->message);
      g_clear_error (&error);
      g_free (ui_file);
      g_object_unref (builder);
      return FALSE;
    }

  g_free (ui_file);

  dialog = gimp_dialog_new (title,
                            role,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_HELP,

                            GTK_STOCK_OK, GTK_RESPONSE_OK,
                            NULL);

  save_button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_SAVE, METAINFO_RESPONSE_SAVE);
  gtk_widget_set_sensitive (GTK_WIDGET (save_button), FALSE);
  set_save_attributes_button (save_button);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (metainfo_dialog_response),
                    attributes);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  g_free (title);
  g_free (role);

  gtk_window_set_default_size (GTK_WINDOW (dialog),
                               250,
                               500);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           METAINFO_RESPONSE_SAVE,
                                           GTK_RESPONSE_OK,
                                           -1);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  attributes_vbox = GTK_WIDGET (gtk_builder_get_object (builder,
                                                      "metainfo-vbox"));

  gtk_container_set_border_width (GTK_CONTAINER (attributes_vbox), 2);
  gtk_box_pack_start (GTK_BOX (content_area), attributes_vbox, TRUE, TRUE, 0);

  label_header = GTK_WIDGET (gtk_builder_get_object (builder, "label-header"));
  gimp_label_set_attributes (GTK_LABEL (label_header),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_label_set_text (GTK_LABEL (label_header), header);

  label_info = GTK_WIDGET (gtk_builder_get_object (builder, "label-info"));
  gimp_label_set_attributes (GTK_LABEL (label_info),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_label_set_text (GTK_LABEL (label_info), fname);

  g_free (header);
  g_free (fname);

  if (pixbuf)
    {
      GtkWidget *image;

      thumb_box = GTK_WIDGET (gtk_builder_get_object (builder, "thumb-box"));

      if (thumb_box)
        {
          image = gtk_image_new_from_pixbuf (pixbuf);
          gtk_box_pack_end (GTK_BOX (thumb_box), image, FALSE, FALSE, 0);
          gtk_widget_show (image);
        }
    }

  page_description_read_from_attributes    (attributes,
                                            builder);
  page_artwork_read_from_attributes        (attributes,
                                            builder);
  page_administration_read_from_attributes (attributes,
                                            builder);
  page_rights_read_from_attributes         (attributes,
                                            builder);

  gtk_widget_show (GTK_WIDGET (dialog));

  gtk_main ();
  
  return TRUE;
}
