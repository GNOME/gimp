/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmaimagemetadata.c
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

#include <string.h>
#include <sys/time.h>

#include <gexiv2/gexiv2.h>

#include "ligma.h"
#include "ligmaimagemetadata.h"

#include "libligma-intl.h"


typedef struct
{
  gchar *tag;
  gint  type;
} XmpStructs;

static gchar *     ligma_image_metadata_interpret_comment    (gchar *comment);

static void        ligma_image_metadata_rotate               (LigmaImage         *image,
                                                             GExiv2Orientation  orientation);

/*  public functions  */

/**
 * ligma_image_metadata_load_prepare:
 * @image:     The image
 * @mime_type: The loaded file's mime-type
 * @file:      The file to load the metadata from
 * @error:     Return location for error
 *
 * Loads and returns metadata from @file to be passed into
 * ligma_image_metadata_load_finish().
 *
 * Returns: (transfer full): The file's metadata.
 *
 * Since: 2.10
 */
LigmaMetadata *
ligma_image_metadata_load_prepare (LigmaImage    *image,
                                  const gchar  *mime_type,
                                  GFile        *file,
                                  GError      **error)
{
  LigmaMetadata *metadata;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  metadata = ligma_metadata_load_from_file (file, error);

  return metadata;
}

static gchar *
ligma_image_metadata_interpret_comment (gchar *comment)
{
  /* Exiv2 can return unwanted text at the start of a comment
   * taken from Exif.Photo.UserComment since 0.27.3.
   * Let's remove that part and return NULL if there
   * is nothing else left as comment. */

  if (comment)
    {
      if (g_str_has_prefix (comment, "charset=Ascii "))
        {
          gchar *real_comment;

          /* Skip "charset=Ascii " (length 14) to find the real comment */
          real_comment = g_strdup (comment + 14);
          g_free (comment);
          comment = real_comment;
        }

#warning TODO: remove second "binary comment" condition when Exiv2 minimum requirement >= 0.27.4

      if (comment[0] == '\0' ||
          /* TODO: this second test is ugly as hell and should be
           * removed once our Exiv2 dependency is bumped to 0.27.4.
           *
           * Basically Exiv2 used to return "binary comment" for some
           * comments, even just a comment filled of 0s (instead of
           * returning the empty string). This has recently be reverted.
           * For now, let's ignore such "comment", though this is weak
           * too. What if the comment actually contained the "binary
           * comment" text? (which would be weird anyway and possibly a
           * result of the previously bugged implementation, so let's
           * accept the weakness as we can't do anything to distinguish
           * the 2 cases)
           * See commit 9b510858 in Exiv2 repository.
           */
          ! g_strcmp0 (comment, "binary comment"))
        {
          /* Removing an empty comment.*/
          g_free (comment);
          return NULL;
        }
    }

  return comment;
}

/**
 * ligma_image_metadata_load_finish:
 * @image:       The image
 * @mime_type:   The loaded file's mime-type
 * @metadata:    The metadata to set on the image
 * @flags:       Flags to specify what of the metadata to apply to the image
 *
 * Applies the @metadata previously loaded with
 * ligma_image_metadata_load_prepare() to the image, taking into account
 * the passed @flags.
 *
 * Since: 3.0
 */
void
ligma_image_metadata_load_finish (LigmaImage             *image,
                                 const gchar           *mime_type,
                                 LigmaMetadata          *metadata,
                                 LigmaMetadataLoadFlags  flags)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (mime_type != NULL);
  g_return_if_fail (GEXIV2_IS_METADATA (metadata));

  if (flags & LIGMA_METADATA_LOAD_COMMENT)
    {
      gchar  *comment;
      GError *error = NULL;

      comment = gexiv2_metadata_try_get_tag_interpreted_string (GEXIV2_METADATA (metadata),
                                                                "Exif.Photo.UserComment",
                                                                &error);
      if (error)
        {
          /* XXX. Should this be rather a user-facing error? */
          g_printerr ("%s: unreadable '%s' metadata tag: %s\n",
                      G_STRFUNC, "Exif.Photo.UserComment", error->message);
          g_clear_error (&error);
        }
      else if (comment)
        {
          comment = ligma_image_metadata_interpret_comment (comment);
        }

      if (! comment)
        {
          comment = gexiv2_metadata_try_get_tag_interpreted_string (GEXIV2_METADATA (metadata),
                                                                    "Exif.Image.ImageDescription",
                                                                    &error);
          if (error)
            {
              g_printerr ("%s: unreadable '%s' metadata tag: %s\n",
                          G_STRFUNC, "Exif.Image.ImageDescription", error->message);
              g_clear_error (&error);
            }
        }

      if (comment)
        {
          LigmaParasite *parasite;

          parasite = ligma_parasite_new ("ligma-comment",
                                        LIGMA_PARASITE_PERSISTENT,
                                        strlen (comment) + 1,
                                        comment);
          g_free (comment);

          ligma_image_attach_parasite (image, parasite);
          ligma_parasite_free (parasite);
        }
    }

  if (flags & LIGMA_METADATA_LOAD_RESOLUTION)
    {
      gdouble   xres;
      gdouble   yres;
      LigmaUnit  unit;

      if (ligma_metadata_get_resolution (metadata, &xres, &yres, &unit))
        {
          ligma_image_set_resolution (image, xres, yres);
          ligma_image_set_unit (image, unit);
        }
    }

  if (! (flags & LIGMA_METADATA_LOAD_ORIENTATION))
    {
      gexiv2_metadata_set_orientation (GEXIV2_METADATA (metadata),
                                       GEXIV2_ORIENTATION_UNSPECIFIED);
    }

  if (flags & LIGMA_METADATA_LOAD_COLORSPACE)
    {
      LigmaColorProfile *profile = ligma_image_get_color_profile (image);

      /* only look for colorspace information from metadata if the
       * image didn't contain an embedded color profile
       */
      if (! profile)
        {
          LigmaMetadataColorspace colorspace;

          colorspace = ligma_metadata_get_colorspace (metadata);

          switch (colorspace)
            {
            case LIGMA_METADATA_COLORSPACE_UNSPECIFIED:
            case LIGMA_METADATA_COLORSPACE_UNCALIBRATED:
            case LIGMA_METADATA_COLORSPACE_SRGB:
              /* use sRGB, a NULL profile will do the right thing  */
              break;

            case LIGMA_METADATA_COLORSPACE_ADOBERGB:
              profile = ligma_color_profile_new_rgb_adobe ();
              break;
            }

          if (profile)
            ligma_image_set_color_profile (image, profile);
        }

      if (profile)
        g_object_unref (profile);
    }

  ligma_image_set_metadata (image, metadata);
}

/**
 * ligma_image_metadata_load_thumbnail:
 * @file:  A #GFile image
 * @error: Return location for error message
 *
 * Retrieves a thumbnail from metadata if present.
 *
 * Returns: (transfer none) (nullable): a #LigmaImage of the @file thumbnail.
 *
 * Since: 2.10
 */
LigmaImage *
ligma_image_metadata_load_thumbnail (GFile   *file,
                                    GError **error)
{
  LigmaMetadata *metadata;
  GInputStream *input_stream;
  GdkPixbuf    *pixbuf;
  guint8       *thumbnail_buffer;
  gint          thumbnail_size;
  LigmaImage    *image = NULL;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  metadata = ligma_metadata_load_from_file (file, error);
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
      LigmaLayer *layer;

      image = ligma_image_new (gdk_pixbuf_get_width  (pixbuf),
                              gdk_pixbuf_get_height (pixbuf),
                              LIGMA_RGB);
      ligma_image_undo_disable (image);

      layer = ligma_layer_new_from_pixbuf (image, _("Background"),
                                          pixbuf,
                                          100.0,
                                          ligma_image_get_default_new_layer_mode (image),
                                          0.0, 0.0);
      g_object_unref (pixbuf);

      ligma_image_insert_layer (image, layer, NULL, 0);

      ligma_image_metadata_rotate (image,
                                  gexiv2_metadata_get_orientation (GEXIV2_METADATA (metadata)));
    }

  g_object_unref (metadata);

  return image;
}


/*  private functions  */

static void
ligma_image_metadata_rotate (LigmaImage         *image,
                            GExiv2Orientation  orientation)
{
  switch (orientation)
    {
    case GEXIV2_ORIENTATION_UNSPECIFIED:
    case GEXIV2_ORIENTATION_NORMAL:  /* standard orientation, do nothing */
      break;

    case GEXIV2_ORIENTATION_HFLIP:
      ligma_image_flip (image, LIGMA_ORIENTATION_HORIZONTAL);
      break;

    case GEXIV2_ORIENTATION_ROT_180:
      ligma_image_rotate (image, LIGMA_ROTATE_180);
      break;

    case GEXIV2_ORIENTATION_VFLIP:
      ligma_image_flip (image, LIGMA_ORIENTATION_VERTICAL);
      break;

    case GEXIV2_ORIENTATION_ROT_90_HFLIP:  /* flipped diagonally around '\' */
      ligma_image_rotate (image, LIGMA_ROTATE_90);
      ligma_image_flip (image, LIGMA_ORIENTATION_HORIZONTAL);
      break;

    case GEXIV2_ORIENTATION_ROT_90:  /* 90 CW */
      ligma_image_rotate (image, LIGMA_ROTATE_90);
      break;

    case GEXIV2_ORIENTATION_ROT_90_VFLIP:  /* flipped diagonally around '/' */
      ligma_image_rotate (image, LIGMA_ROTATE_90);
      ligma_image_flip (image, LIGMA_ORIENTATION_VERTICAL);
      break;

    case GEXIV2_ORIENTATION_ROT_270:  /* 90 CCW */
      ligma_image_rotate (image, LIGMA_ROTATE_270);
      break;

    default: /* shouldn't happen */
      break;
    }
}
