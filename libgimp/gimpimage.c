/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpimage.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"
#include "gimpimage.h"

/**
 * gimp_image_get_colormap:
 * @image_ID:   The image.
 * @num_colors: Returns the number of colors in the colormap array.
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
gimp_image_get_colormap (gint32  image_ID,
                         gint   *num_colors)
{
  gint    num_bytes;
  guchar *cmap;

  cmap = _gimp_image_get_colormap (image_ID, &num_bytes);

  if (num_colors)
    *num_colors = num_bytes / 3;

  return cmap;
}

/**
 * gimp_image_set_colormap:
 * @image_ID:   The image.
 * @colormap:   The new colormap values.
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
gimp_image_set_colormap (gint32        image_ID,
                         const guchar *colormap,
                         gint          num_colors)
{
  return _gimp_image_set_colormap (image_ID, num_colors * 3, colormap);
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

/**
 * gimp_image_get_metadata:
 * @image_ID: The image.
 *
 * Returns the image's metadata.
 *
 * Returns exif/iptc/xmp metadata from the image.
 *
 * Returns: The exif/ptc/xmp metadata, or %NULL if there is none.
 *
 * Since: 2.10
 **/
GimpMetadata *
gimp_image_get_metadata (gint32 image_ID)
{
  GimpMetadata *metadata = NULL;
  gchar        *metadata_string;

  metadata_string = _gimp_image_get_metadata (image_ID);
  if (metadata_string)
    {
      metadata = gimp_metadata_deserialize (metadata_string);
      g_free (metadata_string);
    }

  return metadata;
}

/**
 * gimp_image_set_metadata:
 * @image_ID: The image.
 * @metadata: The exif/ptc/xmp metadata.
 *
 * Set the image's metadata.
 *
 * Sets exif/iptc/xmp metadata on the image, or deletes it if
 * @metadata is %NULL.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
gimp_image_set_metadata (gint32        image_ID,
                         GimpMetadata *metadata)
{
  gchar    *metadata_string = NULL;
  gboolean  success;

  if (metadata)
    metadata_string = gimp_metadata_serialize (metadata);

  success = _gimp_image_set_metadata (image_ID, metadata_string);

  if (metadata_string)
    g_free (metadata_string);

  return success;
}
