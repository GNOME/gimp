/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-utils.h
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

#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__


GFile     * file_utils_filename_to_file  (Gimp         *gimp,
                                          const gchar  *filename,
                                          GError      **error);

GdkPixbuf * file_utils_load_thumbnail    (GFile        *file);
gboolean    file_utils_save_thumbnail    (GimpImage    *image,
                                          GFile        *file);


#endif /* __FILE_UTILS_H__ */
