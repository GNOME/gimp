/* GIMP - The GNU Image Manipulation Program
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

#ifndef __VECTORS_OPTIONS_DIALOG_H__
#define __VECTORS_OPTIONS_DIALOG_H__


typedef struct _VectorsOptionsDialog VectorsOptionsDialog;

struct _VectorsOptionsDialog
{
  GtkWidget   *dialog;
  GtkWidget   *name_entry;

  GimpImage   *image;
  GimpVectors *vectors;
};


VectorsOptionsDialog * vectors_options_dialog_new (GimpImage   *image,
                                                   GimpVectors *vectors,
                                                   GimpContext *context,
                                                   GtkWidget   *parent,
                                                   const gchar *vectors_name,
                                                   const gchar *title,
                                                   const gchar *role,
                                                   const gchar *stock_id,
                                                   const gchar *desc,
                                                   const gchar *help_id);


#endif /* __VECTORS_OPTIONS_DIALOG_H__ */
