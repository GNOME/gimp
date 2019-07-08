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

#ifndef __JPEG_LOAD_H__
#define __JPEG_LOAD_H__

gint32 load_image           (const gchar  *filename,
                             GimpRunMode   runmode,
                             gboolean      preview,
                             gboolean     *resolution_loaded,
                             GError      **error);

gint32 load_thumbnail_image (GFile         *file,
                             gint          *width,
                             gint          *height,
                             GimpImageType *type,
                             GError       **error);

#endif /* __JPEG_LOAD_H__ */
