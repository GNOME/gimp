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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "imap_default_dialog.h"

#include "libgimp/stdplugins-intl.h"


static void
dialog_cancel(GtkWidget *widget, gpointer data)
{
   DefaultDialog_t *dialog = (DefaultDialog_t*) data;
   gtk_widget_hide(dialog->dialog);
   if (dialog->cancel_cb)
      dialog->cancel_cb(dialog->cancel_cb_data);
}

static gboolean
dialog_destroy(GtkWidget *widget, GdkEvent *event, gpointer data)
{
   dialog_cancel(widget, data);
   return TRUE;
}

static void
dialog_apply(GtkWidget *widget, gpointer data)
{
   DefaultDialog_t *dialog = (DefaultDialog_t*) data;
   if (dialog->apply_cb)
      dialog->apply_cb(dialog->apply_cb_data);
   else if (dialog->ok_cb)
      dialog->ok_cb(dialog->ok_cb_data);
}

static void
dialog_ok(GtkWidget *widget, gpointer data)
{
   DefaultDialog_t *dialog = (DefaultDialog_t*) data;
   gtk_widget_hide(dialog->dialog);
   if (dialog->ok_cb)
      dialog->ok_cb(dialog->ok_cb_data);
}

void 
default_dialog_set_ok_cb(DefaultDialog_t *dialog, void (*ok_cb)(gpointer), 
			 gpointer ok_cb_data)
{
   dialog->ok_cb = ok_cb;
   dialog->ok_cb_data = ok_cb_data;
}

void 
default_dialog_set_apply_cb(DefaultDialog_t *dialog, 
			    void (*apply_cb)(gpointer), 
			    gpointer apply_cb_data)
{
   dialog->apply_cb = apply_cb;
   dialog->apply_cb_data = apply_cb_data;
}

void 
default_dialog_set_cancel_cb(DefaultDialog_t *dialog, 
			     void (*cancel_cb)(gpointer), 
			     gpointer cancel_cb_data)
{
   dialog->cancel_cb = cancel_cb;
   dialog->cancel_cb_data = cancel_cb_data;
}

DefaultDialog_t*
make_default_dialog(const gchar *title)
{
   DefaultDialog_t *data = g_new(DefaultDialog_t, 1);
   GtkWidget *dialog;

   data->ok_cb = NULL;
   data->apply_cb = NULL;
   data->cancel_cb = NULL;
   dialog = gimp_dialog_new (title, "imagemap",
			     gimp_standard_help_func, "filters/imagemap.html",
			     GTK_WIN_POS_MOUSE,
			     FALSE, TRUE, FALSE,

			     GTK_STOCK_APPLY, dialog_apply,
			     data, NULL, &data->apply, FALSE, FALSE,

			     GTK_STOCK_CANCEL, dialog_cancel,
			     data, NULL, &data->cancel, FALSE, FALSE,

			     GTK_STOCK_OK, dialog_ok,
			     data, NULL, &data->ok, TRUE, FALSE,

			     NULL);
   data->dialog = dialog;

   g_signal_connect (dialog, "destroy",
		     G_CALLBACK(gtk_widget_destroyed), &data->dialog);
   g_signal_connect(dialog, "delete_event",
                    G_CALLBACK(dialog_destroy), (gpointer) data);
 
   return data;
}

void 
default_dialog_show(DefaultDialog_t *dialog)
{
   gtk_widget_show(dialog->dialog);
}

void 
default_dialog_hide_cancel_button(DefaultDialog_t *dialog)
{
   gtk_widget_hide(dialog->cancel);
}

void 
default_dialog_hide_apply_button(DefaultDialog_t *dialog)
{
   gtk_widget_hide(dialog->apply);
}

void 
default_dialog_hide_help_button(DefaultDialog_t *dialog)
{
  /* gtk_widget_hide(dialog->help); */
}

void
default_dialog_set_title(DefaultDialog_t *dialog, const gchar *title)
{
   gtk_window_set_title(GTK_WINDOW(dialog->dialog), title);
}

void 
default_dialog_set_ok_sensitivity(DefaultDialog_t *dialog, gboolean sensitive)
{
   gtk_widget_set_sensitive(dialog->ok, sensitive);
}

void
default_dialog_set_label(DefaultDialog_t *dialog, gchar *text)
{
   GtkWidget *label = gtk_label_new(text);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog->dialog)->vbox), label, 
		      TRUE, TRUE, 5);
   gtk_widget_show(label);
}

GtkWidget*
default_dialog_add_table(DefaultDialog_t *dialog, gint rows, gint cols)
{
   GtkWidget *table = gtk_table_new(rows, cols, FALSE);
   gtk_container_set_border_width(GTK_CONTAINER(table), 10);
   gtk_table_set_row_spacings(GTK_TABLE(table), 10);
   gtk_table_set_col_spacings(GTK_TABLE(table), 10);
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog->dialog)->vbox), table);
   gtk_widget_show(table);
   return table;
}
