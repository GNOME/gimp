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

#pragma once


GimpImage * xcf_load_image         (Gimp           *gimp,
                                    XcfInfo        *info,
                                    GError        **error);

gboolean    xcf_load_magic_version (Gimp           *gimp,
                                    GInputStream   *input,
                                    GFile          *input_file,
                                    GimpProgress   *progress,
                                    XcfInfo        *info);
gboolean    xcf_load_image_header  (Gimp           *gimp,
                                    XcfInfo        *info,
                                    gint           *width,
                                    gint           *height,
                                    gint           *image_type,
                                    GimpPrecision  *precision,
                                    GList          *prev_files,
                                    GList         **loop_files,
                                    gboolean       *loop_found,
                                    GError        **error);

gboolean    xcf_load_file_equal    (GFile          *file1,
                                    GFile          *file2);
