/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpimage.h
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

#ifndef __GIMP_IMAGE_H__
#define __GIMP_IMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */

#define gimp_image_convert_rgb        gimp_convert_rgb
#define gimp_image_convert_grayscale  gimp_convert_grayscale
#define gimp_image_convert_indexed    gimp_convert_indexed
#define gimp_image_duplicate          gimp_channel_ops_duplicate


guchar * gimp_image_get_cmap           (gint32  image_ID,
					gint   *num_colors);
void     gimp_image_set_cmap           (gint32  image_ID,
					guchar *cmap,
					gint    num_colors);

guchar * gimp_image_get_thumbnail_data (gint32  image_ID,
					gint   *width,
					gint   *height,
					gint   *bpp);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_IMAGE_H__ */
