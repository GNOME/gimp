/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */
#include <stdlib.h>
#include <string.h>
#include "appenv.h"
#include "gimprc.h"
#include "info_dialog.h"
#include "interface.h"
#include "session.h"

#include "libgimp/gimpintl.h"

/*  static functions  */
static InfoField * info_field_new (InfoDialog *, char *, char *, GtkSignalFunc, gpointer);
static void        update_field (InfoField *);
static gint        info_dialog_delete_callback (GtkWidget *, GdkEvent *, gpointer);

static InfoField *
info_field_new (InfoDialog    *idialog,
		char          *title,
		char          *text_ptr,
		GtkSignalFunc  callback,
		gpointer       client_data)
{
  GtkWidget *label;
  InfoField *field;
  int        row; 

  field = (InfoField *) g_malloc (sizeof (InfoField));

  row = idialog->nfields + 1;
  gtk_table_resize (GTK_TABLE (idialog->info_table), 2, row);

  label = gtk_label_new (gettext(title));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (idialog->info_table), label, 
			     0, 1, row - 1, row);

  if (callback == NULL)
    {
      field->w = gtk_label_new (text_ptr);
      gtk_misc_set_alignment (GTK_MISC (field->w), 0.0, 0.5);
    }
  else
    {
      field->w = gtk_entry_new ();
      gtk_widget_set_usize (field->w, 50, 0);
      gtk_entry_set_text (GTK_ENTRY (field->w), text_ptr);
      gtk_signal_connect (GTK_OBJECT (field->w), "changed",
			  GTK_SIGNAL_FUNC (callback), client_data);
    }

  gtk_table_attach_defaults (GTK_TABLE (idialog->info_table), field->w, 
			     1, 2, row - 1, row);

  gtk_widget_show (field->w);
  gtk_widget_show (label);

  field->text_ptr = text_ptr;
  field->callback = callback;
  field->client_data = client_data;

  return field;
}

static void
update_field (InfoField *field)
{
  gchar *old_text;

  /*  only update the field if its new value differs from the old  */
  if (field->callback == NULL)
    gtk_label_get (GTK_LABEL (field->w), &old_text);
  else
    old_text = gtk_entry_get_text (GTK_ENTRY (field->w));

  if (strcmp (old_text, field->text_ptr))
    {
      /* set the new value and update somehow */
        if (field->callback == NULL)
	  gtk_label_set (GTK_LABEL (field->w), field->text_ptr);
	else
	  gtk_entry_set_text (GTK_ENTRY (field->w), field->text_ptr);
    }
}

/*  function definitions  */

InfoDialog *
info_dialog_new (char *title)
{
  InfoDialog * idialog;
  GtkWidget *shell;
  GtkWidget *vbox;
  GtkWidget *info_table;

  idialog = (InfoDialog *) g_malloc (sizeof (InfoDialog));
  idialog->field_list = NULL;
  idialog->nfields = 0;

  shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (shell), "info_dialog", "Gimp");
  gtk_window_set_title (GTK_WINDOW (shell), gettext(title));
  session_set_window_geometry (shell, &info_dialog_session_info, FALSE );

  gtk_signal_connect (GTK_OBJECT (shell), "delete_event",
		      GTK_SIGNAL_FUNC (info_dialog_delete_callback),
		      idialog);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (shell)->vbox), vbox, TRUE, TRUE, 0);

  info_table = gtk_table_new (0, 0, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (info_table), 5);
  gtk_box_pack_start (GTK_BOX (vbox), info_table, TRUE, TRUE, 0);

  idialog->shell = shell;
  idialog->vbox = vbox;
  idialog->info_table = info_table;

  gtk_widget_show (idialog->info_table);
  gtk_widget_show (idialog->vbox);

  return idialog;
}

void
info_dialog_free (InfoDialog *idialog)
{
  GSList *list;

  if (!idialog)
    return;

  /*  Free each item in the field list  */
  list = idialog->field_list;

  while (list)
    {
      g_free (list->data);
      list = g_slist_next (list);
    }

  /*  Free the actual field linked list  */
  g_slist_free (idialog->field_list);

  session_get_window_info (idialog->shell, &info_dialog_session_info);

  /*  Destroy the associated widgets  */
  gtk_widget_destroy (idialog->shell);

  /*  Free the info dialog memory  */
  g_free (idialog);
}

void
info_dialog_add_field (InfoDialog    *idialog,
		       char          *title,
		       char          *text_ptr,
		       GtkSignalFunc  callback,
		       gpointer       data)
{
  InfoField * new_field;

  if (!idialog)
    return;

  new_field = info_field_new (idialog, title, text_ptr, callback, data);
  idialog->field_list = g_slist_prepend (idialog->field_list, (void *) new_field);
  idialog->nfields++;
}

void
info_dialog_popup (InfoDialog *idialog)
{
  if (!idialog)
    return;

  if (!GTK_WIDGET_VISIBLE (idialog->shell))
    gtk_widget_show (idialog->shell);
}

void
info_dialog_popdown (InfoDialog *idialog)
{
  if (!idialog)
    return;
  
  if (GTK_WIDGET_VISIBLE (idialog->shell))
    gtk_widget_hide (idialog->shell);
}

void
info_dialog_update (InfoDialog *idialog)
{
  GSList *list;

  if (!idialog)
    return;

  list = idialog->field_list;

  while (list)
    {
      update_field ((InfoField *) list->data);
      list = g_slist_next (list);
    }
}

static gint
info_dialog_delete_callback (GtkWidget *w,
			     GdkEvent *e,
			     gpointer client_data)
{
  info_dialog_popdown ((InfoDialog *) client_data);

  return TRUE;
}





