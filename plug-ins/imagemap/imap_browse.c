/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2002 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#include <gtk/gtk.h>

#include "imap_browse.h"
#include "imap_stock.h"

static GtkTargetEntry target_table[] = {
   {"STRING", 0, 1 },
   {"text/plain", 0, 2 }
};

static void
select_cb (GtkWidget      *dialog,
           gint            response_id,
           BrowseWidget_t *browse)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar       *p;
      const gchar *file;

      file = gtk_file_selection_get_filename (GTK_FILE_SELECTION (dialog));

      p = (browse->filter) ? browse->filter (file, browse->filter_data) : (gchar *) file;

      gtk_entry_set_text (GTK_ENTRY (browse->file), p);

      if (browse->filter)
        g_free (p);
    }

  gtk_widget_hide (dialog);
  gtk_widget_grab_focus (browse->file);
}

static void
browse_cb (GtkWidget      *widget,
           BrowseWidget_t *browse)
{
   if (! browse->file_selection)
     {
       GtkWidget *dialog;

       dialog = browse->file_selection = gtk_file_selection_new (browse->name);

       gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                     GTK_WINDOW (gtk_widget_get_toplevel (widget)));

       g_signal_connect (dialog, "delete_event",
                         G_CALLBACK (gtk_true),
                         NULL);
       g_signal_connect (dialog, "response",
                         G_CALLBACK (select_cb),
                         browse);
     }

   gtk_window_present (GTK_WINDOW (browse->file_selection));
}

static void 
handle_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, 
	    GtkSelectionData *data, guint info, guint time)
{
   gboolean success;
   if (data->length >= 0 && data->format == 8) {
      gtk_entry_set_text(GTK_ENTRY(widget), data->data);
      success = TRUE;
   } else {
      success = FALSE;
   }
   gtk_drag_finish(context, success, FALSE, time);
}

BrowseWidget_t*
browse_widget_new(const gchar *name)
{
   BrowseWidget_t *browse = g_new(BrowseWidget_t, 1);
   GtkWidget *button;
   GtkWidget *icon;

   browse->file_selection = NULL;
   browse->name = name;
   browse->filter = NULL;

   browse->hbox = gtk_hbox_new(FALSE, 1);
   gtk_widget_show(browse->hbox);

   browse->file = gtk_entry_new();
   gtk_box_pack_start(GTK_BOX(browse->hbox), browse->file, TRUE, TRUE, 0);
   gtk_drag_dest_set(browse->file, GTK_DEST_DEFAULT_ALL, target_table,
		     2, GDK_ACTION_COPY);
   g_signal_connect(browse->file, "drag_data_received",
		    G_CALLBACK(handle_drop), NULL);

   gtk_widget_show(browse->file);

   browse->button = button = gtk_button_new();
   icon = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
   gtk_container_add(GTK_CONTAINER(button), icon);
   gtk_widget_show(icon);

   gtk_box_pack_end(GTK_BOX(browse->hbox), button, FALSE, FALSE, 0);
   g_signal_connect(button, "clicked", 
		    G_CALLBACK(browse_cb), (gpointer) browse);
   gtk_widget_show(button);

   return browse;
}

void 
browse_widget_set_filename(BrowseWidget_t *browse, const gchar *filename)
{
   gtk_entry_set_text(GTK_ENTRY(browse->file), filename);
}

void 
browse_widget_set_filter(BrowseWidget_t *browse, BrowseFilter_t filter, 
			 gpointer data)
{
   browse->filter = filter;
   browse->filter_data = data;
}
