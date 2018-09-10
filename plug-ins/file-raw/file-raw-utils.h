/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-raw-utils.h -- raw file format plug-in
 * Copyright (C) 2016 Tobias Ellinghaus <me@houz.org>
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

#ifndef __FILE_RAW_UTILS_H__
#define __FILE_RAW_UTILS_H__


gchar * file_raw_get_executable_path (const gchar *main_executable,
                                      const gchar *suffix,
                                      const gchar *env_variable,
                                      const gchar *mac_bundle_id,
                                      const gchar *win32_registry_key_base,
                                      gboolean    *search_path);


#endif /* __FILE_RAW_UTILS_H__ */
