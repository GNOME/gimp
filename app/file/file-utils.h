/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-utils.h
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

#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__


gchar         * file_utils_filename_to_uri        (GSList        *procs,
                                                   const gchar   *filename,
                                                   GError       **error);
gchar         * file_utils_filename_from_uri      (const gchar   *uri);

gchar         * file_utils_uri_to_utf8_filename   (const gchar   *uri);

gchar         * file_utils_uri_display_basename   (const gchar   *uri);
gchar         * file_utils_uri_display_name       (const gchar   *uri);

PlugInProcDef * file_utils_find_proc              (GSList        *procs,
                                                   const gchar   *filename);
PlugInProcDef * file_utils_find_proc_by_extension (GSList        *procs,
                                                   const gchar   *uri);


#endif /* __FILE_UTILS_H__ */
