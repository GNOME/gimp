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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "imap_browse.h"
#include "imap_main.h"
#include "imap_settings.h"
#include "imap_string.h"
#include "imap_ui_grid.h"

#include "libgimp/stdplugins-intl.h"

typedef struct {
  DefaultDialog_t *dialog;
  BrowseWidget_t *imagename;
  GtkWidget     *filename;
  GtkWidget     *title;
  GtkWidget     *author;
  GtkWidget     *default_url;
  GtkWidget     *ncsa;
  GtkWidget     *cern;
  GtkWidget     *csim;
  GtkTextBuffer *description;
} SettingsDialog_t;

static MapFormat_t _map_format = CSIM;

static void
settings_ok_cb(gpointer data)
{
   SettingsDialog_t *param = (SettingsDialog_t*) data;
   MapInfo_t *info = get_map_info();
   gchar *description;
   GtkTextIter start, end;

   g_strreplace(&info->image_name, gtk_entry_get_text(
      GTK_ENTRY(param->imagename->file)));
   g_strreplace(&info->title, gtk_entry_get_text(GTK_ENTRY(param->title)));
   g_strreplace(&info->author, gtk_entry_get_text(GTK_ENTRY(param->author)));
   g_strreplace(&info->default_url,
                gtk_entry_get_text(GTK_ENTRY(param->default_url)));
   gtk_text_buffer_get_bounds(param->description, &start, &end);
   description = gtk_text_buffer_get_text(param->description, &start, &end,
                                          FALSE);
   g_strreplace(&info->description, description);
   g_free(description);

   info->map_format = _map_format;
}

static void
type_toggled_cb(GtkWidget *widget, gpointer data)
{
  if (gtk_widget_get_state (widget) & GTK_STATE_SELECTED)
    _map_format = (MapFormat_t) data;
}

static SettingsDialog_t*
create_settings_dialog(void)
{
   SettingsDialog_t *data = g_new(SettingsDialog_t, 1);
   GtkWidget *grid, *view, *frame, *hbox, *label, *swin;
   DefaultDialog_t *dialog;

   dialog = data->dialog = make_default_dialog(_("Settings for this Mapfile"));
   default_dialog_set_ok_cb(dialog, settings_ok_cb, (gpointer) data);
   grid = default_dialog_add_grid (dialog);

   create_label_in_grid (grid, 0, 0, _("Filename:"));
   data->filename = create_label_in_grid (grid, 0, 1, "");

   create_label_in_grid (grid, 1, 0, _("Image name:"));
   data->imagename = browse_widget_new(_("Select Image File"));
   gtk_grid_attach (GTK_GRID (grid), data->imagename->hbox, 1, 1, 1, 1);

   label = create_label_in_grid (grid, 2, 0, _("_Title:"));
   data->title = create_entry_in_grid (grid, label, 2, 1);
   label = create_label_in_grid (grid, 3, 0, _("Aut_hor:"));
   data->author = create_entry_in_grid (grid, label, 3, 1);
   label = create_label_in_grid (grid, 4, 0, _("Default _URL:"));
   data->default_url = create_entry_in_grid (grid, label, 4, 1);
   label = create_label_in_grid (grid, 5, 0, _("_Description:"));

   data->description = gtk_text_buffer_new(NULL);

   view = gtk_text_view_new_with_buffer(data->description);
   gtk_widget_set_size_request(view, -1, 128);
   gtk_widget_show(view);

   gtk_label_set_mnemonic_widget (GTK_LABEL (label), view);

   swin = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin),
                                       GTK_SHADOW_IN);
   gtk_grid_attach (GTK_GRID (grid), swin, 1, 5, 1, 3);
                    // GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                    // GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
   gtk_widget_show(swin);
   gtk_container_add(GTK_CONTAINER(swin), view);

   frame = gimp_frame_new(_("Map File Format"));
   gtk_widget_show(frame);
   gtk_grid_attach (GTK_GRID (grid), frame, 0, 9, 2, 1);
   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
   gtk_container_add(GTK_CONTAINER(frame), hbox);
   gtk_widget_show(hbox);

   data->ncsa = gtk_radio_button_new_with_mnemonic_from_widget(NULL, "_NCSA");
   g_signal_connect(data->ncsa, "toggled",
                    G_CALLBACK(type_toggled_cb), (gpointer) NCSA);
   gtk_box_pack_start(GTK_BOX(hbox), data->ncsa, FALSE, FALSE, 0);
   gtk_widget_show(data->ncsa);

   data->cern = gtk_radio_button_new_with_mnemonic_from_widget(
      GTK_RADIO_BUTTON(data->ncsa), "C_ERN");
   g_signal_connect(data->cern, "toggled",
                    G_CALLBACK(type_toggled_cb), (gpointer) CERN);
   gtk_box_pack_start(GTK_BOX(hbox), data->cern, FALSE, FALSE, 0);
   gtk_widget_show(data->cern);

   data->csim = gtk_radio_button_new_with_mnemonic_from_widget(
      GTK_RADIO_BUTTON(data->cern), "C_SIM");
   g_signal_connect(data->csim, "toggled",
                    G_CALLBACK(type_toggled_cb), (gpointer) CSIM);
   gtk_box_pack_start(GTK_BOX(hbox), data->csim, FALSE, FALSE, 0);
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
      dialog = create_settings_dialog();

   if (!filename)
     filename = _("<Untitled>");

   gtk_label_set_text(GTK_LABEL(dialog->filename), filename);
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

   gtk_text_buffer_set_text (dialog->description, info->description, -1);
}
