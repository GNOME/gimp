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

typedef struct _info_field InfoField;

struct _info_field
{
  GtkWidget     *w;
  char          *text_ptr;
  GtkSignalFunc  callback;
  gpointer       client_data;
};


typedef struct _info_dialog InfoDialog;

struct _info_dialog
{
  GtkWidget   *shell;
  GtkWidget   *vbox;
  GtkWidget   *info_table;

  GSList      *field_list;
  int          nfields;

  void        *user_data;
};


/*  Info Dialog functions  */

InfoDialog *  info_dialog_new         (char *);
void          info_dialog_free        (InfoDialog *);
void          info_dialog_add_field   (InfoDialog *, 
				       char *, 
				       char *, 
				       GtkSignalFunc,
				       gpointer);
void          info_dialog_popup       (InfoDialog *);
void          info_dialog_popdown     (InfoDialog *);
void          info_dialog_update      (InfoDialog *);

#endif  /*  __INFO_DIALOG_H__  */
