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

#include <stdio.h>
#include <string.h>

#include "imap_browse.h"
#include "imap_cmd_edit_object.h"
#include "imap_default_dialog.h"
#include "imap_edit_area_info.h"
#include "imap_main.h"
#include "imap_table.h"

static gint callback_lock;


static const gchar*
relative_filter(const char *name, gpointer data)
{
   AreaInfoDialog_t *param = (AreaInfoDialog_t*) data;
   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(param->relative_link)))
      return g_basename(name);
   return name;
}

static void
url_changed(GtkWidget *widget, gpointer data)
{
   AreaInfoDialog_t *param = (AreaInfoDialog_t*) data;
   gchar *url = gtk_entry_get_text(GTK_ENTRY(param->url));
   GtkWidget *button;

   if (!g_strncasecmp(url, "http://", sizeof("http://") - 1))
      button = param->web_site;
   else if (!g_strncasecmp(url, "ftp://", sizeof("ftp://") - 1))
      button = param->ftp_site;
   else if (!g_strncasecmp(url, "gopher://", sizeof("gopher://") - 1))
      button = param->gopher;
   else if (!g_strncasecmp(url, "file:/", sizeof("file:/") - 1))
      button = param->file;
   else if (!g_strncasecmp(url, "wais://", sizeof("wais://") - 1))
      button = param->wais;
   else if (!g_strncasecmp(url, "telnet://", sizeof("telnet://") - 1))
      button = param->telnet;
   else if (!g_strncasecmp(url, "mailto:", sizeof("mailto:") - 1))
      button = param->email;
   else
      button = param->other;

   callback_lock = TRUE;
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
}

static void
set_url(GtkWidget *widget, AreaInfoDialog_t *param, const gchar *prefix)
{
   if (callback_lock) {
      callback_lock = FALSE;
   } else {
      if (GTK_WIDGET_STATE(widget) & GTK_STATE_SELECTED) {
	 char *p;
	 gchar *url = g_strdup(gtk_entry_get_text(GTK_ENTRY(param->url)));
	 
	 p = strstr(url, "//");    /* 'http://' */
	 if (p) {
	    p += 2;
	 } else {
	    p = strchr(url, ':');	/* 'mailto:' */
	    if (p) {
	       p++;
	       if (*p == '/')	/* 'file:/' */
		  p++;
	    } else {
	       p = url;
	    }
	 }
	 gtk_entry_set_text(GTK_ENTRY(param->url), prefix);
	 gtk_entry_append_text(GTK_ENTRY(param->url), p);
	 g_free(url);
      }
   }
   gtk_widget_grab_focus(param->url);
}

static void
select_web_cb(GtkWidget *widget, AreaInfoDialog_t *param)
{
   set_url(widget, param, "http://");
}

static void
select_ftp_cb(GtkWidget *widget, AreaInfoDialog_t *param)
{
   set_url(widget, param, "ftp://");
}

static void
select_gopher_cb(GtkWidget *widget, AreaInfoDialog_t *param)
{
   set_url(widget, param, "gopher://");
}

static void
select_other_cb(GtkWidget *widget, AreaInfoDialog_t *param)
{
   set_url(widget, param, "");
}

static void
select_file_cb(GtkWidget *widget, AreaInfoDialog_t *param)
{
   set_url(widget, param, "file:/");
}

static void
select_wais_cb(GtkWidget *widget, AreaInfoDialog_t *param)
{
   set_url(widget, param, "wais://");
}

static void
select_telnet_cb(GtkWidget *widget, AreaInfoDialog_t *param)
{
   set_url(widget, param, "telnet://");
}

static void
select_email_cb(GtkWidget *widget, AreaInfoDialog_t *param)
{
   set_url(widget, param, "mailto:");
}

static void
create_link_tab(AreaInfoDialog_t *dialog, GtkWidget *notebook)
{
   BrowseWidget_t *browse;
   GtkWidget *table, *label;
   GtkWidget *subtable, *frame;
   GSList    *group;

   table = gtk_table_new(11, 1, FALSE);
   gtk_container_set_border_width(GTK_CONTAINER(table), 10);
   gtk_table_set_row_spacings(GTK_TABLE(table), 10);
   gtk_widget_show(table);
   
   frame = gtk_frame_new("Link Type");
   gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 2, 0, 1);
   gtk_widget_show(frame);

   subtable = gtk_table_new(2, 4, FALSE);
   gtk_container_add(GTK_CONTAINER(frame), subtable);
   gtk_widget_show(subtable);

   dialog->web_site = create_radio_button_in_table(subtable, NULL, 0, 0, 
						   "Web Site");
   gtk_signal_connect(GTK_OBJECT(dialog->web_site), "toggled", 
		      (GtkSignalFunc) select_web_cb, (gpointer) dialog);
   group = gtk_radio_button_group(GTK_RADIO_BUTTON(dialog->web_site));

   dialog->ftp_site = create_radio_button_in_table(subtable, group, 0, 1, 
						   "Ftp Site");
   gtk_signal_connect(GTK_OBJECT(dialog->ftp_site), "toggled", 
		      (GtkSignalFunc) select_ftp_cb, (gpointer) dialog);
   group = gtk_radio_button_group(GTK_RADIO_BUTTON(dialog->ftp_site));

   dialog->gopher = create_radio_button_in_table(subtable, group, 0, 2, 
						 "Gopher");
   gtk_signal_connect(GTK_OBJECT(dialog->gopher), "toggled", 
		      (GtkSignalFunc) select_gopher_cb, (gpointer) dialog);
   group = gtk_radio_button_group(GTK_RADIO_BUTTON(dialog->gopher));

   dialog->other = create_radio_button_in_table(subtable, group, 0, 3, 
						"Other");
   gtk_signal_connect(GTK_OBJECT(dialog->other), "toggled", 
		      (GtkSignalFunc) select_other_cb, (gpointer) dialog);
   group = gtk_radio_button_group(GTK_RADIO_BUTTON(dialog->other));

   dialog->file = create_radio_button_in_table(subtable, group, 1, 0, "File");
   gtk_signal_connect(GTK_OBJECT(dialog->file), "toggled", 
		      (GtkSignalFunc) select_file_cb, (gpointer) dialog);
   group = gtk_radio_button_group(GTK_RADIO_BUTTON(dialog->file));

   dialog->wais = create_radio_button_in_table(subtable, group, 1, 1, "WAIS");
   gtk_signal_connect(GTK_OBJECT(dialog->wais), "toggled", 
		      (GtkSignalFunc) select_wais_cb, (gpointer) dialog);
   group = gtk_radio_button_group(GTK_RADIO_BUTTON(dialog->wais));

   dialog->telnet = create_radio_button_in_table(subtable, group, 1, 2, 
						 "Telnet");
   gtk_signal_connect(GTK_OBJECT(dialog->telnet), "toggled", 
		      (GtkSignalFunc) select_telnet_cb, (gpointer) dialog);
   group = gtk_radio_button_group(GTK_RADIO_BUTTON(dialog->telnet));

   dialog->email = create_radio_button_in_table(subtable, group, 1, 3, 
						"e-mail");
   gtk_signal_connect(GTK_OBJECT(dialog->email), "toggled", 
		      (GtkSignalFunc) select_email_cb, (gpointer) dialog);

   create_label_in_table(
      table, 2, 0,
      "URL to activate when this area is clicked: (required)");

   browse = browse_widget_new("Select HTML file");
   browse_widget_set_filter(browse, relative_filter, (gpointer) dialog);
   gtk_table_attach_defaults(GTK_TABLE(table), browse->hbox, 0, 1, 3, 4);
   dialog->url = browse->file;
   gtk_signal_connect(GTK_OBJECT(dialog->url), "changed", 
		      GTK_SIGNAL_FUNC(url_changed), dialog);

   dialog->relative_link = create_check_button_in_table(table, 4, 0, 
							"Relative link");
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->relative_link), 
				TRUE);

   create_label_in_table(
      table, 6, 0,
      "Target frame name/ID: (optional - used for FRAMES only)");
   dialog->target = create_entry_in_table(table, 7, 0);

   create_label_in_table(table, 9, 0,
			 "Comment about this area: (optional)");
   dialog->comment = create_entry_in_table(table, 10, 0);

   label = gtk_label_new("Link");
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);
}

static void
create_java_script_tab(AreaInfoDialog_t *dialog, GtkWidget *notebook)
{
   GtkWidget *vbox, *table, *label;
   
   vbox = gtk_vbox_new(FALSE, 1);
   gtk_widget_show(vbox);

   table = gtk_table_new(11, 1, FALSE);
   gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(table), 10);
   gtk_table_set_row_spacings(GTK_TABLE(table), 10);
   gtk_widget_show(table);
   
   create_label_in_table(table, 0, 0, "onMouseover:");
   dialog->mouse_over = create_entry_in_table(table, 1, 0);

   create_label_in_table(table, 3, 0, "onMouseout:");
   dialog->mouse_out = create_entry_in_table(table, 4, 0);

   create_label_in_table(table, 6, 0, "onFocus (HTML 4.0):");
   dialog->focus = create_entry_in_table(table, 7, 0);

   create_label_in_table(table, 9, 0, "onBlur (HTML 4.0):");
   dialog->blur = create_entry_in_table(table, 10, 0);

   label = gtk_label_new("JavaScript");
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
}

static void
edit_area_ok_cb(gpointer data)
{
   AreaInfoDialog_t *param = (AreaInfoDialog_t*) data;
   Object_t *obj = param->obj;
   gint old_x, old_y, old_width, old_height;
   gint new_x, new_y, new_width, new_height;

   /* Fix me: nasty hack */
   if (param->add)
      command_list_add(edit_object_command_new(obj));

   object_get_dimensions(obj, &old_x, &old_y, &old_width, &old_height);

   object_set_url(obj, gtk_entry_get_text(GTK_ENTRY(param->url)));
   object_set_target(obj, gtk_entry_get_text(GTK_ENTRY(param->target)));
   object_set_comment(obj, gtk_entry_get_text(GTK_ENTRY(param->comment)));
   object_set_mouse_over(obj, 
			 gtk_entry_get_text(GTK_ENTRY(param->mouse_over)));
   object_set_mouse_out(obj, gtk_entry_get_text(GTK_ENTRY(param->mouse_out)));
   object_set_focus(obj, gtk_entry_get_text(GTK_ENTRY(param->focus)));
   object_set_blur(obj, gtk_entry_get_text(GTK_ENTRY(param->blur)));
   object_update(obj, param->infotab);
   update_shape(obj);
   object_unlock(obj);

   object_get_dimensions(obj, &new_x, &new_y, &new_width, &new_height);
   if (new_x != old_x || new_y != old_y || new_width != old_width ||
       new_height != old_height)
      redraw_preview();
}

static void
edit_area_cancel_cb(gpointer data)
{
   Object_t *obj = ((AreaInfoDialog_t*) data)->obj;
   object_unlock(obj);
}

static void
switch_page(GtkWidget *widget, GtkNotebookPage *page, gint page_num, 
	    gpointer data)
{
   AreaInfoDialog_t *param = (AreaInfoDialog_t*) data;
   if (page_num == 0) {
      gtk_widget_grab_focus(param->url);
   } else if (page_num == 1) {
      Object_t *obj = param->obj;
      obj->class->set_initial_focus(obj, param->infotab);
   } else {
      gtk_widget_grab_focus(param->mouse_over);
   }
}

AreaInfoDialog_t*
create_edit_area_info_dialog(Object_t *obj)
{
   AreaInfoDialog_t *data = g_new(AreaInfoDialog_t, 1);
   GtkWidget *notebook;

   data->obj = obj;
   data->browse = NULL;
   data->dialog = make_default_dialog("Area Settings");
   default_dialog_set_ok_cb(data->dialog, edit_area_ok_cb, data);
   default_dialog_set_cancel_cb(data->dialog, edit_area_cancel_cb, data);

   data->notebook = notebook = gtk_notebook_new();
   gtk_container_set_border_width(GTK_CONTAINER(notebook), 10);
   gtk_signal_connect_after(GTK_OBJECT(notebook), "switch_page", 
		      GTK_SIGNAL_FUNC(switch_page), (gpointer) data);

   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(data->dialog->dialog)->vbox), 
		      notebook, TRUE, TRUE, 10);
   create_link_tab(data, notebook);
   data->infotab = obj->class->create_info_tab(notebook);
   create_java_script_tab(data, notebook);
   gtk_widget_show(notebook);

   return data;
}

void
edit_area_info_dialog_show(AreaInfoDialog_t *dialog, Object_t *obj, 
			   gboolean add)
{
   char title[64];

   object_unlock(dialog->obj);
   object_lock(obj);
   dialog->obj = obj;
   dialog->add = add;
   obj->class->fill_info_tab(obj, dialog->infotab);
   gtk_entry_set_text(GTK_ENTRY(dialog->url), obj->url);
   gtk_entry_set_text(GTK_ENTRY(dialog->target), obj->target);
   gtk_entry_set_text(GTK_ENTRY(dialog->comment), obj->comment);
   gtk_entry_set_text(GTK_ENTRY(dialog->mouse_over), obj->mouse_over);
   gtk_entry_set_text(GTK_ENTRY(dialog->mouse_out), obj->mouse_out);
   gtk_entry_set_text(GTK_ENTRY(dialog->focus), obj->focus);
   gtk_entry_set_text(GTK_ENTRY(dialog->blur), obj->blur);
   gtk_widget_grab_focus(dialog->url);

   sprintf(title, "Area #%d Settings", object_get_position_in_list(obj) + 1);
   default_dialog_set_title(dialog->dialog, title);
   default_dialog_show(dialog->dialog);
}


