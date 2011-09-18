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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __JPEG_EXIF_H__
#define __JPEG_EXIF_H__

#ifdef HAVE_LIBEXIF

extern ExifData *exif_data;

ExifData * jpeg_exif_data_new_from_file (const gchar   *filename,
                                         GError       **error);

gint      jpeg_exif_get_orientation     (ExifData      *exif_data);

gboolean  jpeg_exif_get_resolution      (ExifData       *exif_data,
                                         gdouble        *xresolution,
                                         gdouble        *yresolution,
                                         gint           *unit);

void      jpeg_setup_exif_for_save      (ExifData      *exif_data,
                                         const gint32   image_ID);

void      jpeg_exif_rotate              (gint32         image_ID,
                                         gint           orientation);
void      jpeg_exif_rotate_query        (gint32         image_ID,
                                         gint           orientation);

#endif /* HAVE_LIBEXIF */

#endif /* __JPEG_EXIF_H__ */
