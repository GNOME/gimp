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

#ifndef __INFO_DIALOG_H__
#define __INFO_DIALOG_H__


typedef enum
{
  INFO_LABEL,
  INFO_ENTRY,
  INFO_SCALE,
  INFO_SPINBUTTON,
  INFO_SIZEENTRY
} InfoFieldType;

typedef struct _InfoField  InfoField;

struct _InfoField
{
  InfoFieldType  field_type;

  GtkObject     *obj;
  gpointer       value_ptr;

  GCallback      callback;
  gpointer       callback_data;
};

struct _InfoDialog
{
  GtkWidget   *shell;
  GtkWidget   *vbox;
  GtkWidget   *info_table;
  GtkWidget   *info_notebook;

  GSList      *field_list;
  gint         nfields;

  gpointer     user_data;
};

/*  Info Dialog functions  */

InfoDialog *info_dialog_new            (GimpViewable    *viewable,
                                        const gchar     *title,
                                        const gchar     *role,
                                        const gchar     *stock_id,
                                        const gchar     *desc,
                                        GtkWidget       *parent,
					GimpHelpFunc     help_func,
					gpointer         help_data);
InfoDialog *info_dialog_notebook_new   (GimpViewable    *viewable,
                                        const gchar     *title,
                                        const gchar     *role,
                                        const gchar     *stock_id,
                                        const gchar     *desc,
                                        GtkWidget       *parent,
					GimpHelpFunc     help_func,
					gpointer         help_data);
void        info_dialog_free           (InfoDialog      *idialog);

void        info_dialog_show           (InfoDialog      *idialog);
void        info_dialog_present        (InfoDialog      *idialog);
void        info_dialog_hide           (InfoDialog      *idialog);

void        info_dialog_update         (InfoDialog      *idialog);

GtkWidget  *info_dialog_add_label      (InfoDialog      *idialog,
					gchar           *title,
					gchar           *text_ptr);

GtkWidget  *info_dialog_add_entry      (InfoDialog      *idialog,
					gchar           *title,
					gchar           *text_ptr,
					GCallback        callback,
					gpointer         callback_data);

GtkWidget  *info_dialog_add_scale      (InfoDialog      *idialog,
					gchar           *title,
					gdouble         *value_ptr,
					gfloat           lower,
					gfloat           upper,
					gfloat           step_increment,
					gfloat           page_increment,
					gfloat           page_size,
					gint             digits,
					GCallback        callback,
					gpointer         callback_data);

GtkWidget  *info_dialog_add_spinbutton (InfoDialog      *idialog,
					gchar           *title,
					gdouble         *value_ptr,
					gfloat           lower,
					gfloat           upper,
					gfloat           step_increment,
					gfloat           page_increment,
					gfloat           page_size,
					gfloat           climb_rate,
					gint             digits,
					GCallback        callback,
					gpointer         callback_data);

GtkWidget  *info_dialog_add_sizeentry  (InfoDialog      *idialog,
					gchar           *title,
					gdouble         *value_ptr,
					gint             nfields,
					GimpUnit         unit,
					gchar           *unit_format,
					gboolean         menu_show_pixels,
					gboolean         menu_show_percent,
					gboolean         show_refval,
					GimpSizeEntryUpdatePolicy update_policy,
					GCallback        callback,
					gpointer         callback_data);

#endif  /*  __INFO_DIALOG_H__  */
