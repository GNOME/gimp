/* The GIMP -- an image manipulation program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "widgets/gimphelp-ids.h"

#include "file-open-location-dialog.h"

#include "gimp-intl.h"


static void  file_open_location_response (GtkWidget *dialog,
                                          gint       response_id,
                                          Gimp      *gimp);


/*  public functions  */

void
file_open_location_dialog_show (Gimp      *gimp,
                                GtkWidget *parent)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *entry;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (parent == NULL || GTK_IS_WIDGET (parent));

  dialog = gimp_dialog_new (_("Open Location"),
                            "gimp-file-open-location",
                            parent, 0,
                            gimp_standard_help_func,
                            GIMP_HELP_FILE_OPEN_LOCATION,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OPEN,   GTK_RESPONSE_OK,

                            NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_open_location_response),
                    gimp);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Enter location (URI):"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  g_object_set_data (G_OBJECT (dialog), "location-entry", entry);

  gtk_widget_show (dialog);
}

static void
file_open_location_response (GtkWidget *dialog,
                             gint       response_id,
                             Gimp      *gimp)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GtkWidget   *entry;
      const gchar *text = NULL;

      gtk_widget_set_sensitive (dialog, FALSE);

      entry = g_object_get_data (G_OBJECT (dialog), "location-entry");

      text = gtk_entry_get_text (GTK_ENTRY (entry));

      if (text && strlen (text))
        {
          GimpImage         *image;
          gchar             *uri;
          GError            *error = NULL;
          GimpPDBStatusType  status;

          uri = file_utils_filename_to_uri (gimp->load_procs, text, NULL);

          image = file_open_with_proc_and_display (gimp,
                                                   gimp_get_user_context (gimp),
                                                   uri, text, NULL,
                                                   &status, &error);

          if (image == NULL && status != GIMP_PDB_CANCEL)
            {
              gchar *filename = file_utils_uri_to_utf8_filename (uri);

              g_message (_("Opening '%s' failed:\n\n%s"),
                         filename, error->message);
              g_clear_error (&error);

              g_free (filename);
            }

          g_free (uri);
        }
    }

  gtk_widget_destroy (dialog);
}
