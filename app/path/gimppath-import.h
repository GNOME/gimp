/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPath Import
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_PATH_IMPORT_H__
#define __GIMP_PATH_IMPORT_H__


gboolean  gimp_path_import_file   (GimpImage    *image,
                                   GFile        *file,
                                   gboolean      merge,
                                   gboolean      scale,
                                   GimpPath     *parent,
                                   gint          position,
                                   GList       **ret_paths,
                                   GError      **error);
gboolean  gimp_path_import_buffer (GimpImage    *image,
                                   const gchar  *buffer,
                                   gsize         len,
                                   gboolean      merge,
                                   gboolean      scale,
                                   GimpPath     *parent,
                                   gint          position,
                                   GList       **ret_paths,
                                   GError      **error);


#endif /* __GIMP_PATH_IMPORT_H__ */
