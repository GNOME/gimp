/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2003 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#include "imap_default_dialog.h"
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
       dialog = gtk_file_selection_new (_("Load Imagemap"));

       g_signal_connect (dialog, "delete_event",
                         G_CALLBACK (gtk_true),
                         NULL);
       g_signal_connect (dialog, "response",
                         G_CALLBACK (open_cb),
                         NULL);
     }

   gtk_window_present (GTK_WINDOW (dialog));
}

static void
really_overwrite(gpointer data)
{
   gtk_widget_hide((GtkWidget*) data);
   save_as(gtk_file_selection_get_filename(GTK_FILE_SELECTION(data)));
}

static void
do_file_exists_dialog(gpointer data)
{
   static DefaultDialog_t *dialog;

   if (!dialog) {
      dialog = make_default_dialog(_("File exists!"));
      default_dialog_hide_apply_button(dialog);
      default_dialog_set_ok_cb(dialog, really_overwrite, data);
      default_dialog_set_label(
	 dialog,
	 _("File already exists.\n"
	 "  Do you really want to overwrite?  "));
   }
   default_dialog_show(dialog);
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
       dialog = gtk_file_selection_new (_("Save Imagemap"));

       g_signal_connect (dialog, "delete_event",
                         G_CALLBACK (gtk_true),
                         NULL);
       g_signal_connect (dialog, "response",
                         G_CALLBACK (save_cb),
                         NULL);
     }

   gtk_window_present (GTK_WINDOW (dialog));
}

void
do_file_error_dialog(const char *error, const char *filename)
{
   static Alert_t *alert;

   if (!alert)
      alert = create_alert(GTK_STOCK_DIALOG_ERROR);

   alert_set_text(alert, error, gimp_filename_to_utf8 (filename));

   default_dialog_show(alert->dialog);
}
