/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2004 Maurits Rijk  m.rijk@chello.nl
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

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "imap_browse.h"

#include "libgimp/stdplugins-intl.h"


static const GtkTargetEntry target_table[] =
{
  {"STRING",     0, 1 },
  {"text/plain", 0, 2 }
};


static void
select_cb (GtkWidget      *dialog,
           gint            response_id,
           BrowseWidget_t *browse)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *p;
      gchar *file;

      file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      p = (browse->filter ?
           browse->filter (file, browse->filter_data) : file);

      gtk_entry_set_text (GTK_ENTRY (browse->file), p);

      if (browse->filter)
        g_free (p);

      g_free (file);
    }

  gtk_widget_hide (dialog);
  gtk_widget_grab_focus (browse->file);
}

static void
browse_cb (GtkWidget      *widget,
           BrowseWidget_t *browse)
{
  if (!browse->file_chooser)
    {
      GtkWidget *dialog;

      dialog = browse->file_chooser =
        gtk_file_chooser_dialog_new (browse->name,
                                     GTK_WINDOW (gtk_widget_get_toplevel (widget)),
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
                        G_CALLBACK (select_cb),
                        browse);
    }

  gtk_window_present (GTK_WINDOW (browse->file_chooser));
}

static void
handle_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
            GtkSelectionData *data, guint info, guint time)
{
   gboolean success = FALSE;

   if (gtk_selection_data_get_length (data) >= 0 &&
       gtk_selection_data_get_format (data) == 8)
     {
       const gchar *text = (const gchar *) gtk_selection_data_get_data (data);

       if (g_utf8_validate (text, -1, NULL))
         {
           gtk_entry_set_text (GTK_ENTRY (widget), text);
           success = TRUE;
         }
     }

   gtk_drag_finish(context, success, FALSE, time);
}

BrowseWidget_t*
browse_widget_new (const gchar *name)
{
   BrowseWidget_t *browse = g_new(BrowseWidget_t, 1);
   GtkWidget *button;
   GtkWidget *icon;

   browse->file_chooser = NULL;
   browse->name = name;
   browse->filter = NULL;

   browse->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
   gtk_widget_show (browse->hbox);

   browse->file = gtk_entry_new ();
   gtk_box_pack_start (GTK_BOX(browse->hbox), browse->file, TRUE, TRUE, 0);
   gtk_drag_dest_set (browse->file, GTK_DEST_DEFAULT_ALL, target_table,
                      2, GDK_ACTION_COPY);
   g_signal_connect (browse->file, "drag-data-received",
                     G_CALLBACK(handle_drop), NULL);

   gtk_widget_show (browse->file);

   browse->button = button = gtk_button_new ();
   icon = gtk_image_new_from_icon_name (GIMP_ICON_DOCUMENT_OPEN,
                                        GTK_ICON_SIZE_BUTTON);
   gtk_container_add (GTK_CONTAINER (button), icon);
   gtk_widget_show (icon);

   gtk_box_pack_end(GTK_BOX (browse->hbox), button, FALSE, FALSE, 0);
   g_signal_connect (button, "clicked",
                     G_CALLBACK(browse_cb), (gpointer) browse);
   gtk_widget_show (button);

   return browse;
}

void
browse_widget_set_filename(BrowseWidget_t *browse, const gchar *filename)
{
   gtk_entry_set_text (GTK_ENTRY (browse->file), filename);
}

void
browse_widget_set_filter(BrowseWidget_t *browse, BrowseFilter_t filter,
                         gpointer data)
{
   browse->filter = filter;
   browse->filter_data = data;
}
