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

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "imap_default_dialog.h"
#include "imap_file.h"
#include "imap_main.h"
#include "imap_table.h"

#include "libgimp/stdplugins-intl.h"


static void
open_cb(GtkWidget *widget, gpointer data)
{
   const gchar *filename;
   struct stat buf;
   int err;

   filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));
   err = stat(filename, &buf);
   if (!err && (buf.st_mode & S_IFREG)) {
      gtk_widget_hide((GtkWidget*) data);
      load(filename);
   } else {
      do_file_error_dialog(_("Error opening file"), filename);
   }
}

void
do_file_open_dialog(void)
{
   static GtkWidget *dialog;
   if (!dialog) {
      dialog = gtk_file_selection_new(_("Load Imagemap"));
      g_signal_connect_swapped(
	 G_OBJECT(GTK_FILE_SELECTION(dialog)->cancel_button),
	 "clicked", G_CALLBACK(gtk_widget_hide), dialog);
      g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(dialog)->ok_button),
                       "clicked", G_CALLBACK(open_cb), dialog);
      g_signal_connect(G_OBJECT(dialog), "delete_event",
		       G_CALLBACK(gtk_widget_hide), NULL);
   }
   gtk_widget_show(dialog);
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
save_cb(GtkWidget *widget, gpointer data)
{
   const gchar *filename;
   struct stat buf;
   int err;

   filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));
   err = stat(filename, &buf);
   if (err) {
      gtk_widget_hide((GtkWidget*) data);
      save_as(filename);
   } else {			/* File exists */
      if (buf.st_mode & S_IFREG) {
	 do_file_exists_dialog(data);
      } else {
				/* Fix me! */
      }
   }
}

void
do_file_save_as_dialog(void)
{
   static GtkWidget *dialog;
   if (!dialog) {
      dialog = gtk_file_selection_new(_("Save Imagemap"));
      g_signal_connect_swapped(
	 G_OBJECT(GTK_FILE_SELECTION(dialog)->cancel_button),
	 "clicked", G_CALLBACK(gtk_widget_hide), dialog);
      g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(dialog)->ok_button),
                       "clicked", G_CALLBACK(save_cb), dialog);
      g_signal_connect(G_OBJECT(dialog), "delete_event",
		       G_CALLBACK(gtk_widget_hide), NULL);
   }
   gtk_widget_show(dialog);
}

typedef struct {
   DefaultDialog_t *dialog;
   GtkWidget *label;
} Alert_t;

Alert_t*
create_alert(const gchar *stock_id)
{
   Alert_t *alert = g_new(Alert_t, 1);
   DefaultDialog_t *dialog;
   GtkWidget *hbox;
   GtkWidget *image;

   alert->dialog = dialog = make_default_dialog("");
   default_dialog_hide_apply_button(dialog);
   default_dialog_hide_cancel_button(dialog);

   hbox = gtk_hbox_new(FALSE, 12);
   gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog->dialog)->vbox), hbox, 
		      TRUE, TRUE, 10);
   gtk_widget_show(hbox);

   image = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_DIALOG);
   gtk_container_add(GTK_CONTAINER(hbox), image);
   gtk_widget_show(image);

   alert->label = gtk_label_new("");
   gtk_misc_set_alignment(GTK_MISC(alert->label), 0.0, 0.0);
   gtk_container_add(GTK_CONTAINER(hbox), alert->label);
   gtk_widget_show(alert->label);

   return alert;
}

void
alert_set_text(Alert_t *alert, const char *primary_text, 
	       const char *secondary_text)
{
   gchar *text;

   text = 
      g_strdup_printf("<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
		      primary_text, secondary_text);
   gtk_label_set_markup(GTK_LABEL(alert->label), text);

   g_free(text);
}

void
do_file_error_dialog(const char *error, const char *filename)
{
   static Alert_t *alert;

   if (!alert)
      alert = create_alert(GTK_STOCK_DIALOG_ERROR);

   alert_set_text(alert, error, filename);

   default_dialog_show(alert->dialog);
}
