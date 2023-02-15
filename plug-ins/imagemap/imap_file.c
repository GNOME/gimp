/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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
 *
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "imap_file.h"
#include "imap_main.h"

#include "libgimp/stdplugins-intl.h"


static void
open_cb (GtkWidget *dialog,
         gint       response_id,
         gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *filename;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (! g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        {
          do_file_error_dialog (_("Error opening file"), filename);
          g_free (filename);
          return;
        }

      load (filename, data);
      g_free (filename);
    }

  gtk_widget_hide (dialog);
}

void
do_file_open_dialog (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  static GtkWidget *dialog;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Load Image Map"),
                                     NULL,
                                     GTK_FILE_CHOOSER_ACTION_OPEN,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Open"),   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dialog);
      g_signal_connect (dialog, "response",
                        G_CALLBACK (open_cb),
                        user_data);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
save_cb (GtkWidget *dialog,
         gint       response_id,
         gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *filename;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      save_as (filename);
      g_free (filename);
    }

  gtk_widget_hide (dialog);
}

void
do_file_save_as_dialog (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
  static GtkWidget *dialog;

  if (! dialog)
    {
      gchar *filename;

      dialog = gtk_file_chooser_dialog_new (_("Save Image Map"),
                                            NULL,
                                            GTK_FILE_CHOOSER_ACTION_SAVE,

                                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                                            _("_Save"),   GTK_RESPONSE_OK,

                                            NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                      TRUE);

      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dialog);
      g_signal_connect (dialog, "response",
                        G_CALLBACK (save_cb),
                        dialog);

      /*  Suggest a filename based on the image name.
       *  The image name is in UTF-8 encoding.
       */
      filename = g_strconcat (get_image_name(), ".map", NULL);

      if (filename)
        {
          gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
                                             filename);
          g_free (filename);
        }
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
do_file_error_dialog (const char *error,
                      const char *filename)
{
   GtkWidget *dialog;

   dialog = gtk_message_dialog_new_with_markup
     (NULL,
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_ERROR,
      GTK_BUTTONS_CLOSE,
      "<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
      error,
      gimp_filename_to_utf8 (filename));

   g_signal_connect_swapped (dialog, "response",
                             G_CALLBACK (gtk_widget_destroy),
                             dialog);
   gtk_dialog_run (GTK_DIALOG (dialog));
}
