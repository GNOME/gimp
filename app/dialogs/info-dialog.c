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
#include <string.h>
#include "appenv.h"
#include "gimprc.h"
#include "info_dialog.h"
#include "interface.h"
#include "session.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpsizeentry.h"

/*  static functions  */
static void info_field_new (InfoDialog *, InfoFieldType, char *, GtkWidget *,
			    GtkObject *, void *, GtkSignalFunc, gpointer);
static void update_field   (InfoField *);
static gint info_dialog_delete_callback (GtkWidget *, GdkEvent *, gpointer);

static void
info_field_new (InfoDialog    *idialog,
		InfoFieldType  field_type,
		char          *title,
		GtkWidget     *widget,
		GtkObject     *obj,
		void          *value_ptr,
		GtkSignalFunc  callback,
		gpointer       client_data)
{
  GtkWidget *label;
  InfoField *field;
  int        row; 

  field = (InfoField *) g_malloc (sizeof (InfoField));

  row = idialog->nfields + 1;
  gtk_table_resize (GTK_TABLE (idialog->info_table), 2, row);

  label = gtk_label_new (gettext (title));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (idialog->info_table), label, 
		    0, 1, row - 1, row,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  gtk_table_attach_defaults (GTK_TABLE (idialog->info_table), widget, 
			     1, 2, row - 1, row);
  gtk_widget_show (widget);

  gtk_table_set_col_spacing (GTK_TABLE (idialog->info_table), 0, 6);
  gtk_table_set_row_spacings (GTK_TABLE (idialog->info_table), 2);

  field->field_type = field_type;
  if (obj == NULL)
    field->obj = GTK_OBJECT (widget);
  else
    field->obj = obj;
  field->value_ptr = value_ptr;
  field->callback = callback;
  field->client_data = client_data;

  idialog->field_list =
    g_slist_prepend (idialog->field_list, (void *) field);
  idialog->nfields++;
}

static void
update_field (InfoField *field)
{
  gchar *old_text;
  int    num;
  int    i;

  if (field->value_ptr == NULL)
    return;

  switch (field->field_type)
    {
    case INFO_LABEL:
      gtk_label_get (GTK_LABEL (field->obj), &old_text);
      if (strcmp (old_text, (gchar*) field->value_ptr))
	gtk_label_set_text (GTK_LABEL (field->obj), (gchar*) field->value_ptr);
      break;

    case INFO_ENTRY:
      old_text = gtk_entry_get_text (GTK_ENTRY (field->obj));
      if (strcmp (old_text, (gchar*) field->value_ptr))
	gtk_entry_set_text (GTK_ENTRY (field->obj), (gchar*) field->value_ptr);
      break;

    case INFO_SCALE:
    case INFO_SPINBUTTON:
      gtk_adjustment_set_value (GTK_ADJUSTMENT (field->obj),
				*((gfloat*) field->value_ptr));
      break;

    case INFO_SIZEENTRY:
      num = GIMP_SIZE_ENTRY (field->obj)->number_of_fields;
      for (i = 0; i < num; i++)
	gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (field->obj), i,
				    ((gfloat*) field->value_ptr)[i]);
      break;

    default:
      g_warning (_("Unknown info_dialog field type."));
      break;
    }
}

/*  function definitions  */

InfoDialog *
info_dialog_new (char *title)
{
  InfoDialog *idialog;
  GtkWidget  *shell;
  GtkWidget  *vbox;
  GtkWidget  *info_table;

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

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (shell)->vbox), vbox, TRUE, TRUE, 0);

  info_table = gtk_table_new (2, 0, FALSE);
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

  g_return_if_fail (idialog != NULL);

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

GtkWidget *
info_dialog_add_label (InfoDialog    *idialog,
		       char          *title,
		       char          *text_ptr)
{
  GtkWidget *label;

  g_return_val_if_fail (idialog != NULL, NULL);

  label = gtk_label_new (text_ptr);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  info_field_new (idialog, INFO_LABEL, title, label, NULL, (void*) text_ptr,
		  NULL, NULL);

  return label;
}

GtkWidget *
info_dialog_add_entry (InfoDialog    *idialog,
		       char          *title,
		       char          *text_ptr,
		       GtkSignalFunc  callback,
		       gpointer       data)
{
  GtkWidget *entry;

  g_return_val_if_fail (idialog != NULL, NULL);

  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 50, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), text_ptr ? text_ptr : "");

  if (callback)
    gtk_signal_connect (GTK_OBJECT (entry), "changed",
			GTK_SIGNAL_FUNC (callback), data);

  info_field_new (idialog, INFO_ENTRY, title, entry, NULL, (void*) text_ptr,
		  callback, data);

  return entry;
}

GtkWidget *
info_dialog_add_scale   (InfoDialog    *idialog,
			 char          *title,
			 float         *value_ptr,
			 gfloat         lower,
			 gfloat         upper,
			 gfloat         step_increment,
			 gfloat         page_increment,
			 gfloat         page_size,
			 gint           digits,
			 GtkSignalFunc  callback,
			 gpointer       data)
{
  GtkObject *adjustment;
  GtkWidget *scale;

  g_return_val_if_fail (idialog != NULL, NULL);

  adjustment = gtk_adjustment_new (value_ptr ? *value_ptr : 0, lower, upper,
				   step_increment, page_increment, page_size);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));

  if (digits >= 0)
    gtk_scale_set_digits (GTK_SCALE (scale), MAX (digits, 6));
  else
    gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  if (callback)
    gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			GTK_SIGNAL_FUNC (callback), data);

  info_field_new (idialog, INFO_SCALE, title, scale, adjustment,
		  (void*) value_ptr, callback, data);

  return scale;
}

GtkWidget *
info_dialog_add_spinbutton (InfoDialog    *idialog,
			    gchar         *title,
			    gfloat        *value_ptr,
			    gfloat         lower,
			    gfloat         upper,
			    gfloat         step_increment,
			    gfloat         page_increment,
			    gfloat         page_size,
			    gfloat         climb_rate,
			    gint           digits,
			    GtkSignalFunc  callback,
			    gpointer       data)
{
  GtkWidget *alignment;
  GtkObject *adjustment;
  GtkWidget *spinbutton;

  g_return_val_if_fail (idialog != NULL, NULL);

  alignment = gtk_alignment_new (0.0, 0.5, 0.0, 1.0);

  adjustment = gtk_adjustment_new (value_ptr ? *value_ptr : 0, lower, upper,
				   step_increment, page_increment, page_size);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment),
				    climb_rate, MAX (MIN (digits, 6), 0));
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);

  if (callback)
    gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
			GTK_SIGNAL_FUNC (callback), data);

  gtk_container_add (GTK_CONTAINER (alignment), spinbutton);
  gtk_widget_show (spinbutton);

  info_field_new (idialog, INFO_SPINBUTTON, title, alignment, adjustment,
		  (void*) value_ptr, callback, data);

  return spinbutton;
}

GtkWidget *
info_dialog_add_sizeentry (InfoDialog      *idialog,
			   gchar           *title,
			   gfloat          *value_ptr,
			   gint             nfields,
			   GUnit            unit,
			   gchar           *unit_format,
			   gboolean         menu_show_pixels,
			   gboolean         menu_show_percent,
			   gboolean         show_refval,
			   GimpSizeEntryUP  update_policy,
			   GtkSignalFunc    callback,
			   gpointer         data)
{
  GtkWidget *alignment;
  GtkWidget *sizeentry;
  int        i;

  g_return_val_if_fail (idialog != NULL, NULL);

  alignment = gtk_alignment_new (0.0, 0.5, 0.0, 1.0);

  sizeentry = gimp_size_entry_new (nfields, unit, unit_format,
				   menu_show_pixels, menu_show_percent,
				   show_refval, 75,
				   update_policy);
  if (value_ptr)
    for (i = 0; i < nfields; i++)
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), i, value_ptr[i]);

  if (callback)
    gtk_signal_connect (GTK_OBJECT (sizeentry), "value_changed",
			GTK_SIGNAL_FUNC (callback), data);

  gtk_container_add (GTK_CONTAINER (alignment), sizeentry);
  gtk_widget_show (sizeentry);

  info_field_new (idialog, INFO_SIZEENTRY, title, alignment,
		  GTK_OBJECT (sizeentry),
		  (void*) value_ptr, callback, data);

  return sizeentry;
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
			     GdkEvent  *e,
			     gpointer   client_data)
{
  info_dialog_popdown ((InfoDialog *) client_data);

  return TRUE;
}
