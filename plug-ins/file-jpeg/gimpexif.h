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

/*
 * EXIF-handling code for the metadata library.
 */

#ifndef __GIMP_EXIF_H__
#define __GIMP_EXIF_H__

void          gimp_metadata_store_exif    (gint32       image_ID,
                                           ExifData    *exif_data);

ExifData *    gimp_metadata_generate_exif (gint32       image_ID);

const gchar * gimp_exif_content_get_value (ExifContent *content,
                                           ExifTag      tag,
                                           gchar       *value,
                                           gint         maxlen);

void          gimp_exif_data_remove_entry (ExifData    *exif_data,
                                           ExifIfd      ifd,
                                           ExifTag      tag);

#endif /* __GIMP_EXIF_H__ */
