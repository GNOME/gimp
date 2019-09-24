/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpimagemetadata.c
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

#include <gtk/gtk.h>
#include <gexiv2/gexiv2.h>

#include "gimp.h"
#include "gimpui.h"
#include "gimpimagemetadata.h"

#include "libgimp-intl.h"


typedef struct
{
  gchar *tag;
  gint  type;
} XmpStructs;


static void        gimp_image_metadata_rotate        (gint32             image_ID,
                                                      GExiv2Orientation  orientation);
static GdkPixbuf * gimp_image_metadata_rotate_pixbuf (GdkPixbuf         *pixbuf,
                                                      GExiv2Orientation  orientation);
static void        gimp_image_metadata_rotate_query  (gint32             image_ID,
                                                      const gchar       *mime_type,
                                                      GimpMetadata      *metadata,
                                                      gboolean           interactive);
static gboolean    gimp_image_metadata_rotate_dialog (gint32             image_ID,
                                                      GExiv2Orientation  orientation,
                                                      const gchar       *parasite_name);


/*  public functions  */

/**
 * gimp_image_metadata_load_prepare:
 * @image_ID:  The image
 * @mime_type: The loaded file's mime-type
 * @file:      The file to load the metadata from
 * @error:     Return location for error
 *
 * Loads and returns metadata from @file to be passed into
 * gimp_image_metadata_load_finish().
 *
 * Returns: The file's metadata.
 *
 * Since: 2.10
 */
GimpMetadata *
gimp_image_metadata_load_prepare (gint32        image_ID,
                                  const gchar  *mime_type,
                                  GFile        *file,
                                  GError      **error)
{
  GimpMetadata *metadata;

  g_return_val_if_fail (image_ID > 0, NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  metadata = gimp_metadata_load_from_file (file, error);

  if (metadata)
    {
      gexiv2_metadata_erase_exif_thumbnail (GEXIV2_METADATA (metadata));
    }

  return metadata;
}

/**
 * gimp_image_metadata_load_finish:
 * @image_ID:    The image
 * @mime_type:   The loaded file's mime-type
 * @metadata:    The metadata to set on the image
 * @flags:       Flags to specify what of the metadata to apply to the image
 * @interactive: Whether this function is allowed to query info with dialogs
 *
 * Applies the @metadata previously loaded with
 * gimp_image_metadata_load_prepare() to the image, taking into account
 * the passed @flags.
 *
 * Since: 2.10
 */
void
gimp_image_metadata_load_finish (gint32                 image_ID,
                                 const gchar           *mime_type,
                                 GimpMetadata          *metadata,
                                 GimpMetadataLoadFlags  flags,
                                 gboolean               interactive)
{
  g_return_if_fail (image_ID > 0);
  g_return_if_fail (mime_type != NULL);
  g_return_if_fail (GEXIV2_IS_METADATA (metadata));

  if (flags & GIMP_METADATA_LOAD_COMMENT)
    {
      gchar *comment;

      comment = gexiv2_metadata_get_tag_interpreted_string (GEXIV2_METADATA (metadata),
                                                            "Exif.Photo.UserComment");
      if (! comment)
        comment = gexiv2_metadata_get_tag_interpreted_string (GEXIV2_METADATA (metadata),
                                                              "Exif.Image.ImageDescription");

      if (comment)
        {
          GimpParasite *parasite;

          parasite = gimp_parasite_new ("gimp-comment",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (comment) + 1,
                                        comment);
          g_free (comment);

          gimp_image_attach_parasite (image_ID, parasite);
          gimp_parasite_free (parasite);
        }
    }

  if (flags & GIMP_METADATA_LOAD_RESOLUTION)
    {
      gdouble   xres;
      gdouble   yres;
      GimpUnit  unit;

      if (gimp_metadata_get_resolution (metadata, &xres, &yres, &unit))
        {
          gimp_image_set_resolution (image_ID, xres, yres);
          gimp_image_set_unit (image_ID, unit);
        }
    }

  if (flags & GIMP_METADATA_LOAD_ORIENTATION)
    {
      gimp_image_metadata_rotate_query (image_ID, mime_type,
                                        metadata, interactive);
    }

  if (flags & GIMP_METADATA_LOAD_COLORSPACE)
    {
      GimpColorProfile *profile = gimp_image_get_color_profile (image_ID);

      /* only look for colorspace information from metadata if the
       * image didn't contain an embedded color profile
       */
      if (! profile)
        {
          GimpMetadataColorspace colorspace;

          colorspace = gimp_metadata_get_colorspace (metadata);

          switch (colorspace)
            {
            case GIMP_METADATA_COLORSPACE_UNSPECIFIED:
            case GIMP_METADATA_COLORSPACE_UNCALIBRATED:
            case GIMP_METADATA_COLORSPACE_SRGB:
              /* use sRGB, a NULL profile will do the right thing  */
              break;

            case GIMP_METADATA_COLORSPACE_ADOBERGB:
              profile = gimp_color_profile_new_rgb_adobe ();
              break;
            }

          if (profile)
            gimp_image_set_color_profile (image_ID, profile);
        }

      if (profile)
        g_object_unref (profile);
    }

  gimp_image_set_metadata (image_ID, metadata);
}

/**
 * gimp_image_metadata_save_prepare:
 * @image_ID:        The image
 * @mime_type:       The saved file's mime-type
 * @suggested_flags: Suggested default values for the @flags passed to
 *                   gimp_image_metadata_save_finish()
 *
 * Gets the image metadata for saving it using
 * gimp_image_metadata_save_finish().
 *
 * The @suggested_flags are determined from what kind of metadata
 * (Exif, XMP, ...) is actually present in the image and the preferences
 * for metadata exporting.
 * The calling application may still update @available_flags, for
 * instance to follow the settings from a previous export in the same
 * session, or a previous export of the same image. But it should not
 * override the preferences without a good reason since it is a data
 * leak.
 *
 * The suggested value for GIMP_METADATA_SAVE_THUMBNAIL is determined by
 * whether there was a thumbnail in the previously imported image.
 *
 * Returns: The image's metadata, prepared for saving.
 *
 * Since: 2.10
 */
GimpMetadata *
gimp_image_metadata_save_prepare (gint32                 image_ID,
                                  const gchar           *mime_type,
                                  GimpMetadataSaveFlags *suggested_flags)
{
  GimpMetadata *metadata;

  g_return_val_if_fail (image_ID > 0, NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (suggested_flags != NULL, NULL);

  *suggested_flags = GIMP_METADATA_SAVE_ALL;

  metadata = gimp_image_get_metadata (image_ID);

  if (metadata)
    {
      GDateTime          *datetime;
      const GimpParasite *comment_parasite;
      const gchar        *comment = NULL;
      gint                image_width;
      gint                image_height;
      gdouble             xres;
      gdouble             yres;
      gchar               buffer[32];
      GExiv2Metadata     *g2metadata = GEXIV2_METADATA (metadata);

      image_width  = gimp_image_width  (image_ID);
      image_height = gimp_image_height (image_ID);

      datetime = g_date_time_new_now_local ();

      comment_parasite = gimp_image_get_parasite (image_ID, "gimp-comment");
      if (comment_parasite)
        comment = gimp_parasite_data (comment_parasite);

      /* Exif */

      if (! gimp_export_exif () ||
          ! gexiv2_metadata_has_exif (g2metadata))
        *suggested_flags &= ~GIMP_METADATA_SAVE_EXIF;

      if (comment)
        {
          gexiv2_metadata_set_tag_string (g2metadata,
                                          "Exif.Photo.UserComment",
                                          comment);
          gexiv2_metadata_set_tag_string (g2metadata,
                                          "Exif.Image.ImageDescription",
                                          comment);
        }

      g_snprintf (buffer, sizeof (buffer),
                  "%d:%02d:%02d %02d:%02d:%02d",
                  g_date_time_get_year (datetime),
                  g_date_time_get_month (datetime),
                  g_date_time_get_day_of_month (datetime),
                  g_date_time_get_hour (datetime),
                  g_date_time_get_minute (datetime),
                  g_date_time_get_second (datetime));
      gexiv2_metadata_set_tag_string (g2metadata,
                                      "Exif.Image.DateTime",
                                      buffer);

      gexiv2_metadata_set_tag_string (g2metadata,
                                      "Exif.Image.Software",
                                      PACKAGE_STRING);

      gimp_metadata_set_pixel_size (metadata,
                                    image_width, image_height);

      gimp_image_get_resolution (image_ID, &xres, &yres);
      gimp_metadata_set_resolution (metadata, xres, yres,
                                    gimp_image_get_unit (image_ID));

      /* XMP */

      if (! gimp_export_xmp () ||
          ! gexiv2_metadata_has_xmp (g2metadata))
        *suggested_flags &= ~GIMP_METADATA_SAVE_XMP;

      gexiv2_metadata_set_tag_string (g2metadata,
                                      "Xmp.dc.Format",
                                      mime_type);

      if (! g_strcmp0 (mime_type, "image/tiff"))
        {
          /* TIFF specific XMP data */

          g_snprintf (buffer, sizeof (buffer), "%d", image_width);
          gexiv2_metadata_set_tag_string (g2metadata,
                                          "Xmp.tiff.ImageWidth",
                                          buffer);

          g_snprintf (buffer, sizeof (buffer), "%d", image_height);
          gexiv2_metadata_set_tag_string (g2metadata,
                                          "Xmp.tiff.ImageLength",
                                          buffer);

          g_snprintf (buffer, sizeof (buffer),
                      "%d:%02d:%02d %02d:%02d:%02d",
                      g_date_time_get_year (datetime),
                      g_date_time_get_month (datetime),
                      g_date_time_get_day_of_month (datetime),
                      g_date_time_get_hour (datetime),
                      g_date_time_get_minute (datetime),
                      g_date_time_get_second (datetime));
          gexiv2_metadata_set_tag_string (g2metadata,
                                          "Xmp.tiff.DateTime",
                                          buffer);
        }

      /* IPTC */

      if (! gimp_export_iptc () ||
          ! gexiv2_metadata_has_iptc (g2metadata))
        *suggested_flags &= ~GIMP_METADATA_SAVE_IPTC;

      g_snprintf (buffer, sizeof (buffer),
                  "%d-%d-%d",
                  g_date_time_get_year (datetime),
                  g_date_time_get_month (datetime),
                  g_date_time_get_day_of_month (datetime));
      gexiv2_metadata_set_tag_string (g2metadata,
                                      "Iptc.Application2.DateCreated",
                                      buffer);

      g_snprintf (buffer, sizeof (buffer),
                  "%02d:%02d:%02d-%02d:%02d",
                  g_date_time_get_hour (datetime),
                  g_date_time_get_minute (datetime),
                  g_date_time_get_second (datetime),
                  g_date_time_get_hour (datetime),
                  g_date_time_get_minute (datetime));
      gexiv2_metadata_set_tag_string (g2metadata,
                                      "Iptc.Application2.TimeCreated",
                                      buffer);

      g_date_time_unref (datetime);

    }

  /* Thumbnail */

  if (FALSE /* FIXME if (original image had a thumbnail) */)
    *suggested_flags &= ~GIMP_METADATA_SAVE_THUMBNAIL;

  /* Color profile */

  if (! gimp_export_color_profile ())
    *suggested_flags &= ~GIMP_METADATA_SAVE_COLOR_PROFILE;

  return metadata;
}


static void
gimp_image_metadata_copy_tag (GExiv2Metadata *src,
                              GExiv2Metadata *dest,
                              const gchar    *tag)
{
  gchar **values = gexiv2_metadata_get_tag_multiple (src, tag);

  if (values)
    {
      gexiv2_metadata_set_tag_multiple (dest, tag, (const gchar **) values);
      g_strfreev (values);
    }
  else
    {
      gchar *value = gexiv2_metadata_get_tag_string (src, tag);

      if (value)
        {
          gexiv2_metadata_set_tag_string (dest, tag, value);
          g_free (value);
        }
    }
}

/**
 * gimp_image_metadata_save_finish:
 * @image_ID:  The image
 * @mime_type: The saved file's mime-type
 * @metadata:  The metadata to set on the image
 * @flags:     Flags to specify what of the metadata to save
 * @file:      The file to load the metadata from
 * @error:     Return location for error message
 *
 * Saves the @metadata retrieved from the image with
 * gimp_image_metadata_save_prepare() to @file, taking into account
 * the passed @flags.
 *
 * Return value: Whether the save was successful.
 *
 * Since: 2.10
 */
gboolean
gimp_image_metadata_save_finish (gint32                  image_ID,
                                 const gchar            *mime_type,
                                 GimpMetadata           *metadata,
                                 GimpMetadataSaveFlags   flags,
                                 GFile                  *file,
                                 GError                **error)
{
  GimpMetadata   *new_metadata;
  GExiv2Metadata *new_g2metadata;
  gboolean        support_exif;
  gboolean        support_xmp;
  gboolean        support_iptc;
  gboolean        success = FALSE;
  gint            i;

  g_return_val_if_fail (image_ID > 0, FALSE);
  g_return_val_if_fail (mime_type != NULL, FALSE);
  g_return_val_if_fail (GEXIV2_IS_METADATA (metadata), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! (flags & (GIMP_METADATA_SAVE_EXIF |
                  GIMP_METADATA_SAVE_XMP  |
                  GIMP_METADATA_SAVE_IPTC |
                  GIMP_METADATA_SAVE_THUMBNAIL)))
    return TRUE;

  /* read metadata from saved file */
  new_metadata = gimp_metadata_load_from_file (file, error);
  new_g2metadata = GEXIV2_METADATA (new_metadata);

  if (! new_metadata)
    return FALSE;

  support_exif = gexiv2_metadata_get_supports_exif (new_g2metadata);
  support_xmp  = gexiv2_metadata_get_supports_xmp  (new_g2metadata);
  support_iptc = gexiv2_metadata_get_supports_iptc (new_g2metadata);

  if ((flags & GIMP_METADATA_SAVE_EXIF) && support_exif)
    {
      gchar **exif_data = gexiv2_metadata_get_exif_tags (GEXIV2_METADATA (metadata));

      for (i = 0; exif_data[i] != NULL; i++)
        {
          if (! gexiv2_metadata_has_tag (new_g2metadata, exif_data[i]) &&
              gimp_metadata_is_tag_supported (exif_data[i], mime_type))
            {
              gimp_image_metadata_copy_tag (GEXIV2_METADATA (metadata),
                                            new_g2metadata,
                                            exif_data[i]);
            }
        }

      g_strfreev (exif_data);
    }

  if ((flags & GIMP_METADATA_SAVE_XMP) && support_xmp)
    {
      static const XmpStructs structlist[] =
      {
        { "Xmp.iptcExt.LocationCreated", GEXIV2_STRUCTURE_XA_BAG },
        { "Xmp.iptcExt.LocationShown",   GEXIV2_STRUCTURE_XA_BAG },
        { "Xmp.iptcExt.ArtworkOrObject", GEXIV2_STRUCTURE_XA_BAG },
        { "Xmp.iptcExt.RegistryId",      GEXIV2_STRUCTURE_XA_BAG },
        { "Xmp.xmpMM.History",           GEXIV2_STRUCTURE_XA_SEQ },
        { "Xmp.plus.ImageSupplier",      GEXIV2_STRUCTURE_XA_SEQ },
        { "Xmp.plus.ImageCreator",       GEXIV2_STRUCTURE_XA_SEQ },
        { "Xmp.plus.CopyrightOwner",     GEXIV2_STRUCTURE_XA_SEQ },
        { "Xmp.plus.Licensor",           GEXIV2_STRUCTURE_XA_SEQ }
      };

      gchar         **xmp_data;
      struct timeval  timer_usec;
      gint64          timestamp_usec;
      gchar           ts[128];

      gettimeofday (&timer_usec, NULL);
      timestamp_usec = ((gint64) timer_usec.tv_sec) * 1000000ll +
                        (gint64) timer_usec.tv_usec;
      g_snprintf (ts, sizeof (ts), "%" G_GINT64_FORMAT, timestamp_usec);

      gimp_metadata_add_xmp_history (metadata, "");

      gexiv2_metadata_set_tag_string (GEXIV2_METADATA (metadata),
                                      "Xmp.GIMP.TimeStamp",
                                      ts);

      gexiv2_metadata_set_tag_string (GEXIV2_METADATA (metadata),
                                      "Xmp.xmp.CreatorTool",
                                      N_("GIMP 2.10"));

      gexiv2_metadata_set_tag_string (GEXIV2_METADATA (metadata),
                                      "Xmp.GIMP.Version",
                                      GIMP_VERSION);

      gexiv2_metadata_set_tag_string (GEXIV2_METADATA (metadata),
                                      "Xmp.GIMP.API",
                                      GIMP_API_VERSION);

      gexiv2_metadata_set_tag_string (GEXIV2_METADATA (metadata),
                                      "Xmp.GIMP.Platform",
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
                                      "Windows"
#elif defined(__linux__)
                                      "Linux"
#elif defined(__APPLE__) && defined(__MACH__)
                                      "Mac OS"
#elif defined(unix) || defined(__unix__) || defined(__unix)
                                      "Unix"
#else
                                      "Unknown"
#endif
                                      );

      xmp_data = gexiv2_metadata_get_xmp_tags (GEXIV2_METADATA (metadata));

      /* Patch necessary structures */
      for (i = 0; i < G_N_ELEMENTS (structlist); i++)
        {
          gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (new_g2metadata),
                                              structlist[i].tag,
                                              structlist[i].type);
        }

      for (i = 0; xmp_data[i] != NULL; i++)
        {
          if (! gexiv2_metadata_has_tag (new_g2metadata, xmp_data[i]) &&
              gimp_metadata_is_tag_supported (xmp_data[i], mime_type))
            {
              gimp_image_metadata_copy_tag (GEXIV2_METADATA (metadata),
                                            new_g2metadata,
                                            xmp_data[i]);
            }
        }

      g_strfreev (xmp_data);
    }

  if ((flags & GIMP_METADATA_SAVE_IPTC) && support_iptc)
    {
      gchar **iptc_data = gexiv2_metadata_get_iptc_tags (GEXIV2_METADATA (metadata));

      for (i = 0; iptc_data[i] != NULL; i++)
        {
          if (! gexiv2_metadata_has_tag (new_g2metadata, iptc_data[i]) &&
              gimp_metadata_is_tag_supported (iptc_data[i], mime_type))
            {
              gimp_image_metadata_copy_tag (GEXIV2_METADATA (metadata),
                                            new_g2metadata,
                                            iptc_data[i]);
            }
        }

      g_strfreev (iptc_data);
    }

  if (flags & GIMP_METADATA_SAVE_THUMBNAIL)
    {
      GdkPixbuf *thumb_pixbuf;
      gchar     *thumb_buffer;
      gint       image_width;
      gint       image_height;
      gsize      count;
      gint       thumbw;
      gint       thumbh;

#define EXIF_THUMBNAIL_SIZE 256

      image_width  = gimp_image_width  (image_ID);
      image_height = gimp_image_height (image_ID);

      if (image_width > image_height)
        {
          thumbw = EXIF_THUMBNAIL_SIZE;
          thumbh = EXIF_THUMBNAIL_SIZE * image_height / image_width;
        }
      else
        {
          thumbh = EXIF_THUMBNAIL_SIZE;
          thumbw = EXIF_THUMBNAIL_SIZE * image_width / image_height;
        }

      thumb_pixbuf = gimp_image_get_thumbnail (image_ID, thumbw, thumbh,
                                               GIMP_PIXBUF_KEEP_ALPHA);

      if (gdk_pixbuf_save_to_buffer (thumb_pixbuf, &thumb_buffer, &count,
                                     "jpeg", NULL,
                                     "quality", "75",
                                     NULL))
        {
          gchar buffer[32];

          gexiv2_metadata_set_exif_thumbnail_from_buffer (new_g2metadata,
                                                          (guchar *) thumb_buffer,
                                                          count);

          g_snprintf (buffer, sizeof (buffer), "%d", thumbw);
          gexiv2_metadata_set_tag_string (new_g2metadata,
                                          "Exif.Thumbnail.ImageWidth",
                                          buffer);

          g_snprintf (buffer, sizeof (buffer), "%d", thumbh);
          gexiv2_metadata_set_tag_string (new_g2metadata,
                                          "Exif.Thumbnail.ImageLength",
                                          buffer);

          gexiv2_metadata_set_tag_string (new_g2metadata,
                                          "Exif.Thumbnail.BitsPerSample",
                                          "8 8 8");
          gexiv2_metadata_set_tag_string (new_g2metadata,
                                          "Exif.Thumbnail.SamplesPerPixel",
                                          "3");
          gexiv2_metadata_set_tag_string (new_g2metadata,
                                          "Exif.Thumbnail.PhotometricInterpretation",
                                          "6");

          g_free (thumb_buffer);
        }

      g_object_unref (thumb_pixbuf);
    }

  if (flags & GIMP_METADATA_SAVE_COLOR_PROFILE)
    {
      /* nothing to do, but if we ever need to modify metadata based
       * on the exported color profile, this is probably the place to
       * add it
       */
    }

  success = gimp_metadata_save_to_file (new_metadata, file, error);

  g_object_unref (new_metadata);

  return success;
}

gint32
gimp_image_metadata_load_thumbnail (GFile   *file,
                                    GError **error)
{
  GimpMetadata *metadata;
  GInputStream *input_stream;
  GdkPixbuf    *pixbuf;
  guint8       *thumbnail_buffer;
  gint          thumbnail_size;
  gint32        image_ID = -1;

  g_return_val_if_fail (G_IS_FILE (file), -1);
  g_return_val_if_fail (error == NULL || *error == NULL, -1);

  metadata = gimp_metadata_load_from_file (file, error);
  if (! metadata)
    return -1;

  if (! gexiv2_metadata_get_exif_thumbnail (GEXIV2_METADATA (metadata),
                                            &thumbnail_buffer,
                                            &thumbnail_size))
    {
      g_object_unref (metadata);
      return -1;
    }

  input_stream = g_memory_input_stream_new_from_data (thumbnail_buffer,
                                                      thumbnail_size,
                                                      (GDestroyNotify) g_free);
  pixbuf = gdk_pixbuf_new_from_stream (input_stream, NULL, error);
  g_object_unref (input_stream);

  if (pixbuf)
    {
      gint32 layer_ID;

      image_ID = gimp_image_new (gdk_pixbuf_get_width  (pixbuf),
                                 gdk_pixbuf_get_height (pixbuf),
                                 GIMP_RGB);
      gimp_image_undo_disable (image_ID);

      layer_ID = gimp_layer_new_from_pixbuf (image_ID, _("Background"),
                                             pixbuf,
                                             100.0,
                                             gimp_image_get_default_new_layer_mode (image_ID),
                                             0.0, 0.0);
      g_object_unref (pixbuf);

      gimp_image_insert_layer (image_ID, layer_ID, -1, 0);

      gimp_image_metadata_rotate (image_ID,
                                  gexiv2_metadata_get_orientation (GEXIV2_METADATA (metadata)));
    }

  g_object_unref (metadata);

  return image_ID;
}


/*  private functions  */

static void
gimp_image_metadata_rotate (gint32             image_ID,
                            GExiv2Orientation  orientation)
{
  switch (orientation)
    {
    case GEXIV2_ORIENTATION_UNSPECIFIED:
    case GEXIV2_ORIENTATION_NORMAL:  /* standard orientation, do nothing */
      break;

    case GEXIV2_ORIENTATION_HFLIP:
      gimp_image_flip (image_ID, GIMP_ORIENTATION_HORIZONTAL);
      break;

    case GEXIV2_ORIENTATION_ROT_180:
      gimp_image_rotate (image_ID, GIMP_ROTATE_180);
      break;

    case GEXIV2_ORIENTATION_VFLIP:
      gimp_image_flip (image_ID, GIMP_ORIENTATION_VERTICAL);
      break;

    case GEXIV2_ORIENTATION_ROT_90_HFLIP:  /* flipped diagonally around '\' */
      gimp_image_rotate (image_ID, GIMP_ROTATE_90);
      gimp_image_flip (image_ID, GIMP_ORIENTATION_HORIZONTAL);
      break;

    case GEXIV2_ORIENTATION_ROT_90:  /* 90 CW */
      gimp_image_rotate (image_ID, GIMP_ROTATE_90);
      break;

    case GEXIV2_ORIENTATION_ROT_90_VFLIP:  /* flipped diagonally around '/' */
      gimp_image_rotate (image_ID, GIMP_ROTATE_90);
      gimp_image_flip (image_ID, GIMP_ORIENTATION_VERTICAL);
      break;

    case GEXIV2_ORIENTATION_ROT_270:  /* 90 CCW */
      gimp_image_rotate (image_ID, GIMP_ROTATE_270);
      break;

    default: /* shouldn't happen */
      break;
    }
}

static GdkPixbuf *
gimp_image_metadata_rotate_pixbuf (GdkPixbuf         *pixbuf,
                                   GExiv2Orientation  orientation)
{
  GdkPixbuf *rotated = NULL;
  GdkPixbuf *temp;

  switch (orientation)
    {
    case GEXIV2_ORIENTATION_UNSPECIFIED:
    case GEXIV2_ORIENTATION_NORMAL:  /* standard orientation, do nothing */
      rotated = g_object_ref (pixbuf);
      break;

    case GEXIV2_ORIENTATION_HFLIP:
      rotated = gdk_pixbuf_flip (pixbuf, TRUE);
      break;

    case GEXIV2_ORIENTATION_ROT_180:
      rotated = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
      break;

    case GEXIV2_ORIENTATION_VFLIP:
      rotated = gdk_pixbuf_flip (pixbuf, FALSE);
      break;

    case GEXIV2_ORIENTATION_ROT_90_HFLIP:  /* flipped diagonally around '\' */
      temp = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_CLOCKWISE);
      rotated = gdk_pixbuf_flip (temp, TRUE);
      g_object_unref (temp);
      break;

    case GEXIV2_ORIENTATION_ROT_90:  /* 90 CW */
      rotated = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_CLOCKWISE);
      break;

    case GEXIV2_ORIENTATION_ROT_90_VFLIP:  /* flipped diagonally around '/' */
      temp = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_CLOCKWISE);
      rotated = gdk_pixbuf_flip (temp, FALSE);
      g_object_unref (temp);
      break;

    case GEXIV2_ORIENTATION_ROT_270:  /* 90 CCW */
      rotated = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
      break;

    default: /* shouldn't happen */
      break;
    }

  return rotated;
}

static void
gimp_image_metadata_rotate_query (gint32        image_ID,
                                  const gchar  *mime_type,
                                  GimpMetadata *metadata,
                                  gboolean      interactive)
{
  GimpParasite      *parasite;
  gchar             *parasite_name;
  GExiv2Orientation  orientation;
  gboolean           query = interactive;

  orientation = gexiv2_metadata_get_orientation (GEXIV2_METADATA (metadata));

  if (orientation <= GEXIV2_ORIENTATION_NORMAL ||
      orientation >  GEXIV2_ORIENTATION_MAX)
    return;

  parasite_name = g_strdup_printf ("gimp-metadata-exif-rotate(%s)", mime_type);

  parasite = gimp_get_parasite (parasite_name);

  if (parasite)
    {
      if (strncmp (gimp_parasite_data (parasite), "yes",
                   gimp_parasite_data_size (parasite)) == 0)
        {
          query = FALSE;
        }
      else if (strncmp (gimp_parasite_data (parasite), "no",
                        gimp_parasite_data_size (parasite)) == 0)
        {
          gimp_parasite_free (parasite);
          g_free (parasite_name);
          return;
        }

      gimp_parasite_free (parasite);
    }

  if (query && ! gimp_image_metadata_rotate_dialog (image_ID,
                                                    orientation,
                                                    parasite_name))
    {
      g_free (parasite_name);
      return;
    }

  g_free (parasite_name);

  gimp_image_metadata_rotate (image_ID, orientation);
  gexiv2_metadata_set_orientation (GEXIV2_METADATA (metadata),
                                   GEXIV2_ORIENTATION_NORMAL);
}

static gboolean
gimp_image_metadata_rotate_dialog (gint32             image_ID,
                                   GExiv2Orientation  orientation,
                                   const gchar       *parasite_name)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *toggle;
  GdkPixbuf *pixbuf;
  gchar     *name;
  gchar     *title;
  gint       response;

  name = gimp_image_get_name (image_ID);
  title = g_strdup_printf (_("Rotate %s?"), name);
  g_free (name);

  dialog = gimp_dialog_new (title, "gimp-metadata-rotate-dialog",
                            NULL, 0, NULL, NULL,

                            _("_Keep Original"), GTK_RESPONSE_CANCEL,
                            _("_Rotate"),        GTK_RESPONSE_OK,

                            NULL);

  g_free (title);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

#define THUMBNAIL_SIZE 128

  pixbuf = gimp_image_get_thumbnail (image_ID,
                                     THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                     GIMP_PIXBUF_SMALL_CHECKS);

  if (pixbuf)
    {
      GdkPixbuf *rotated;
      GtkWidget *hbox;
      GtkWidget *image;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
      gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
      gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
      gtk_widget_show (vbox);

      label = gtk_label_new (_("Original"));
      gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);
      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_STYLE,  PANGO_STYLE_ITALIC,
                                 -1);
      gtk_box_pack_end (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      image = gtk_image_new_from_pixbuf (pixbuf);
      gtk_box_pack_end (GTK_BOX (vbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
      gtk_widget_show (vbox);

      label = gtk_label_new (_("Rotated"));
      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_STYLE,  PANGO_STYLE_ITALIC,
                                 -1);
      gtk_box_pack_end (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      rotated = gimp_image_metadata_rotate_pixbuf (pixbuf, orientation);
      g_object_unref (pixbuf);

      image = gtk_image_new_from_pixbuf (rotated);
      g_object_unref (rotated);

      gtk_box_pack_end (GTK_BOX (vbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);
    }

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   _("This image contains Exif orientation "
                                     "metadata."),
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.5,
                        NULL);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  /* eek */
  gtk_widget_set_size_request (GTK_WIDGET (label),
                               2 * THUMBNAIL_SIZE + 12, -1);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   _("Would you like to rotate the image?"),
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.5,
                        NULL);
  /* eek */
  gtk_widget_set_size_request (GTK_WIDGET (label),
                               2 * THUMBNAIL_SIZE + 12, -1);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  toggle = gtk_check_button_new_with_mnemonic (_("_Don't ask me again"));
  gtk_box_pack_end (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), FALSE);
  gtk_widget_show (toggle);

  response = gimp_dialog_run (GIMP_DIALOG (dialog));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)))
    {
      GimpParasite *parasite;
      const gchar  *str = (response == GTK_RESPONSE_OK) ? "yes" : "no";

      parasite = gimp_parasite_new (parasite_name,
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (str), str);
      gimp_attach_parasite (parasite);
      gimp_parasite_free (parasite);
    }

  gtk_widget_destroy (dialog);

  return (response == GTK_RESPONSE_OK);
}
