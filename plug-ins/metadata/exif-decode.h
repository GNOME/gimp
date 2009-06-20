/* exif-decode.h - decode exif data and convert it to XMP
 *
 * Copyright (C) 2008, Róman Joost <romanofski@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef EXIF_DECODE_H
#define EXIF_DECODE_H

G_BEGIN_DECLS

gboolean        exif_merge_to_xmp           (XMPModel            *xmp_model,
                                             const gchar         *filename,
                                             GError             **error);

gboolean        xmp_merge_from_exifbuffer   (XMPModel            *xmp_model,
                                             gint32               image_ID,
                                             GError             **error);

G_END_DECLS

#endif /* EXIF_DECODE_H */
