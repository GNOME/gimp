/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#include "imap_browse.h"
#include "imap_main.h"
#include "imap_settings.h"
#include "imap_string.h"
#include "imap_table.h"


static MapFormat_t _map_format = CSIM;

static void
settings_ok_cb(gpointer data)
{
   SettingsDialog_t *param = (SettingsDialog_t*) data;
   MapInfo_t *info = get_map_info();

   g_strreplace(&info->image_name, gtk_entry_get_text(
      GTK_ENTRY(param->imagename->file)));
   g_strreplace(&info->title, gtk_entry_get_text(GTK_ENTRY(param->title)));
   g_strreplace(&info->author, gtk_entry_get_text(GTK_ENTRY(param->author)));
   g_strreplace(&info->default_url, 
		gtk_entry_get_text(GTK_ENTRY(param->default_url)));
   g_strreplace(&info->description, 
		gtk_editable_get_chars(GTK_EDITABLE(param->description), 
				       0, -1));
   info->map_format = _map_format;
}

static void
type_toggled_cb(GtkWidget *widget, gpointer data)
{
   if (GTK_WIDGET_STATE(widget) & GTK_STATE_SELECTED)
      _map_format = (MapFormat_t) data;
}

static SettingsDialog_t*
make_settings_dialog()
{
   SettingsDialog_t *data = g_new(SettingsDialog_t, 1);
   GtkWidget *table, *vscrollbar, *frame, *hbox;
   DefaultDialog_t *dialog;
   GSList    *group;

   dialog = data->dialog = make_default_dialog("Settings for this Mapfile");
   default_dialog_set_ok_cb(dialog, settings_ok_cb, (gpointer) data);

   table = gtk_table_new(9, 3, FALSE);
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog->dialog)->vbox), 
		     table);
   gtk_table_set_row_spacings(GTK_TABLE(table), 10);
   gtk_table_set_col_spacings(GTK_TABLE(table), 10);
   gtk_container_set_border_width(GTK_CONTAINER(table), 10);
   gtk_widget_show(table);

   create_label_in_table(table, 0, 0, "Filename:");
   data->filename = create_label_in_table(table, 0, 1, "");

   create_label_in_table(table, 1, 0, "Image name:");
   data->imagename = browse_widget_new("Select Image File");
   gtk_table_attach_defaults(GTK_TABLE(table), data->imagename->hbox, 1, 2, 
			     1, 2);

   create_label_in_table(table, 2, 0, "Title:");
   data->title = create_entry_in_table(table, 2, 1);
   create_label_in_table(table, 3, 0, "Author:");
   data->author = create_entry_in_table(table, 3, 1);
   create_label_in_table(table, 4, 0, "Default URL:");
   data->default_url = create_entry_in_table(table, 4, 1);
   create_label_in_table(table, 5, 0, "Description:");

   data->description = gtk_text_new(NULL, NULL);
   gtk_text_set_editable(GTK_TEXT(data->description), TRUE);
   gtk_table_attach(GTK_TABLE(table), data->description, 1, 2, 5, 8,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
   gtk_widget_show(data->description);

   /* Add a vertical scrollbar to the GtkText widget */
   vscrollbar = gtk_vscrollbar_new(GTK_TEXT(data->description)->vadj);
   gtk_table_attach(GTK_TABLE(table), vscrollbar, 2, 3, 5, 8,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
   gtk_widget_show(vscrollbar);

   frame = gtk_frame_new("Map file format");
   gtk_widget_show(frame);
   gtk_table_attach_defaults(GTK_TABLE(table), frame, 1, 2, 9, 10);
   hbox = gtk_hbox_new(FALSE, 1);
   gtk_container_add(GTK_CONTAINER(frame), hbox);
   gtk_widget_show(hbox);

   data->ncsa = gtk_radio_button_new_with_label(NULL, "NCSA");
   gtk_signal_connect(GTK_OBJECT(data->ncsa), "toggled", 
		      (GtkSignalFunc) type_toggled_cb, (gpointer) NCSA);
   gtk_box_pack_start(GTK_BOX(hbox), data->ncsa, TRUE, TRUE, 10);
   gtk_widget_show(data->ncsa);

   group = gtk_radio_button_group(GTK_RADIO_BUTTON(data->ncsa));
   data->cern = gtk_radio_button_new_with_label(group, "CERN");
   gtk_signal_connect(GTK_OBJECT(data->cern), "toggled", 
		      (GtkSignalFunc) type_toggled_cb, (gpointer) CERN);
   gtk_box_pack_start(GTK_BOX(hbox), data->cern, TRUE, TRUE, 10);
   gtk_widget_show(data->cern);

   group = gtk_radio_button_group(GTK_RADIO_BUTTON(data->cern));
   data->csim = gtk_radio_button_new_with_label(group, "CSIM");
   gtk_signal_connect(GTK_OBJECT(data->csim), "toggled", 
		      (GtkSignalFunc) type_toggled_cb, (gpointer) CSIM);
   gtk_box_pack_start(GTK_BOX(hbox), data->csim, TRUE, TRUE, 10);
   gtk_widget_show(data->csim);

   return data;
}

void
do_settings_dialog(void)
{
   static SettingsDialog_t *dialog;
   const char *filename = get_filename();
   MapInfo_t *info = get_map_info();

   if (!dialog)
      dialog = make_settings_dialog();

   gtk_label_set_text(GTK_LABEL(dialog->filename), 
		      (filename) ? filename : "<Untitled>");
   browse_widget_set_filename(dialog->imagename, info->image_name);
   gtk_entry_set_text(GTK_ENTRY(dialog->title), info->title);
   gtk_entry_set_text(GTK_ENTRY(dialog->author), info->author);
   gtk_entry_set_text(GTK_ENTRY(dialog->default_url), info->default_url);

   if (info->map_format == NCSA)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->ncsa), TRUE);
   else if (info->map_format == CERN)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->cern), TRUE);
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->csim), TRUE);

   gtk_widget_grab_focus(dialog->imagename->file);
   default_dialog_show(dialog->dialog);

   gtk_editable_delete_text(GTK_EDITABLE(dialog->description), 0, -1);
   gtk_text_insert(GTK_TEXT(dialog->description), NULL, NULL, NULL,
		   info->description, -1);
}
