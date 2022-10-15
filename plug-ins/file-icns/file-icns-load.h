/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * file-icns-load.h
 * Copyright (C) 2004 Brion Vibber <brion@pobox.com>
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

#ifndef __ICNS_LOAD_H__
#define __ICNS_LOAD_H__

GimpImage * icns_load_image           (GFile         *file,
                                       gint32        *file_offset,
                                       GError       **error);

GimpImage * icns_load_thumbnail_image (GFile         *file,
                                       gint          *width,
                                       gint          *height,
                                       gint32         file_offset,
                                       GError       **error);

#endif
