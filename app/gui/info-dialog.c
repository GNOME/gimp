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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <string.h>
#include "appenv.h"
#include "gimprc.h"
#include "info_dialog.h"
#include "interface.h"

/*  static functions  */
static InfoField * info_field_new (InfoDialog *, char *, char *);
static void        update_field (InfoField *);
static gint        info_dialog_delete_callback (GtkWidget *, GdkEvent *, gpointer);

static InfoField *
info_field_new (InfoDialog *idialog,
		char       *title,
		char       *text_ptr)
{
  GtkWidget *label;
  InfoField *field;

  field = (InfoField *) g_malloc (sizeof (InfoField));

  label = gtk_label_new (title);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (idialog->labels), label, FALSE, FALSE, 0);

  field->w = gtk_label_new (text_ptr);
  gtk_misc_set_alignment (GTK_MISC (field->w), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (idialog->values), field->w, FALSE, FALSE, 0);

  field->text_ptr = text_ptr;

  gtk_widget_show (field->w);
  gtk_widget_show (label);

  return field;
}

static void
update_field (InfoField *field)
{
  gchar *old_text;

  /*  only update the field if its new value differs from the old  */
  gtk_label_get (GTK_LABEL (field->w), &old_text);

  if (strcmp (old_text, field->text_ptr))
    {
      /* set the new value and update somehow */
      gtk_label_set (GTK_LABEL (field->w), field->text_ptr);
    }
}

/*  function definitions  */

InfoDialog *
info_dialog_new (char *title)
{
  InfoDialog * idialog;
  GtkWidget *shell;
  GtkWidget *vbox;
  GtkWidget *labels, *values;
  GtkWidget *info_area;

  idialog = (InfoDialog *) g_malloc (sizeof (InfoDialog));
  idialog->field_list = NULL;

  shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (shell), "info_dialog", "Gimp");
  gtk_window_set_title (GTK_WINDOW (shell), title);
  gtk_widget_set_uposition (shell, info_x, info_y);

  gtk_signal_connect (GTK_OBJECT (shell), "delete_event",
		      GTK_SIGNAL_FUNC (info_dialog_delete_callback),
		      idialog);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (shell)->vbox), vbox, TRUE, TRUE, 0);

  info_area = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (info_area), 5);
  gtk_box_pack_start (GTK_BOX (vbox), info_area, TRUE, TRUE, 0);

  labels = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (info_area), labels, TRUE, TRUE, 0);

  values = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (info_area), values, TRUE, TRUE, 0);

  idialog->shell = shell;
  idialog->vbox = vbox;
  idialog->info_area = info_area;
  idialog->labels = labels;
  idialog->values = values;

  gtk_widget_show (idialog->labels);
  gtk_widget_show (idialog->values);
  gtk_widget_show (idialog->info_area);
  gtk_widget_show (idialog->vbox);

  return idialog;
}

void
info_dialog_free (InfoDialog *idialog)
{
  link_ptr list;

  if (!idialog)
    return;

  /*  Free each item in the field list  */
  list = idialog->field_list;

  while (list)
    {
      g_free (list->data);
      list = next_item (list);
    }

  /*  Free the actual field linked list  */
  free_list (idialog->field_list);

  /*  Destroy the associated widgets  */
  gtk_widget_destroy (idialog->shell);

  /*  Free the info dialog memory  */
  g_free (idialog);
}

void
info_dialog_add_field (InfoDialog *idialog,
		       char       *title,
		       char       *text_ptr)
{
  InfoField * new_field;

  if (!idialog)
    return;

  new_field = info_field_new (idialog, title, text_ptr);
  idialog->field_list = add_to_list (idialog->field_list, (void *) new_field);
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
  link_ptr list;

  if (!idialog)
    return;

  list = idialog->field_list;

  while (list)
    {
      update_field ((InfoField *) list->data);
      list = next_item (list);
    }
}

static gint
info_dialog_delete_callback (GtkWidget *w,
			     GdkEvent *e,
			     gpointer client_data) {

  info_dialog_popdown ((InfoDialog *) client_data);
  return FALSE;

}





