/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2004 Maurits Rijk  m.rijk@chello.nl
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
 *
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "imap_file.h"
#include "imap_main.h"
#include "imap_misc.h"

#include "libgimp/stdplugins-intl.h"


static void
open_cb (GtkFileSelection *fs,
         gint              response_id,
         gpointer          data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *filename;

      filename = gtk_file_selection_get_filename (fs);

      if (! g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        {
          do_file_error_dialog (_("Error opening file"), filename);
          return;
        }

      load (filename);
    }

  gtk_widget_hide (GTK_WIDGET (fs));
}

void
do_file_open_dialog (void)
{
  static GtkWidget *dialog;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Load Imagemap"),
                                     NULL,
                                     GTK_FILE_CHOOSER_ACTION_OPEN,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OPEN,   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dialog);
      g_signal_connect (dialog, "response",
                        G_CALLBACK (open_cb),
                        dialog);
    }  
  gtk_window_present (GTK_WINDOW (dialog));
}

static void
really_overwrite_cb (GtkMessageDialog *dialog,
		     gint              response_id,
		     gpointer          data)
{
  if (response_id == GTK_RESPONSE_YES)
    {
      save_as (gtk_file_selection_get_filename (GTK_FILE_SELECTION (data)));
    }
  gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
do_file_exists_dialog (gpointer data)
{
  static GtkWidget *dialog;

  if (!dialog)
    {
      dialog = gtk_message_dialog_new_with_markup 
	(GTK_WINDOW(get_dialog()), 
	 GTK_DIALOG_DESTROY_WITH_PARENT,
	 GTK_MESSAGE_QUESTION,
	 GTK_BUTTONS_YES_NO,
	 _("<span weight=\"bold\" size=\"larger\">File already exists.</span>\n\n"
	   "Do you really want to overwrite?"));
      g_signal_connect (dialog, "delete_event",
			G_CALLBACK (gtk_true),
			NULL);
      g_signal_connect (dialog, "response",
			G_CALLBACK (really_overwrite_cb),
			data);
    }
  gtk_widget_show (dialog);
}

static void
save_cb (GtkFileSelection *fs,
         gint              response_id,
         gpointer          data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *filename;
      
      filename = gtk_file_selection_get_filename (fs);
      
      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        {
          do_file_exists_dialog (fs);
          return;
        }

      save_as (filename);
    }

  gtk_widget_hide (GTK_WIDGET (fs));
}

void
do_file_save_as_dialog (void)
{
  static GtkWidget *dialog;

  if (! dialog)
    {
      dialog =
        gtk_file_chooser_dialog_new (_("Save Imagemap"),
                                     NULL,
                                     GTK_FILE_CHOOSER_ACTION_OPEN,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OPEN,   GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dialog);
      g_signal_connect (dialog, "response",
                        G_CALLBACK (save_cb),
                        dialog);
    }  
  gtk_window_present (GTK_WINDOW (dialog));
}

void
do_file_error_dialog(const char *error, const char *filename)
{
  static Alert_t *alert;
  
  if (!alert)
    alert = create_alert (GTK_STOCK_DIALOG_ERROR);
  
  alert_set_text(alert, error, gimp_filename_to_utf8 (filename));
  
  alert_set_text (alert, error, filename);
  
  default_dialog_show (alert->dialog);
}
