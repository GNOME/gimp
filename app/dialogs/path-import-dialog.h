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

#ifndef __PATH_IMPORT_DIALOG_H__
#define __PATH_IMPORT_DIALOG_H__


typedef void (* GimpPathImportCallback) (GtkWidget *dialog,
                                         GimpImage *image,
                                         GFile     *file,
                                         GFile     *import_folder,
                                         gboolean   merge_vectors,
                                         gboolean   scale_vectors,
                                         gpointer   user_data);


GtkWidget * path_import_dialog_new (GimpImage                 *image,
                                    GtkWidget                 *parent,
                                    GFile                     *import_folder,
                                    gboolean                   merge_path,
                                    gboolean                   scale_path,
                                    GimpPathImportCallback     callback,
                                    gpointer                   user_data);


#endif /* __PATH_IMPORT_DIALOG_H__ */
