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
  DESTROYED,
  ADDED_LAYER,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};

struct _GimpImagePrivate
{
  gint id;
};

static GHashTable *gimp_images = NULL;


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

static GParamSpec *props[N_PROPS]       = { NULL, };
static guint       signals[LAST_SIGNAL] = { 0 };

static void
gimp_image_class_init (GimpImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_image_set_property;
  object_class->get_property = gimp_image_get_property;

  /**
   * GimpImageClass::destroy:
   * @image: a #GimpImage
   *
   * This signal will be emitted when an image has been destroyed, just
   * before we g_object_unref() it. This image is now invalid, none of
   * its data can be accessed anymore, therefore all processing has to
   * be stopped immediately.
   * The only thing still feasible is to compare the image if you kept a
   * reference to identify images, or to run gimp_image_get_id().
   */
  signals[DESTROYED] =
    g_signal_new ("destroyed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, destroyed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * GimpImageClass::added-layer:
   * @image:    a #GimpImage
   * @layer:    the new #GimpLayer
   * @add_name: the string passed initially as @layer name. It may be
   *            different from the actual name @layer has now.
   *
   * This signal will be emitted just after a new layer has been added
   * to @image.
   */
  signals[ADDED_LAYER] =
    g_signal_new ("added-layer",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImageClass, new_layer),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 2,
                  GIMP_TYPE_LAYER,
                  G_TYPE_STRING);

  props[PROP_ID] =
    g_param_spec_int ("id",
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
  return image ? image->priv->id : -1;
}

/**
 * gimp_image_get_by_id:
 * @image_id: The image id.
 *
 * Returns: (nullable) (transfer none): a #GimpImage for @image_id or
 *          %NULL if @image_id does not represent a valid image.
 *          The object belongs to libgimp and you should not free it.
 *
 * Since: 3.0
 **/
GimpImage *
gimp_image_get_by_id (gint32 image_id)
{
  GimpImage *image = NULL;

  if (G_UNLIKELY (! gimp_images))
    gimp_images = g_hash_table_new_full (g_direct_hash,
                                         g_direct_equal,
                                         NULL,
                                         (GDestroyNotify) g_object_unref);

  if (! _gimp_image_is_valid (image_id))
    {
      g_hash_table_remove (gimp_images, GINT_TO_POINTER (image_id));
    }
  else
    {
      image = g_hash_table_lookup (gimp_images,
                                   GINT_TO_POINTER (image_id));

      if (! image)
        {
          image = g_object_new (GIMP_TYPE_IMAGE,
                                "id", image_id,
                                NULL);
          g_hash_table_insert (gimp_images,
                               GINT_TO_POINTER (image_id),
                               image);
        }
    }

  return image;
}

/**
 * gimp_image_list:
 *
 * Returns the list of images currently open.
 *
 * This procedure returns the list of images currently open in GIMP.
 *
 * Returns: (element-type GimpImage) (transfer container):
 *          The list of images currently open.
 *          The returned value must be freed with g_list_free(). Image
 *          elements belong to libgimp and must not be freed.
 **/
GList *
gimp_image_list (void)
{
  GList *images = NULL;
  gint  *ids;
  gint   num_images;
  gint   i;

  ids = _gimp_image_list (&num_images);
  for (i = 0; i < num_images; i++)
    images = g_list_prepend (images,
                             gimp_image_get_by_id (ids[i]));
  images = g_list_reverse (images);
  g_free (ids);

  return images;
}

/**
 * gimp_image_get_layers:
 * @image: The image.
 *
 * Returns the list of layers contained in the specified image.
 *
 * This procedure returns the list of layers contained in the specified
 * image. The order of layers is from topmost to bottommost.
 *
 * Returns: (element-type GimpImage) (transfer container):
 *          The list of layers contained in the image.
 *          The returned value must be freed with g_list_free(). Layer
 *          elements belong to libgimp and must not be freed.
 *
 * Since: 3.0
 **/
GList *
gimp_image_get_layers (GimpImage *image)
{
  GList *layers = NULL;
  gint  *ids;
  gint   num_layers;
  gint   i;

  ids = _gimp_image_get_layers (image, &num_layers);
  for (i = 0; i < num_layers; i++)
    layers = g_list_prepend (layers,
                             GIMP_LAYER (gimp_item_get_by_id (ids[i])));
  layers = g_list_reverse (layers);
  g_free (ids);

  return layers;
}

/**
 * gimp_image_get_channels:
 * @image: The image.
 *
 * Returns the list of channels contained in the specified image.
 *
 * This procedure returns the list of channels contained in the
 * specified image. This does not include the selection mask, or layer
 * masks. The order is from topmost to bottommost. Note that
 * \"channels\" are custom channels and do not include the image's
 * color components.
 *
 * Returns: (element-type GimpChannel) (transfer container):
 *          The list of channels contained in the image.
 *          The returned value must be freed with g_list_free(). Channel
 *          elements belong to libgimp and must not be freed.
 *
 * Since: 3.0
 **/
GList *
gimp_image_get_channels (GimpImage *image)
{
  GList *channels = NULL;
  gint  *ids;
  gint   num_channels;
  gint   i;

  ids = _gimp_image_get_channels (image, &num_channels);
  for (i = 0; i < num_channels; i++)
    channels = g_list_prepend (channels,
                               GIMP_CHANNEL (gimp_item_get_by_id (ids[i])));
  channels = g_list_reverse (channels);
  g_free (ids);

  return channels;
}

/**
 * gimp_image_get_vectors:
 * @image: The image.
 *
 * Returns the list of vectors contained in the specified image.
 *
 * This procedure returns the list of vectors contained in the
 * specified image.
 *
 * Returns: (element-type GimpVectors) (transfer container):
 *          The list of vectors contained in the image.
 *          The returned value must be freed with g_list_free(). Vectors
 *          elements belong to libgimp and must not be freed.
 *
 * Since: 3.0
 **/
GList *
gimp_image_get_vectors (GimpImage *image)
{
  GList *vectors = NULL;
  gint  *ids;
  gint   num_vectors;
  gint   i;

  ids = _gimp_image_get_vectors (image, &num_vectors);
  for (i = 0; i < num_vectors; i++)
    vectors = g_list_prepend (vectors,
                              GIMP_VECTORS (gimp_item_get_by_id (ids[i])));
  vectors = g_list_reverse (vectors);
  g_free (ids);

  return vectors;
}

/**
 * gimp_image_get_colormap:
 * @image:      The image.
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
gimp_image_get_colormap (GimpImage *image,
                         gint      *num_colors)
{
  gint    num_bytes;
  guchar *cmap;

  cmap = _gimp_image_get_colormap (image, &num_bytes);

  if (num_colors)
    *num_colors = num_bytes / 3;

  return cmap;
}

/**
 * gimp_image_set_colormap:
 * @image:      The image.
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
gimp_image_set_colormap (GimpImage    *image,
                         const guchar *colormap,
                         gint          num_colors)
{
  return _gimp_image_set_colormap (image, num_colors * 3, colormap);
}

/**
 * gimp_image_get_thumbnail_data:
 * @image:  The image.
 * @width:  (inout): The requested thumbnail width.
 * @height: (inout): The requested thumbnail height.
 * @bpp:    (out): The previews bpp.
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
gimp_image_get_thumbnail_data (GimpImage *image,
                               gint      *width,
                               gint      *height,
                               gint      *bpp)
{
  gint    ret_width;
  gint    ret_height;
  guchar *image_data;
  gint    data_size;

  _gimp_image_thumbnail (image,
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
 * @image:  the image ID
 * @width:  the requested thumbnail width  (<= 1024 pixels)
 * @height: the requested thumbnail height (<= 1024 pixels)
 * @alpha:  how to handle an alpha channel
 *
 * Retrieves a thumbnail pixbuf for the image identified by @image->priv->id.
 * The thumbnail will be not larger than the requested size.
 *
 * Returns: (transfer full): a new #GdkPixbuf
 *
 * Since: 2.2
 **/
GdkPixbuf *
gimp_image_get_thumbnail (GimpImage              *image,
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

  data = gimp_image_get_thumbnail_data (image,
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
 * @image: The image.
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
gimp_image_get_metadata (GimpImage *image)
{
  GimpMetadata *metadata = NULL;
  gchar        *metadata_string;

  metadata_string = _gimp_image_get_metadata (image);
  if (metadata_string)
    {
      metadata = gimp_metadata_deserialize (metadata_string);
      g_free (metadata_string);
    }

  return metadata;
}

/**
 * gimp_image_set_metadata:
 * @image:    The image.
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
gimp_image_set_metadata (GimpImage    *image,
                         GimpMetadata *metadata)
{
  gchar    *metadata_string = NULL;
  gboolean  success;

  if (metadata)
    metadata_string = gimp_metadata_serialize (metadata);

  success = _gimp_image_set_metadata (image, metadata_string);

  if (metadata_string)
    g_free (metadata_string);

  return success;
}


/* Internal API. */


void
_gimp_image_process_signal (gint32          image_id,
                            const gchar    *name,
                            GimpValueArray *params)
{
  GimpImage *image = NULL;

  if (! gimp_images)
    return;

  image = g_hash_table_lookup (gimp_images,
                               GINT_TO_POINTER (image_id));

  if (! image)
    /* No need to create images not referenced by the plug-in. */
    return;

  /* Below process image signals. */

  if (g_strcmp0 (name, "destroyed") == 0)
    {
      g_hash_table_steal (gimp_images,
                          GINT_TO_POINTER (image->priv->id));

      g_signal_emit (image, signals[DESTROYED], 0);
      g_object_unref (image);
    }
  else if (g_strcmp0 (name, "added-layer") == 0)
    {
      GimpItem    *layer;
      const gchar *name;
      GValue      *value;

      g_return_if_fail (gimp_value_array_length (params) == 2                          &&
                        GIMP_VALUE_HOLDS_LAYER_ID (gimp_value_array_index (params, 0)) &&
                        G_VALUE_HOLDS_STRING (gimp_value_array_index (params, 1)));

      value = gimp_value_array_index (params, 0);
      layer = gimp_item_get_by_id (gimp_value_get_layer_id (value));
      value = gimp_value_array_index (params, 1);
      name  = g_value_get_string (value);

      g_signal_emit (image, signals[ADDED_LAYER], 0, layer, name);
    }
}


/* Deprecated API. */


/**
 * gimp_image_list_deprecated: (skip)
 * @num_images: (out): The number of images currently open.
 *
 * Returns the list of images currently open.
 *
 * This procedure returns the list of images currently open in GIMP.
 *
 * Returns: (array length=num_images) (element-type gint32) (transfer full):
 *          The list of images currently open.
 *          The returned value must be freed with g_free().
 **/
gint *
gimp_image_list_deprecated (gint *num_images)
{
  return _gimp_image_list (num_images);
}

/**
 * gimp_image_get_layers_deprecated: (skip)
 * @image_id: The image id.
 * @num_layers: (out): The number of layers contained in the image.
 *
 * Returns the list of layers contained in the specified image.
 *
 * This procedure returns the list of layers contained in the specified
 * image. The order of layers is from topmost to bottommost.
 *
 * Returns: (array length=num_layers) (element-type gint32) (transfer full):
 *          The list of layers contained in the image.
 *          The returned value must be freed with g_free().
 **/
gint *
gimp_image_get_layers_deprecated (gint32  image_id,
                                  gint   *num_layers)
{
  return _gimp_image_get_layers (gimp_image_get_by_id (image_id),
                                   num_layers);
}

/**
 * gimp_image_get_channels_deprecated: (skip)
 * @image_id: The image.
 * @num_channels: (out): The number of channels contained in the image.
 *
 * Returns the list of channels contained in the specified image.
 *
 * This procedure returns the list of channels contained in the
 * specified image. This does not include the selection mask, or layer
 * masks. The order is from topmost to bottommost. Note that
 * \"channels\" are custom channels and do not include the image's
 * color components.
 *
 * Returns: (array length=num_channels):
 *          The list of channels contained in the image.
 *          The returned value must be freed with g_free().
 **/
gint *
gimp_image_get_channels_deprecated (gint32  image_id,
                                    gint   *num_channels)
{
  return _gimp_image_get_layers (gimp_image_get_by_id (image_id),
                                 num_channels);
}

/**
 * gimp_image_get_vectors_deprecated: (skip)
 * @image_id: The image.
 * @num_vectors: (out): The number of vectors contained in the image.
 *
 * Returns the list of vectors contained in the specified image.
 *
 * This procedure returns the list of vectors contained in the
 * specified image.
 *
 * Returns: (array length=num_vectors) (element-type gint32) (transfer full):
 *          The list of vectors contained in the image.
 *          The returned value must be freed with g_free().
 *
 * Since: 2.4
 **/
gint *
gimp_image_get_vectors_deprecated (gint32  image_id,
                                   gint   *num_vectors)
{
  return _gimp_image_get_vectors (gimp_image_get_by_id (image_id),
                                  num_vectors);
}

/**
 * gimp_image_get_colormap_deprecated: (skip)
 * @image_id:   The image.
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
gimp_image_get_colormap_deprecated (gint32  image_id,
                                    gint   *num_colors)
{
  return gimp_image_get_colormap (gimp_image_get_by_id (image_id),
                                  num_colors);
}

/**
 * gimp_image_set_colormap_deprecated: (skip)
 * @image_id:   The image.
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
gimp_image_set_colormap_deprecated (gint32        image_id,
                                    const guchar *colormap,
                                    gint          num_colors)
{
  return gimp_image_set_colormap (gimp_image_get_by_id (image_id),
                                  colormap, num_colors);
}

/**
 * gimp_image_get_thumbnail_data_deprecated: (skip)
 * @image_id: The image.
 * @width:   (inout): The requested thumbnail width.
 * @height:  (inout): The requested thumbnail height.
 * @bpp:     (out): The previews bpp.
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
gimp_image_get_thumbnail_data_deprecated (gint32  image_id,
                                          gint   *width,
                                          gint   *height,
                                          gint   *bpp)
{
  return gimp_image_get_thumbnail_data (gimp_image_get_by_id (image_id),
                                        width, height, bpp);
}

/**
 * gimp_image_get_thumbnail_deprecated: (skip)
 * @image_id: the image ID
 * @width:    the requested thumbnail width  (<= 1024 pixels)
 * @height:   the requested thumbnail height (<= 1024 pixels)
 * @alpha:    how to handle an alpha channel
 *
 * Retrieves a thumbnail pixbuf for the image identified by @image->priv->id.
 * The thumbnail will be not larger than the requested size.
 *
 * Returns: (transfer full): a new #GdkPixbuf
 *
 * Since: 2.2
 **/
GdkPixbuf *
gimp_image_get_thumbnail_deprecated (gint32                 image_id,
                                     gint                   width,
                                     gint                   height,
                                     GimpPixbufTransparency alpha)
{
  return gimp_image_get_thumbnail (gimp_image_get_by_id (image_id),
                                   width, height, alpha);
}

/**
 * gimp_image_get_metadata_deprecated: (skip)
 * @image_id: The image.
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
gimp_image_get_metadata_deprecated (gint32 image_id)
{
  return gimp_image_get_metadata (gimp_image_get_by_id (image_id));
}

/**
 * gimp_image_set_metadata_deprecated: (skip)
 * @image_id: The image.
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
gimp_image_set_metadata_deprecated (gint32        image_id,
                                    GimpMetadata *metadata)
{
  return gimp_image_set_metadata (gimp_image_get_by_id (image_id),
                                  metadata);
}
