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

#include "libgimpbase/gimpwire.h" /* FIXME kill this include */

#include "gimppixbuf.h"
#include "gimpplugin-private.h"
#include "gimpprocedure-private.h"


enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};

struct _GimpImage
{
  GObject parent_instance;
  gint    id;
};


static void   gimp_image_set_property  (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec);
static void   gimp_image_get_property  (GObject      *object,
                                        guint         property_id,
                                        GValue       *value,
                                        GParamSpec   *pspec);


G_DEFINE_TYPE (GimpImage, gimp_image, G_TYPE_OBJECT)

#define parent_class gimp_image_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
gimp_image_class_init (GimpImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_image_set_property;
  object_class->get_property = gimp_image_get_property;

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
      image->id = g_value_get_int (value);
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
      g_value_set_int (value, image->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public API */

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
  return image ? image->id : -1;
}

/**
 * gimp_image_get_by_id:
 * @image_id: The image id.
 *
 * Returns: (nullable) (transfer none): a #GimpImage for @image_id or
 *          %NULL if @image_id does not represent a valid image.
 *          The object belongs to libgimp and you must not modify
 *          or unref it.
 *
 * Since: 3.0
 **/
GimpImage *
gimp_image_get_by_id (gint32 image_id)
{
  if (image_id > 0)
    {
      GimpPlugIn    *plug_in   = gimp_get_plug_in ();
      GimpProcedure *procedure = _gimp_plug_in_get_procedure (plug_in);

      return _gimp_procedure_get_image (procedure, image_id);
    }

  return NULL;
}

/**
 * gimp_image_is_valid:
 * @image: The image to check.
 *
 * Returns TRUE if the image is valid.
 *
 * This procedure checks if the given image is valid and refers to
 * an existing image.
 *
 * Returns: Whether the image is valid.
 *
 * Since: 2.4
 **/
gboolean
gimp_image_is_valid (GimpImage *image)
{
  return gimp_image_id_is_valid (gimp_image_get_id (image));
}

/**
 * gimp_list_images: (skip)
 *
 * Returns the list of images currently open.
 *
 * This procedure returns the list of images currently open in GIMP.
 *
 * Returns: (element-type GimpImage) (transfer container):
 *          The list of images currently open.
 *          The returned list must be freed with g_list_free(). Image
 *          elements belong to libgimp and must not be freed.
 *
 * Since: 3.0
 **/
GList *
gimp_list_images (void)
{
  GimpImage **images;
  gint        num_images;
  GList      *list = NULL;
  gint        i;

  images = gimp_get_images (&num_images);

  for (i = 0; i < num_images; i++)
    list = g_list_prepend (list, images[i]);

  g_free (images);

  return g_list_reverse (list);
}

/**
 * gimp_image_list_layers: (skip)
 * @image: The image.
 *
 * Returns the list of layers contained in the specified image.
 *
 * This procedure returns the list of layers contained in the specified
 * image. The order of layers is from topmost to bottommost.
 *
 * Returns: (element-type GimpLayer) (transfer container):
 *          The list of layers contained in the image.
 *          The returned list must be freed with g_list_free(). Layer
 *          elements belong to libgimp and must not be freed.
 *
 * Since: 3.0
 **/
GList *
gimp_image_list_layers (GimpImage *image)
{
  GimpLayer **layers;
  gint        num_layers;
  GList      *list = NULL;
  gint        i;

  layers = gimp_image_get_layers (image, &num_layers);

  for (i = 0; i < num_layers; i++)
    list = g_list_prepend (list, layers[i]);

  g_free (layers);

  return g_list_reverse (list);
}

/**
 * gimp_image_list_selected_layers: (skip)
 * @image: The image.
 *
 * Returns the list of layers selected in the specified image.
 *
 * This procedure returns the list of layers selected in the specified
 * image.
 *
 * Returns: (element-type GimpLayer) (transfer container):
 *          The list of selected layers in the image.
 *          The returned list must be freed with g_list_free(). Layer
 *          elements belong to libgimp and must not be freed.
 *
 * Since: 3.0
 **/
GList *
gimp_image_list_selected_layers (GimpImage *image)
{
  GimpLayer **layers;
  gint        num_layers;
  GList      *list = NULL;
  gint        i;

  layers = gimp_image_get_selected_layers (image, &num_layers);

  for (i = 0; i < num_layers; i++)
    list = g_list_prepend (list, layers[i]);

  g_free (layers);

  return g_list_reverse (list);
}

/**
 * gimp_image_take_selected_layers:
 * @image: The image.
 * @layers: (transfer container) (element-type GimpLayer): The list of layers to select.
 *
 * The layers are set as the selected layers in the image. Any previous
 * selected layers or channels are unselected. An exception is a previously
 * existing floating selection, in which case this procedure will return an
 * execution error.
 *
 * Returns: TRUE on success.
 *
 * Since: 3.0
 **/
gboolean
gimp_image_take_selected_layers (GimpImage *image,
                                 GList     *layers)
{
  GimpLayer **sel_layers;
  GList      *list;
  gboolean    success;
  gint        i;

  sel_layers = g_new0 (GimpLayer *, g_list_length (layers));
  for (list = layers, i = 0; list; list = list->next, i++)
    sel_layers[i] = list->data;

  success = gimp_image_set_selected_layers (image, g_list_length (layers),
                                            (const GimpLayer **) sel_layers);
  g_list_free (layers);
  g_free (sel_layers);

  return success;
}

/**
 * gimp_image_list_selected_channels: (skip)
 * @image: The image.
 *
 * Returns the list of channels selected in the specified image.
 *
 * This procedure returns the list of channels selected in the specified
 * image.
 *
 * Returns: (element-type GimpChannel) (transfer container):
 *          The list of selected channels in the image.
 *          The returned list must be freed with g_list_free(). Layer
 *          elements belong to libgimp and must not be freed.
 *
 * Since: 3.0
 **/
GList *
gimp_image_list_selected_channels (GimpImage *image)
{
  GimpChannel **channels;
  gint          num_channels;
  GList        *list = NULL;
  gint          i;

  channels = gimp_image_get_selected_channels (image, &num_channels);

  for (i = 0; i < num_channels; i++)
    list = g_list_prepend (list, channels[i]);

  g_free (channels);

  return g_list_reverse (list);
}

/**
 * gimp_image_take_selected_channels:
 * @image: The image.
 * @channels: (transfer container) (element-type GimpChannel): The list of channels to select.
 *
 * The channels are set as the selected channels in the image. Any previous
 * selected layers or channels are unselected. An exception is a previously
 * existing floating selection, in which case this procedure will return an
 * execution error.
 *
 * Returns: TRUE on success.
 *
 * Since: 3.0
 **/
gboolean
gimp_image_take_selected_channels (GimpImage *image,
                                   GList     *channels)
{
  GimpChannel **sel_channels;
  GList        *list;
  gboolean      success;
  gint          i;

  sel_channels = g_new0 (GimpChannel *, g_list_length (channels));
  for (list = channels, i = 0; list; list = list->next, i++)
    sel_channels[i] = list->data;

  success = gimp_image_set_selected_channels (image, g_list_length (channels),
                                              (const GimpChannel **) sel_channels);
  g_list_free (channels);

  return success;
}

/**
 * gimp_image_list_selected_paths: (skip)
 * @image: The image.
 *
 * Returns the list of paths selected in the specified image.
 *
 * This procedure returns the list of paths selected in the specified
 * image.
 *
 * Returns: (element-type GimpPath) (transfer container):
 *          The list of selected paths in the image.
 *          The returned list must be freed with g_list_free().
 *          Path elements belong to libgimp and must not be freed.
 *
 * Since: 3.0
 **/
GList *
gimp_image_list_selected_paths (GimpImage *image)
{
  GimpPath **paths;
  gint       num_paths;
  GList     *list = NULL;
  gint       i;

  paths = gimp_image_get_selected_paths (image, &num_paths);

  for (i = 0; i < num_paths; i++)
    list = g_list_prepend (list, paths[i]);

  g_free (paths);

  return g_list_reverse (list);
}

/**
 * gimp_image_take_selected_paths:
 * @image: The image.
 * @paths: (transfer container) (element-type GimpPath): The list of paths to select.
 *
 * The paths are set as the selected paths in the image. Any previous
 * selected paths are unselected.
 *
 * Returns: TRUE on success.
 *
 * Since: 3.0
 **/
gboolean
gimp_image_take_selected_paths (GimpImage *image,
                                GList     *paths)
{
  GimpPath **sel_paths;
  GList     *list;
  gboolean   success;
  gint       i;

  sel_paths = g_new0 (GimpPath *, g_list_length (paths));
  for (list = paths, i = 0; list; list = list->next, i++)
    sel_paths[i] = list->data;

  success = gimp_image_set_selected_paths (image, g_list_length (paths),
                                           (const GimpPath **) sel_paths);
  g_list_free (paths);

  return success;
}

/**
 * gimp_image_list_channels: (skip)
 * @image: The image.
 *
 * Returns the list of channels contained in the specified image.
 *
 * This procedure returns the list of channels contained in the
 * specified image. This does not include the selection mask, or layer
 * masks. The order is from topmost to bottommost. Note that
 * "channels" are custom channels and do not include the image's
 * color components.
 *
 * Returns: (element-type GimpChannel) (transfer container):
 *          The list of channels contained in the image.
 *          The returned list must be freed with g_list_free(). Channel
 *          elements belong to libgimp and must not be freed.
 *
 * Since: 3.0
 **/
GList *
gimp_image_list_channels (GimpImage *image)
{
  GimpChannel **channels;
  gint          num_channels;
  GList        *list = NULL;
  gint          i;

  channels = gimp_image_get_channels (image, &num_channels);

  for (i = 0; i < num_channels; i++)
    list = g_list_prepend (list, channels[i]);

  g_free (channels);

  return g_list_reverse (list);
}

/**
 * gimp_image_list_paths: (skip)
 * @image: The image.
 *
 * Returns the list of paths contained in the specified image.
 *
 * This procedure returns the list of paths contained in the
 * specified image.
 *
 * Returns: (element-type GimpPath) (transfer container):
 *          The list of paths contained in the image.
 *          The returned value must be freed with g_list_free(). Path
 *          elements belong to libgimp and must not be freed.
 *
 * Since: 3.0
 **/
GList *
gimp_image_list_paths (GimpImage *image)
{
  GimpPath **paths;
  gint       num_paths;
  GList     *list = NULL;
  gint       i;

  paths = gimp_image_get_paths (image, &num_paths);

  for (i = 0; i < num_paths; i++)
    list = g_list_prepend (list, paths[i]);

  g_free (paths);

  return g_list_reverse (list);
}

/**
 * gimp_image_list_selected_drawables: (skip)
 * @image: The image.
 *
 * Returns the list of drawables selected in the specified image.
 *
 * This procedure returns the list of drawables selected in the specified
 * image.
 * These can be either a list of layers or a list of channels (a list mixing
 * layers and channels is not possible), or it can be a layer mask (a list
 * containing only a layer mask as single item), if a layer mask is in edit
 * mode.
 *
 * Returns: (element-type GimpItem) (transfer container):
 *          The list of selected drawables in the image.
 *          The returned list must be freed with g_list_free(). Layer
 *          elements belong to libgimp and must not be freed.
 *
 * Since: 3.0
 **/
GList *
gimp_image_list_selected_drawables (GimpImage *image)
{
  GimpItem **drawables;
  gint       num_drawables;
  GList     *list = NULL;
  gint       i;

  drawables = gimp_image_get_selected_drawables (image, &num_drawables);

  for (i = 0; i < num_drawables; i++)
    list = g_list_prepend (list, drawables[i]);

  g_free (drawables);

  return g_list_reverse (list);
}

/**
 * gimp_image_get_colormap:
 * @image:      The image.
 * @colormap_len: (out) (optional): The size (in bytes) of the returned colormap
 * @num_colors: (out) (optional): Returns the number of colors in the colormap array.
 *
 * Returns the image's colormap
 *
 * This procedure returns an actual pointer to the image's colormap, as
 * well as the number of colors contained in the colormap. If the image
 * is not of base type INDEXED, this pointer will be NULL.
 *
 * Returns: (array length=colormap_len): The image's colormap.
 */
guchar *
gimp_image_get_colormap (GimpImage *image,
                         gint      *colormap_len,
                         gint      *num_colors)
{
  GBytes *bytes;
  gsize   num_bytes;
  guchar *cmap;

  bytes = _gimp_image_get_colormap (image);
  cmap = g_bytes_unref_to_data (bytes, &num_bytes);

  if (colormap_len)
    *colormap_len = num_bytes;
  if (num_colors)
    *num_colors = num_bytes / 3;

  return cmap;
}

/**
 * gimp_image_set_colormap:
 * @image:      The image.
 * @colormap: (array): The new colormap values.
 * @num_colors: Number of colors in the colormap array.
 *
 * Sets the entries in the image's colormap.
 *
 * This procedure sets the entries in the specified image's colormap.
 * The number of colors is specified by the "num_colors" parameter
 * and corresponds to the number of INT8 triples that must be contained
 * in the "cmap" array.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_image_set_colormap (GimpImage    *image,
                         const guchar *colormap,
                         gint          num_colors)
{
  GBytes *bytes;
  gboolean ret;

  bytes = g_bytes_new_static (colormap, num_colors * 3);
  ret = _gimp_image_set_colormap (image, bytes);
  g_bytes_unref (bytes);

  return ret;
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
GBytes *
gimp_image_get_thumbnail_data (GimpImage *image,
                               gint      *width,
                               gint      *height,
                               gint      *bpp)
{
  GBytes *image_bytes;

  _gimp_image_thumbnail (image,
                         *width,
                         *height,
                         width,
                         height,
                         bpp,
                         &image_bytes);

  return image_bytes;
}

/**
 * gimp_image_get_thumbnail:
 * @image:  the #GimpImage
 * @width:  the requested thumbnail width  (<= 1024 pixels)
 * @height: the requested thumbnail height (<= 1024 pixels)
 * @alpha:  how to handle an alpha channel
 *
 * Retrieves a thumbnail pixbuf for @image.
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
  GBytes *data;
  gsize   data_size;

  g_return_val_if_fail (width  > 0 && width  <= 1024, NULL);
  g_return_val_if_fail (height > 0 && height <= 1024, NULL);

  data = gimp_image_get_thumbnail_data (image,
                                        &thumb_width,
                                        &thumb_height,
                                        &thumb_bpp);
  if (data)
    return _gimp_pixbuf_from_data (g_bytes_unref_to_data (data, &data_size),
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
