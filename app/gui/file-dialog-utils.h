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

#ifndef __FILE_DIALOG_UTILS_H__
#define __FILE_DIALOG_UTILS_H__


GtkWidget * file_dialog_new         (Gimp              *gimp,
                                     GimpDialogFactory *dialog_factory,
                                     const gchar       *dialog_identifier,
                                     GimpMenuFactory   *menu_factory,
                                     const gchar       *menu_identifier,
                                     const gchar       *title,
                                     const gchar       *role,
                                     const gchar       *help_id);

void        file_dialog_show        (GtkWidget         *filesel,
                                     GtkWidget         *parent);
gboolean    file_dialog_hide        (GtkWidget         *filesel);

void        file_dialog_update_name (PlugInProcDef     *proc,
                                     GtkFileSelection  *filesel);


#endif /* __FILE_DIALOG_UTILS_H__ */
