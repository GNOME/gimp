/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * file-remote.h
 * Copyright (C) 2014  Michael Natterer <mitch@gimp.org>
 *
 * Based on: URI plug-in, GIO/GVfs backend
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
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

#ifndef __FILE_REMOTE_H__
#define __FILE_REMOTE_H__


gboolean   file_remote_mount_file           (Gimp          *gimp,
                                             GFile         *file,
                                             GimpProgress  *progress,
                                             GError       **error);

GFile    * file_remote_download_image       (Gimp          *gimp,
                                             GFile         *file,
                                             GimpProgress  *progress,
                                             GError       **error);

GFile    * file_remote_upload_image_prepare (Gimp          *gimp,
                                             GFile         *file,
                                             GimpProgress  *progress,
                                             GError       **error);
gboolean   file_remote_upload_image_finish  (Gimp          *gimp,
                                             GFile         *file,
                                             GFile         *local_file,
                                             GimpProgress  *progress,
                                             GError       **error);

#endif /* __FILE_REMOTE_H__ */
