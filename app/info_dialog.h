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

#include "gtk/gtk.h"

#include "libgimp/gimpsizeentry.h"
#include "libgimp/gimpunit.h"

typedef enum
{ 
  INFO_LABEL,
  INFO_ENTRY,
  INFO_SCALE,
  INFO_SPINBUTTON,
  INFO_SIZEENTRY
} InfoFieldType;

typedef struct _InfoField  InfoField;
typedef struct _InfoDialog InfoDialog;

struct _InfoField
{
  InfoFieldType  field_type;

  GtkObject     *obj;
  void          *value_ptr;

  GtkSignalFunc  callback;
  gpointer       client_data;
};

struct _InfoDialog
{
  GtkWidget   *shell;
  GtkWidget   *vbox;
  GtkWidget   *info_table;
  GtkWidget   *info_notebook;

  GSList      *field_list;
  gint         nfields;

  void        *user_data;
};

/*  Info Dialog functions  */

InfoDialog *info_dialog_new            (gchar           *title);
InfoDialog *info_dialog_notebook_new   (gchar           *title);
void        info_dialog_free           (InfoDialog      *idialog);

void        info_dialog_popup          (InfoDialog      *idialog);
void        info_dialog_popdown        (InfoDialog      *idialog);

void        info_dialog_update         (InfoDialog      *idialog);

GtkWidget  *info_dialog_add_label      (InfoDialog      *idialog,
					gchar           *title,
					gchar           *text_ptr);

GtkWidget  *info_dialog_add_entry      (InfoDialog      *idialog,
					gchar           *title,
					gchar           *text_ptr,
					GtkSignalFunc    callback,
					gpointer         data);

GtkWidget  *info_dialog_add_scale      (InfoDialog      *idialog,
					gchar           *title,
					gdouble         *value_ptr,
					gfloat           lower,
					gfloat           upper,
					gfloat           step_increment,
					gfloat           page_increment,
					gfloat           page_size,
					gint             digits,
					GtkSignalFunc    callback,
					gpointer         data);

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
					GtkSignalFunc    callback,
					gpointer         data);

GtkWidget  *info_dialog_add_sizeentry  (InfoDialog      *idialog,
					gchar           *title,
					gdouble         *value_ptr,
					gint             nfields,
					GUnit            unit,
					gchar           *unit_format,
					gboolean         menu_show_pixels,
					gboolean         menu_show_percent,
					gboolean         show_refval,
					GimpSizeEntryUP  update_policy,
					GtkSignalFunc    callback,
					gpointer         data);

#endif  /*  __INFO_DIALOG_H__  */
