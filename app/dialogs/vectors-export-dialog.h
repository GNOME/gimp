/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __VECTORS_EXPORT_DIALOG_H__
#define __VECTORS_EXPORT_DIALOG_H__


typedef void (* GimpVectorsExportCallback) (GtkWidget *dialog,
                                            GimpImage *image,
                                            GFile     *file,
                                            GFile     *export_folder,
                                            gboolean   active_only,
                                            gpointer   user_data);


GtkWidget * vectors_export_dialog_new (GimpImage                 *image,
                                       GtkWidget                 *parent,
                                       GFile                     *export_folder,
                                       gboolean                   active_only,
                                       GimpVectorsExportCallback  callback,
                                       gpointer                   user_data);


#endif /* __VECTORS_EXPORT_DIALOG_H__ */
