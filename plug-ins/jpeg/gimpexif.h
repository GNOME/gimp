/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/*
 * EXIF-handling code for the metadata library.
 */

#ifdef HAVE_EXIF

#define EXIF_HEADER_SIZE 8


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

#endif /* HAVE_EXIF */
