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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "widgets/gimpviewabledialog.h"

#include "info-dialog.h"

#include "gimp-intl.h"


#define SB_WIDTH 10


/*  local function prototypes  */

static InfoDialog * info_dialog_new_extended    (GimpViewable  *viewable,
                                                 const gchar   *title,
                                                 const gchar   *role,
                                                 const gchar   *stock_id,
                                                 const gchar   *desc,
                                                 GtkWidget     *parent,
                                                 GimpHelpFunc   help_func,
                                                 gpointer       help_data,
                                                 gboolean       in_notebook);
static void         info_dialog_field_new       (InfoDialog    *idialog,
                                                 InfoFieldType  field_type,
                                                 gchar         *title,
                                                 GtkWidget     *widget,
                                                 GtkObject     *object,
                                                 gpointer       value_ptr,
                                                 GCallback      callback,
                                                 gpointer       callback_data);
static void         info_dialog_update_field    (InfoField     *info_field);
static void         info_dialog_field_free      (gpointer       data,
                                                 gpointer       user_data);


/*  public functions  */

InfoDialog *
info_dialog_new (GimpViewable *viewable,
                 const gchar  *title,
                 const gchar  *role,
                 const gchar  *stock_id,
                 const gchar  *desc,
                 GtkWidget    *parent,
		 GimpHelpFunc  help_func,
		 gpointer      help_data)
{
  return info_dialog_new_extended (viewable, title, role,
                                   stock_id, desc,
                                   parent,
                                   help_func, help_data, FALSE);
}

InfoDialog *
info_dialog_notebook_new (GimpViewable *viewable,
                          const gchar  *title,
                          const gchar  *role,
                          const gchar  *stock_id,
                          const gchar  *desc,
                          GtkWidget    *parent,
                          GimpHelpFunc  help_func,
			  gpointer      help_data)
{
  return info_dialog_new_extended (viewable, title, role,
                                   stock_id, desc,
                                   parent,
                                   help_func, help_data, TRUE);
}

static void
info_dialog_field_free (gpointer data,
                        gpointer user_data)
{
  InfoField *field = data;

  g_signal_handlers_disconnect_by_func (field->obj,
                                        field->callback,
                                        field->callback_data);
  g_free (field);
}

void
info_dialog_free (InfoDialog *idialog)
{
  g_return_if_fail (idialog != NULL);

  g_slist_foreach (idialog->field_list, (GFunc) info_dialog_field_free, NULL);
  g_slist_free (idialog->field_list);

  gtk_widget_destroy (idialog->shell);

  g_free (idialog);
}

void
info_dialog_show (InfoDialog *idialog)
{
  g_return_if_fail (idialog != NULL);

  gtk_widget_show (idialog->shell);
}

void
info_dialog_present (InfoDialog *idialog)
{
  g_return_if_fail (idialog != NULL);

  gtk_window_present (GTK_WINDOW (idialog->shell));
}

void
info_dialog_hide (InfoDialog *idialog)
{
  g_return_if_fail (idialog != NULL);

  gtk_widget_hide (idialog->shell);
}

void
info_dialog_update (InfoDialog *idialog)
{
  GSList *list;

  g_return_if_fail (idialog != NULL);

  for (list = idialog->field_list; list; list = g_slist_next (list))
    info_dialog_update_field ((InfoField *) list->data);
}

GtkWidget *
info_dialog_add_label (InfoDialog *idialog,
		       gchar      *title,
		       gchar      *text_ptr)
{
  GtkWidget *label;

  g_return_val_if_fail (idialog != NULL, NULL);

  label = gtk_label_new (text_ptr);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  info_dialog_field_new (idialog, INFO_LABEL, title, label, NULL,
                         text_ptr,
                         NULL, NULL);

  return label;
}

GtkWidget *
info_dialog_add_entry (InfoDialog    *idialog,
		       gchar         *title,
		       gchar         *text_ptr,
		       GCallback      callback,
		       gpointer       callback_data)
{
  GtkWidget *entry;

  g_return_val_if_fail (idialog != NULL, NULL);

  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 50, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), text_ptr ? text_ptr : "");

  if (callback)
    g_signal_connect (entry, "changed",
		      callback,
		      callback_data);

  info_dialog_field_new (idialog, INFO_ENTRY, title, entry, NULL,
                         text_ptr,
                         callback, callback_data);

  return entry;
}

GtkWidget *
info_dialog_add_scale   (InfoDialog    *idialog,
			 gchar         *title,
			 gdouble       *value_ptr,
			 gfloat         lower,
			 gfloat         upper,
			 gfloat         step_increment,
			 gfloat         page_increment,
			 gfloat         page_size,
			 gint           digits,
			 GCallback      callback,
			 gpointer       callback_data)
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
    g_signal_connect (adjustment, "value_changed",
		      callback,
		      callback_data);

  info_dialog_field_new (idialog, INFO_SCALE, title, scale, adjustment,
                         value_ptr,
                         callback, callback_data);

  return scale;
}

GtkWidget *
info_dialog_add_spinbutton (InfoDialog    *idialog,
			    gchar         *title,
			    gdouble       *value_ptr,
			    gfloat         lower,
			    gfloat         upper,
			    gfloat         step_increment,
			    gfloat         page_increment,
			    gfloat         page_size,
			    gfloat         climb_rate,
			    gint           digits,
			    GCallback      callback,
			    gpointer       callback_data)
{
  GtkWidget *alignment;
  GtkObject *adjustment;
  GtkWidget *spinbutton;

  g_return_val_if_fail (idialog != NULL, NULL);

  alignment = gtk_alignment_new (0.0, 0.5, 0.0, 1.0);

  spinbutton = gimp_spin_button_new (&adjustment,
                                     value_ptr ? *value_ptr : 0,
                                     lower, upper,
                                     step_increment, page_increment, page_size,
                                     climb_rate, MAX (MIN (digits, 6), 0));
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);

  if (callback)
    g_signal_connect (adjustment, "value_changed",
		      callback,
		      callback_data);

  gtk_container_add (GTK_CONTAINER (alignment), spinbutton);
  gtk_widget_show (spinbutton);

  info_dialog_field_new (idialog, INFO_SPINBUTTON, title, alignment,
                         adjustment,
                         value_ptr,
                         callback, callback_data);

  return spinbutton;
}

GtkWidget *
info_dialog_add_sizeentry (InfoDialog                *idialog,
			   gchar                     *title,
			   gdouble                   *value_ptr,
			   gint                       nfields,
			   GimpUnit                   unit,
			   gchar                     *unit_format,
			   gboolean                   menu_show_pixels,
			   gboolean                   menu_show_percent,
			   gboolean                   show_refval,
			   GimpSizeEntryUpdatePolicy  update_policy,
			   GCallback                  callback,
			   gpointer                   callback_data)
{
  GtkWidget *alignment;
  GtkWidget *sizeentry;
  gint       i;

  g_return_val_if_fail (idialog != NULL, NULL);

  alignment = gtk_alignment_new (0.0, 0.5, 0.0, 1.0);

  sizeentry = gimp_size_entry_new (nfields, unit, unit_format,
				   menu_show_pixels, menu_show_percent,
				   show_refval, SB_WIDTH,
				   update_policy);
  if (value_ptr)
    for (i = 0; i < nfields; i++)
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), i, value_ptr[i]);

  if (callback)
    g_signal_connect (sizeentry, "value_changed",
		      callback,
		      callback_data);

  gtk_container_add (GTK_CONTAINER (alignment), sizeentry);
  gtk_widget_show (sizeentry);

  info_dialog_field_new (idialog, INFO_SIZEENTRY, title, alignment,
                         GTK_OBJECT (sizeentry),
                         value_ptr,
                         callback, callback_data);

  return sizeentry;
}


/*  private functions  */

static InfoDialog *
info_dialog_new_extended (GimpViewable *viewable,
                          const gchar  *title,
                          const gchar  *role,
                          const gchar  *stock_id,
                          const gchar  *desc,
                          GtkWidget    *parent,
			  GimpHelpFunc  help_func,
			  gpointer      help_data,
			  gboolean      in_notebook)
{
  InfoDialog *idialog;
  GtkWidget  *shell;
  GtkWidget  *vbox;
  GtkWidget  *info_table;
  GtkWidget  *info_notebook;

  idialog = g_new (InfoDialog, 1);
  idialog->field_list = NULL;
  idialog->nfields    = 0;

  shell = gimp_viewable_dialog_new (viewable,
                                    title, role,
                                    stock_id, desc,
                                    parent,
                                    help_func, help_data,
                                    NULL);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell)->vbox), vbox);

  info_table = gtk_table_new (2, 0, FALSE);

  if (in_notebook)
    {
      info_notebook = gtk_notebook_new ();
      gtk_container_set_border_width (GTK_CONTAINER (info_table), 6);
      gtk_notebook_append_page (GTK_NOTEBOOK (info_notebook),
				info_table,
				gtk_label_new (_("General")));
      gtk_box_pack_start (GTK_BOX (vbox), info_notebook, FALSE, FALSE, 0);
    }
  else
    {
      info_notebook = NULL;
      gtk_box_pack_start (GTK_BOX (vbox), info_table, FALSE, FALSE, 0);
    }

  idialog->shell         = shell;
  idialog->vbox          = vbox;
  idialog->info_table    = info_table;
  idialog->info_notebook = info_notebook;

  if (in_notebook)
    gtk_widget_show (idialog->info_notebook);

  gtk_widget_show (idialog->info_table);
  gtk_widget_show (idialog->vbox);

  return idialog;
}

static void
info_dialog_field_new (InfoDialog    *idialog,
                       InfoFieldType  field_type,
                       gchar         *title,
                       GtkWidget     *widget,
                       GtkObject     *obj,
                       gpointer       value_ptr,
                       GCallback      callback,
                       gpointer       callback_data)
{
  GtkWidget *label;
  InfoField *field;
  gint       row;

  field = g_new (InfoField, 1);

  row = idialog->nfields + 1;
  gtk_table_resize (GTK_TABLE (idialog->info_table), 2, row);

  label = gtk_label_new (title);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
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

  field->value_ptr     = value_ptr;
  field->callback      = callback;
  field->callback_data = callback_data;

  idialog->field_list = g_slist_prepend (idialog->field_list, field);
  idialog->nfields++;
}

static void
info_dialog_update_field (InfoField *field)
{
  const gchar *old_text;
  gint         num;
  gint         i;

  if (field->value_ptr == NULL)
    return;

  if (field->field_type != INFO_LABEL)
    g_signal_handlers_block_by_func (field->obj,
				     field->callback,
				     field->callback_data);

  switch (field->field_type)
    {
    case INFO_LABEL:
      gtk_label_set_text (GTK_LABEL (field->obj), (gchar *) field->value_ptr);
      break;

    case INFO_ENTRY:
      old_text = gtk_entry_get_text (GTK_ENTRY (field->obj));
      if (strcmp (old_text, (gchar *) field->value_ptr))
	gtk_entry_set_text (GTK_ENTRY (field->obj), (gchar *) field->value_ptr);
      break;

    case INFO_SCALE:
    case INFO_SPINBUTTON:
      gtk_adjustment_set_value (GTK_ADJUSTMENT (field->obj),
				*((gdouble *) field->value_ptr));
      break;

    case INFO_SIZEENTRY:
      num = GIMP_SIZE_ENTRY (field->obj)->number_of_fields;
      for (i = 0; i < num; i++)
	gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (field->obj), i,
				    ((gdouble *) field->value_ptr)[i]);
      break;

    default:
      g_warning ("%s: Unknown info_dialog field type.", G_STRFUNC);
      break;
    }

  if (field->field_type != INFO_LABEL)
    g_signal_handlers_unblock_by_func (field->obj,
				       field->callback,
				       field->callback_data);
}
