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

#include <sys/types.h>
#include <sys/stat.h>

#include "imap_default_dialog.h"
#include "imap_file.h"
#include "imap_main.h"
#include "imap_table.h"

static void
open_cb(GtkWidget *widget, gpointer data)
{
   char *filename;
   struct stat buf;
   int err;

   filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));
   err = stat(filename, &buf);
   if (!err && (buf.st_mode & S_IFREG)) {
      gtk_widget_hide((GtkWidget*) data);
      load(filename);
   } else {
      do_file_error_dialog("Error opening file", filename);
   }
}

void
do_file_open_dialog(void)
{
   static GtkWidget *dialog;
   if (!dialog) {
      dialog = gtk_file_selection_new("Load Imagemap");
      gtk_signal_connect_object(
	 GTK_OBJECT(GTK_FILE_SELECTION(dialog)->cancel_button),
	 "clicked", GTK_SIGNAL_FUNC(gtk_widget_hide), GTK_OBJECT(dialog));
      gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(dialog)->ok_button),
			 "clicked", GTK_SIGNAL_FUNC(open_cb), dialog);
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
      dialog = make_default_dialog("File exists!");
      default_dialog_hide_apply_button(dialog);
      default_dialog_set_ok_cb(dialog, really_overwrite, data);
      default_dialog_set_label(
	 dialog,
	 "File already exists.\n"
	 "  Do you really want to overwrite?  ");
   }
   default_dialog_show(dialog);
}

static void
save_cb(GtkWidget *widget, gpointer data)
{
   char *filename;
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
      dialog = gtk_file_selection_new("Save Imagemap");
      gtk_signal_connect_object(
	 GTK_OBJECT(GTK_FILE_SELECTION(dialog)->cancel_button),
	 "clicked", GTK_SIGNAL_FUNC(gtk_widget_hide), GTK_OBJECT(dialog));
      gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(dialog)->ok_button),
			 "clicked", GTK_SIGNAL_FUNC(save_cb), dialog);
   }
   gtk_widget_show(dialog);
}

typedef struct {
   DefaultDialog_t *dialog;
   GtkWidget *error;
   GtkWidget *filename;
} FileErrorDialog_t;

static FileErrorDialog_t*
create_file_error_dialog()
{
   FileErrorDialog_t *file_dialog = g_new(FileErrorDialog_t, 1);
   DefaultDialog_t *dialog;
   GtkWidget *table;

   file_dialog->dialog = dialog = make_default_dialog("Error");
   default_dialog_hide_apply_button(dialog);
   default_dialog_hide_cancel_button(dialog);

   table = gtk_table_new(2, 1, FALSE);
   gtk_container_set_border_width(GTK_CONTAINER(table), 10);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog->dialog)->vbox), table, 
		      TRUE, TRUE, 10);
   gtk_widget_show(table);

   file_dialog->error = create_label_in_table(table, 0, 0, "");
   file_dialog->filename = create_label_in_table(table, 1, 0, "");

   return file_dialog;
}

void
do_file_error_dialog(const char *error, const char *filename)
{
   static FileErrorDialog_t *dialog;
   
   if (!dialog)
      dialog = create_file_error_dialog();

   gtk_label_set_text(GTK_LABEL(dialog->error), error);
   gtk_label_set_text(GTK_LABEL(dialog->filename), filename);

   default_dialog_show(dialog->dialog);
}


