/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpimage.c
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

#include "gimp.h"


/**
 * gimp_image_get_cmap:
 * @image_ID: The image.
 * @num_colors: Number of colors in the colormap array.
 *
 * Returns the image's colormap
 *
 * This procedure returns an actual pointer to the image's colormap, as
 * well as the number of colors contained in the colormap. If the image 
 * is not of base type INDEXED, this pointer will be NULL.
 *
 * Returns: The image's colormap.
 */
guchar *
gimp_image_get_cmap (gint32  image_ID,
		     gint   *num_colors)
{
  gint    num_bytes;
  guchar *cmap;

  cmap = _gimp_image_get_cmap (image_ID,
			       &num_bytes);

  *num_colors = num_bytes / 3;

  return cmap;
}

/**
 * gimp_image_set_cmap:
 * @image_ID: The image.
 * @cmap: The new colormap values.
 * @num_colors: Number of colors in the colormap array.
 *
 * Sets the entries in the image's colormap.
 *
 * This procedure sets the entries in the specified image's colormap.
 * The number of colors is specified by the \"num_colors\" parameter
 * and corresponds to the number of INT8 triples that must be contained
 * in the \"cmap\" array.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_image_set_cmap (gint32  image_ID,
		     guchar *cmap,
		     gint    num_colors)
{
  return _gimp_image_set_cmap (image_ID,
			       num_colors * 3,
			       cmap);
}

guchar *
gimp_image_get_thumbnail_data (gint32  image_ID,
			       gint   *width,
			       gint   *height,
			       gint   *bpp)
{
  gint    ret_width;
  gint    ret_height;
  guchar *image_data;
  gint    data_size;

  _gimp_image_thumbnail (image_ID,
			 *width,
			 *height,
			 &ret_width,
			 &ret_height,
			 bpp,
			 &data_size,
			 &image_data);

  *width  = ret_width;
  *height = ret_height;

  return image_data;
}
