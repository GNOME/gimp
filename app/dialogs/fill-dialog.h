/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * fill-dialog.h
 * Copyright (C) 2016  Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __FILL_DIALOG_H__
#define __FILL_DIALOG_H__


typedef void (* GimpFillCallback) (GtkWidget       *dialog,
                                   GList           *items,
                                   GList           *drawables,
                                   GimpContext     *context,
                                   GimpFillOptions *options,
                                   gpointer         user_data);


GtkWidget * fill_dialog_new (GList            *items,
                             GList            *drawables,
                             GimpContext      *context,
                             const gchar      *title,
                             const gchar      *icon_name,
                             const gchar      *help_id,
                             GtkWidget        *parent,
                             GimpFillOptions  *options,
                             GimpFillCallback  callback,
                             gpointer          user_data);


#endif  /*  __FILL_DIALOG_H__  */
