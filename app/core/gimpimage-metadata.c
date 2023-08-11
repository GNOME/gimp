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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <gexiv2/gexiv2.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpimage-metadata.h"
#include "gimpimage-private.h"
#include "gimpimage-rotate.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer-new.h"


/* public functions */


GimpMetadata *
gimp_image_get_metadata (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  return private->metadata;
}

void
gimp_image_set_metadata (GimpImage    *image,
                         GimpMetadata *metadata,
                         gboolean      push_undo)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (metadata != private->metadata)
    {
      if (push_undo)
        gimp_image_undo_push_image_metadata (image, NULL);

      g_set_object (&private->metadata, metadata);

      if (private->metadata)
        {
          gimp_image_metadata_update_pixel_size      (image);
          gimp_image_metadata_update_bits_per_sample (image);
          gimp_image_metadata_update_resolution      (image);
          gimp_image_metadata_update_colorspace      (image);
        }

      g_object_notify (G_OBJECT (image), "metadata");
    }
}

void
gimp_image_metadata_update_pixel_size (GimpImage *image)
{
  GimpMetadata *metadata;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  metadata = gimp_image_get_metadata (image);

  if (metadata)
    {
      gimp_metadata_set_pixel_size (metadata,
                                    gimp_image_get_width  (image),
                                    gimp_image_get_height (image));
    }
}

void
gimp_image_metadata_update_bits_per_sample (GimpImage *image)
{
  GimpMetadata *metadata;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  metadata = gimp_image_get_metadata (image);

  if (metadata)
    {
      switch (gimp_image_get_component_type (image))
        {
        case GIMP_COMPONENT_TYPE_U8:
          gimp_metadata_set_bits_per_sample (metadata, 8);
          break;

        case GIMP_COMPONENT_TYPE_U16:
        case GIMP_COMPONENT_TYPE_HALF:
          gimp_metadata_set_bits_per_sample (metadata, 16);
          break;

        case GIMP_COMPONENT_TYPE_U32:
        case GIMP_COMPONENT_TYPE_FLOAT:
          gimp_metadata_set_bits_per_sample (metadata, 32);
          break;

        case GIMP_COMPONENT_TYPE_DOUBLE:
          gimp_metadata_set_bits_per_sample (metadata, 64);
          break;
        }
    }
}

void
gimp_image_metadata_update_resolution (GimpImage *image)
{
  GimpMetadata *metadata;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  metadata = gimp_image_get_metadata (image);

  if (metadata)
    {
      gdouble xres, yres;

      gimp_image_get_resolution (image, &xres, &yres);
      gimp_metadata_set_resolution (metadata, xres, yres,
                                    gimp_image_get_unit (image));
    }
}

void
gimp_image_metadata_update_colorspace (GimpImage *image)
{
  GimpMetadata *metadata;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  metadata = gimp_image_get_metadata (image);

  if (metadata)
    {
      /*  See the discussions in issue #3532 and issue #301  */

      GimpColorProfile       *profile = gimp_image_get_color_profile (image);
      GimpMetadataColorspace  space   = GIMP_METADATA_COLORSPACE_UNSPECIFIED;

      if (profile)
        {
          static GimpColorProfile *adobe = NULL;

          if (! adobe)
            adobe = gimp_color_profile_new_rgb_adobe ();

          if (gimp_color_profile_is_equal (profile, adobe))
            space = GIMP_METADATA_COLORSPACE_ADOBERGB;
        }
      else
        {
          space = GIMP_METADATA_COLORSPACE_SRGB;
        }

      gimp_metadata_set_colorspace (metadata, space);
    }
}

/**
 * gimp_image_metadata_load_thumbnail:
 * @gimp:              The #Gimp object.
 * @file:              A #GFile image.
 * @full_image_width:  the width of the full image (not the thumbnail).
 * @full_image_height: the height of the full image (not the thumbnail).
 * @error:             Return location for error message
 *
 * Retrieves a thumbnail from metadata in @file if present.
 * It does not need to actually load the full image, only the metadata through
 * GExiv2, which makes it fast.
 *
 * Returns: (transfer none) (nullable): a #GimpImage of the @file thumbnail.
 */
GimpImage *
gimp_image_metadata_load_thumbnail (Gimp        *gimp,
                                    GFile       *file,
                                    gint        *full_image_width,
                                    gint        *full_image_height,
                                    const Babl **format,
                                    GError     **error)
{
  GimpMetadata *metadata;
  GInputStream *input_stream;
  GdkPixbuf    *pixbuf;
  guint8       *thumbnail_buffer;
  gint          thumbnail_size;
  GimpImage    *image = NULL;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  metadata = gimp_metadata_load_from_file (file, error);
  if (! metadata)
    return NULL;

  if (! gexiv2_metadata_get_exif_thumbnail (GEXIV2_METADATA (metadata),
                                            &thumbnail_buffer,
                                            &thumbnail_size))
    {
      g_object_unref (metadata);
      return NULL;
    }

  input_stream = g_memory_input_stream_new_from_data (thumbnail_buffer,
                                                      thumbnail_size,
                                                      (GDestroyNotify) g_free);
  pixbuf = gdk_pixbuf_new_from_stream (input_stream, NULL, error);
  g_object_unref (input_stream);

  if (pixbuf)
    {
      GimpLayer *layer;

      image = gimp_image_new (gimp,
                              gdk_pixbuf_get_width  (pixbuf),
                              gdk_pixbuf_get_height (pixbuf),
                              GIMP_RGB, GIMP_PRECISION_U8_NON_LINEAR);
      gimp_image_undo_disable (image);

      /* XXX This is possibly wrong, because an image of a given color space may
       * have a thumbnail stored in a different colorspace. This is even more
       * true with GIMP which always exports RGB thumbnails (see code in
       * gimp_image_metadata_save_filter()), even for say grayscale images.
       * Nevertheless other software may store thumbnail using the same
       * colorspace.
       */
      *format = gimp_pixbuf_get_format (pixbuf);
      layer = gimp_layer_new_from_pixbuf (pixbuf, image,
                                          gimp_image_get_layer_format (image, FALSE),
                                          /* No need to localize; this image is short-lived. */
                                          "Background",
                                          GIMP_OPACITY_OPAQUE,
                                          gimp_image_get_default_new_layer_mode (image));
      g_object_unref (pixbuf);

      gimp_image_add_layer (image, layer, NULL, 0, FALSE);

      gimp_image_apply_metadata_orientation (image, gimp_get_user_context (gimp), metadata, NULL);
    }

  /* This is the unoriented dimensions. Should we switch when there is a 90 or
   * 270 degree rotation? We don't actually know if the metadata orientation is
   * correct.
   */
  *full_image_width  = gexiv2_metadata_get_pixel_width (GEXIV2_METADATA (metadata));
  *full_image_height = gexiv2_metadata_get_pixel_height (GEXIV2_METADATA (metadata));

  if (*full_image_width < 1 || *full_image_height < 1)
    {
      /* Dimensions stored in metadata might be less accurate, yet it's still
       * informational.
       */
      *full_image_width  = gexiv2_metadata_try_get_metadata_pixel_width (GEXIV2_METADATA (metadata), NULL);
      *full_image_height = gexiv2_metadata_try_get_metadata_pixel_width (GEXIV2_METADATA (metadata), NULL);
    }
  if (*full_image_width < 1 || *full_image_height < 1)
    {
      *full_image_width  = 0;
      *full_image_height = 0;
    }

  g_object_unref (metadata);

  return image;
}
