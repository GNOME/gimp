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
 * <https://www.gnu.org/licenses/>.
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

/**
 * gimp_image_get_cmap:
 * @image_ID:   The image.
 * @num_colors: Number of colors in the colormap array.
 *
 * Deprecated: Use gimp_image_get_colormap() instead.
 *
 * Returns: The image's colormap.
 */
guchar *
gimp_image_get_cmap (gint32  image_ID,
                     gint   *num_colors)
{
  return gimp_image_get_colormap (image_ID, num_colors);
}

/**
 * gimp_image_set_cmap:
 * @image_ID:   The image.
 * @cmap:       The new colormap values.
 * @num_colors: Number of colors in the colormap array.
 *
 * Deprecated: Use gimp_image_set_colormap() instead.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_image_set_cmap (gint32        image_ID,
                     const guchar *cmap,
                     gint          num_colors)
{
  return gimp_image_set_colormap (image_ID, cmap, num_colors);
}

/**
 * gimp_image_get_layer_position:
 * @image_ID: The image.
 * @layer_ID: The layer.
 *
 * Deprecated: Use gimp_image_get_item_position() instead.
 *
 * Returns: The position of the layer in the layer stack.
 *
 * Since: 2.4
 **/
gint
gimp_image_get_layer_position (gint32 image_ID,
                               gint32 layer_ID)
{
  return gimp_image_get_item_position (image_ID, layer_ID);
}

/**
 * gimp_image_raise_layer:
 * @image_ID: The image.
 * @layer_ID: The layer to raise.
 *
 * Deprecated: Use gimp_image_raise_item() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_image_raise_layer (gint32 image_ID,
                        gint32 layer_ID)
{
  return gimp_image_raise_item (image_ID, layer_ID);
}

/**
 * gimp_image_lower_layer:
 * @image_ID: The image.
 * @layer_ID: The layer to lower.
 *
 * Deprecated: Use gimp_image_lower_item() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_image_lower_layer (gint32 image_ID,
                        gint32 layer_ID)
{
  return gimp_image_lower_item (image_ID, layer_ID);
}

/**
 * gimp_image_raise_layer_to_top:
 * @image_ID: The image.
 * @layer_ID: The layer to raise to top.
 *
 * Deprecated: Use gimp_image_raise_item_to_top() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_image_raise_layer_to_top (gint32 image_ID,
                               gint32 layer_ID)
{
  return gimp_image_raise_item_to_top (image_ID, layer_ID);
}

/**
 * gimp_image_lower_layer_to_bottom:
 * @image_ID: The image.
 * @layer_ID: The layer to lower to bottom.
 *
 * Deprecated: Use gimp_image_lower_item_to_bottom() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_image_lower_layer_to_bottom (gint32 image_ID,
                                  gint32 layer_ID)
{
  return gimp_image_lower_item_to_bottom (image_ID, layer_ID);
}

/**
 * gimp_image_get_channel_position:
 * @image_ID: The image.
 * @channel_ID: The channel.
 *
 * Deprecated: Use gimp_image_get_item_position() instead.
 *
 * Returns: The position of the channel in the channel stack.
 *
 * Since: 2.4
 **/
gint
gimp_image_get_channel_position (gint32 image_ID,
                                 gint32 channel_ID)
{
  return gimp_image_get_item_position (image_ID, channel_ID);
}

/**
 * gimp_image_raise_channel:
 * @image_ID: The image.
 * @channel_ID: The channel to raise.
 *
 * Deprecated: Use gimp_image_raise_item() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_image_raise_channel (gint32 image_ID,
                          gint32 channel_ID)
{
  return gimp_image_raise_item (image_ID, channel_ID);
}

/**
 * gimp_image_lower_channel:
 * @image_ID: The image.
 * @channel_ID: The channel to lower.
 *
 * Deprecated: Use gimp_image_lower_item() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_image_lower_channel (gint32 image_ID,
                          gint32 channel_ID)
{
  return gimp_image_lower_item (image_ID, channel_ID);
}

/**
 * gimp_image_get_vectors_position:
 * @image_ID: The image.
 * @vectors_ID: The vectors object.
 *
 * Deprecated: Use gimp_image_get_item_position() instead.
 *
 * Returns: The position of the vectors object in the vectors stack.
 *
 * Since: 2.4
 **/
gint
gimp_image_get_vectors_position (gint32 image_ID,
                                 gint32 vectors_ID)
{
  return gimp_image_get_item_position (image_ID, vectors_ID);
}

/**
 * gimp_image_raise_vectors:
 * @image_ID: The image.
 * @vectors_ID: The vectors object to raise.
 *
 * Deprecated: Use gimp_image_raise_item() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
gimp_image_raise_vectors (gint32 image_ID,
                          gint32 vectors_ID)
{
  return gimp_image_raise_item (image_ID, vectors_ID);
}

/**
 * gimp_image_lower_vectors:
 * @image_ID: The image.
 * @vectors_ID: The vectors object to lower.
 *
 * Deprecated: Use gimp_image_lower_item() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
gimp_image_lower_vectors (gint32 image_ID,
                          gint32 vectors_ID)
{
  return gimp_image_lower_item (image_ID, vectors_ID);
}

/**
 * gimp_image_raise_vectors_to_top:
 * @image_ID: The image.
 * @vectors_ID: The vectors object to raise to top.
 *
 * Deprecated: Use gimp_image_raise_item_to_top() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
gimp_image_raise_vectors_to_top (gint32 image_ID,
                                 gint32 vectors_ID)
{
  return gimp_image_raise_item_to_top (image_ID, vectors_ID);
}

/**
 * gimp_image_lower_vectors_to_bottom:
 * @image_ID: The image.
 * @vectors_ID: The vectors object to lower to bottom.
 *
 * Deprecated: Use gimp_image_lower_item_to_bottom() instead.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
gimp_image_lower_vectors_to_bottom (gint32 image_ID,
                                    gint32 vectors_ID)
{
  return gimp_image_lower_item_to_bottom (image_ID, vectors_ID);
}

/**
 * gimp_image_parasite_find:
 * @image_ID: The image.
 * @name: The name of the parasite to find.
 *
 * Deprecated: Use gimp_image_get_parasite() instead.
 *
 * Returns: The found parasite.
 **/
GimpParasite *
gimp_image_parasite_find (gint32       image_ID,
                          const gchar *name)
{
  return gimp_image_get_parasite (image_ID, name);
}

/**
 * gimp_image_parasite_attach:
 * @image_ID: The image.
 * @parasite: The parasite to attach to an image.
 *
 * Deprecated: Use gimp_image_attach_parasite() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_image_parasite_attach (gint32              image_ID,
                            const GimpParasite *parasite)
{
  return gimp_image_attach_parasite (image_ID, parasite);
}

/**
 * gimp_image_parasite_detach:
 * @image_ID: The image.
 * @name: The name of the parasite to detach from an image.
 *
 * Deprecated: Use gimp_image_detach_parasite() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_image_parasite_detach (gint32       image_ID,
                            const gchar *name)
{
  return gimp_image_detach_parasite (image_ID, name);
}

/**
 * gimp_image_parasite_list:
 * @image_ID: The image.
 * @num_parasites: The number of attached parasites.
 * @parasites: The names of currently attached parasites.
 *
 * Deprecated: Use gimp_image_get_parasite_list() instead.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_image_parasite_list (gint32    image_ID,
                          gint     *num_parasites,
                          gchar  ***parasites)
{
  *parasites = gimp_image_get_parasite_list (image_ID, num_parasites);

  return *parasites != NULL;
}

/**
 * gimp_image_attach_new_parasite:
 * @image_ID: the ID of the image to attach the #GimpParasite to.
 * @name: the name of the #GimpParasite to create and attach.
 * @flags: the flags set on the #GimpParasite.
 * @size: the size of the parasite data in bytes.
 * @data: a pointer to the data attached with the #GimpParasite.
 *
 * Convenience function that creates a parasite and attaches it
 * to GIMP.
 *
 * Deprecated: Use gimp_image_attach_parasite() instead.
 *
 * Return value: TRUE on successful creation and attachment of
 * the new parasite.
 *
 * See Also: gimp_image_parasite_attach()
 */
gboolean
gimp_image_attach_new_parasite (gint32         image_ID,
                                const gchar   *name,
                                gint           flags,
                                gint           size,
                                gconstpointer  data)
{
  GimpParasite *parasite = gimp_parasite_new (name, flags, size, data);
  gboolean      success;

  success = gimp_image_attach_parasite (image_ID, parasite);

  gimp_parasite_free (parasite);

  return success;
}
