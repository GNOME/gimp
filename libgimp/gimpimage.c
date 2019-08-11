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

#include "gimppixbuf.h"

enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};

struct _GimpImagePrivate
{
  gint32 id;
};

static void       gimp_image_set_property  (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void       gimp_image_get_property  (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);

G_DEFINE_TYPE_WITH_PRIVATE (GimpImage, gimp_image, G_TYPE_OBJECT)

#define parent_class gimp_image_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };

static void
gimp_image_class_init (GimpImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_image_set_property;
  object_class->get_property = gimp_image_get_property;

  props[PROP_ID] =
    gimp_param_spec_int32 ("id",
                           "The image id",
                           "The image id for internal use",
                           0, G_MAXINT32, 0,
                           GIMP_PARAM_READWRITE |
                           G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_image_init (GimpImage *image)
{
  image->priv = gimp_image_get_instance_private (image);
}

static void
gimp_image_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpImage *image = GIMP_IMAGE (object);

  switch (property_id)
    {
    case PROP_ID:
      image->priv->id = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GimpImage *image = GIMP_IMAGE (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, image->priv->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public API. */


/**
 * gimp_image_get_id:
 * @image: The image.
 *
 * Returns: the image ID.
 *
 * Since: 3.0
 **/
gint32
gimp_image_get_id (GimpImage *image)
{
  return image->priv->id;
}

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

/**
 * gimp_image_get_thumbnail_data:
 * @image_ID: The image.
 * @width:    (inout): The requested thumbnail width.
 * @height:   (inout): The requested thumbnail height.
 * @bpp:      (out): The previews bpp.
 *
 * Get a thumbnail of an image.
 *
 * This function gets data from which a thumbnail of an image preview
 * can be created. Maximum x or y dimension is 1024 pixels. The pixels
 * are returned in RGB[A] or GRAY[A] format. The bpp return value
 * gives the number of bytes per pixel in the image.
 *
 * Returns: (transfer full): the thumbnail data.
 **/
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
 * gimp_image_get_thumbnail:
 * @image_ID: the image ID
 * @width:    the requested thumbnail width  (<= 1024 pixels)
 * @height:   the requested thumbnail height (<= 1024 pixels)
 * @alpha:    how to handle an alpha channel
 *
 * Retrieves a thumbnail pixbuf for the image identified by @image_ID.
 * The thumbnail will be not larger than the requested size.
 *
 * Returns: (transfer full): a new #GdkPixbuf
 *
 * Since: 2.2
 **/
GdkPixbuf *
gimp_image_get_thumbnail (gint32                  image_ID,
                          gint                    width,
                          gint                    height,
                          GimpPixbufTransparency  alpha)
{
  gint    thumb_width  = width;
  gint    thumb_height = height;
  gint    thumb_bpp;
  guchar *data;

  g_return_val_if_fail (width  > 0 && width  <= 1024, NULL);
  g_return_val_if_fail (height > 0 && height <= 1024, NULL);

  data = gimp_image_get_thumbnail_data (image_ID,
                                        &thumb_width,
                                        &thumb_height,
                                        &thumb_bpp);
  if (data)
    return _gimp_pixbuf_from_data (data,
                                   thumb_width, thumb_height, thumb_bpp,
                                   alpha);
  else
    return NULL;
}

/**
 * gimp_image_get_metadata:
 * @image_ID: The image.
 *
 * Returns the image's metadata.
 *
 * Returns exif/iptc/xmp metadata from the image.
 *
 * Returns: (nullable) (transfer full): The exif/ptc/xmp metadata,
 *          or %NULL if there is none.
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
