/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpimagemetadata-save.c
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

#include "gimp.h"
#include "gimpimagemetadata.h"

#include "libgimp-intl.h"


static const gchar *  gimp_fix_xmp_tag                      (const gchar          *tag);

static void       gimp_image_metadata_copy_tag              (GExiv2Metadata       *src,
                                                             GExiv2Metadata       *dest,
                                                             const gchar          *tag);

static gint       gimp_natural_sort_compare                 (gconstpointer         left,
                                                             gconstpointer         right);

static GList*     gimp_image_metadata_convert_tags_to_list  (gchar               **xmp_tags);

static gboolean   gimp_image_metadata_get_xmp_struct_type   (const gchar          *tag,
                                                             GExiv2StructureType  *struct_type);

static gboolean   gimp_image_metadata_exclude_metadata      (GList                *exclude_list,
                                                             gchar                *tag);

static GList*     gimp_image_metadata_set_xmp_structs       (GList                *xmp_list,
                                                             GExiv2Metadata       *metadata);

/*  public functions  */

/**
 * gimp_image_metadata_save_prepare:
 * @image:           The original image
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
 * The suggested value for %GIMP_METADATA_SAVE_THUMBNAIL is determined by
 * whether there was a thumbnail in the previously imported image.
 *
 * Returns: (transfer full): The image's metadata, prepared for saving.
 *
 * Since: 2.10
 */
GimpMetadata *
gimp_image_metadata_save_prepare (GimpImage             *image,
                                  const gchar           *mime_type,
                                  GimpMetadataSaveFlags *suggested_flags)
{
  GimpMetadata *metadata;
  GError       *error = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (suggested_flags != NULL, NULL);

  *suggested_flags = GIMP_METADATA_SAVE_ALL;

  metadata = gimp_image_get_metadata (image);

  if (metadata)
    {
      GDateTime      *datetime;
      GimpParasite   *comment_parasite;
      gchar          *comment = NULL;
      gint            image_width;
      gint            image_height;
      gdouble         xres;
      gdouble         yres;
      gchar           buffer[32];
      gchar          *datetime_buf = NULL;
      GExiv2Metadata *g2metadata = GEXIV2_METADATA (metadata);

      image_width  = gimp_image_get_width  (image);
      image_height = gimp_image_get_height (image);

      datetime = g_date_time_new_now_local ();

      comment_parasite = gimp_image_get_parasite (image, "gimp-comment");
      if (comment_parasite)
        {
          guint32  parasite_size;

          comment = (gchar *) gimp_parasite_get_data (comment_parasite, &parasite_size);
          comment = g_strndup (comment, parasite_size);

          gimp_parasite_free (comment_parasite);
        }

      /* Exif */

      if (! gimp_export_exif () ||
          ! gexiv2_metadata_has_exif (g2metadata))
        *suggested_flags &= ~GIMP_METADATA_SAVE_EXIF;

      if (comment)
        {
          gexiv2_metadata_try_set_tag_string (g2metadata,
                                              "Exif.Photo.UserComment",
                                              comment, &error);
          if (error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, "Exif.Photo.UserComment", error->message);
              g_clear_error (&error);
            }

          gexiv2_metadata_try_set_tag_string (g2metadata,
                                              "Exif.Image.ImageDescription",
                                              comment, &error);
          if (error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, "Exif.Image.ImageDescription", error->message);
              g_clear_error (&error);
            }
        }

      g_snprintf (buffer, sizeof (buffer),
                  "%d:%02d:%02d %02d:%02d:%02d",
                  g_date_time_get_year (datetime),
                  g_date_time_get_month (datetime),
                  g_date_time_get_day_of_month (datetime),
                  g_date_time_get_hour (datetime),
                  g_date_time_get_minute (datetime),
                  g_date_time_get_second (datetime));
      gexiv2_metadata_try_set_tag_string (g2metadata,
                                          "Exif.Image.DateTime",
                                          buffer, &error);
      if (error)
        {
          g_warning ("%s: failed to set metadata '%s': %s\n",
                     G_STRFUNC, "Exif.Image.DateTime", error->message);
          g_clear_error (&error);
        }

      gexiv2_metadata_try_set_tag_string (g2metadata,
                                          "Exif.Image.Software",
                                          PACKAGE_STRING, &error);
      if (error)
        {
          g_warning ("%s: failed to set metadata '%s': %s\n",
                     G_STRFUNC, "Exif.Image.Software", error->message);
          g_clear_error (&error);
        }

      gimp_metadata_set_pixel_size (metadata,
                                    image_width, image_height);

      gimp_image_get_resolution (image, &xres, &yres);
      gimp_metadata_set_resolution (metadata, xres, yres,
                                    gimp_image_get_unit (image));

      /* XMP */

      if (! gimp_export_xmp () ||
          ! gexiv2_metadata_has_xmp (g2metadata))
        *suggested_flags &= ~GIMP_METADATA_SAVE_XMP;

      gexiv2_metadata_try_set_tag_string (g2metadata,
                                          "Xmp.dc.Format",
                                          mime_type, &error);
      if (error)
        {
          g_warning ("%s: failed to set metadata '%s': %s\n",
                     G_STRFUNC, "Xmp.dc.Format", error->message);
          g_clear_error (&error);
        }

      /* XMP uses datetime in ISO 8601 format */
      datetime_buf = g_date_time_format (datetime, "%Y:%m:%dT%T\%:z");

      gexiv2_metadata_try_set_tag_string (g2metadata,
                                          "Xmp.xmp.ModifyDate",
                                          datetime_buf, &error);
      if (error)
        {
          g_warning ("%s: failed to set metadata '%s': %s\n",
                      G_STRFUNC, "Xmp.xmp.ModifyDate", error->message);
          g_clear_error (&error);
        }
      gexiv2_metadata_try_set_tag_string (g2metadata,
                                          "Xmp.xmp.MetadataDate",
                                          datetime_buf, &error);
      if (error)
        {
          g_warning ("%s: failed to set metadata '%s': %s\n",
                      G_STRFUNC, "Xmp.xmp.MetadataDate", error->message);
          g_clear_error (&error);
        }

      if (! g_strcmp0 (mime_type, "image/tiff"))
        {
          /* TIFF specific XMP data */

          g_snprintf (buffer, sizeof (buffer), "%d", image_width);
          gexiv2_metadata_try_set_tag_string (g2metadata,
                                              "Xmp.tiff.ImageWidth",
                                              buffer, &error);
          if (error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, "Xmp.tiff.ImageWidth", error->message);
              g_clear_error (&error);
            }

          g_snprintf (buffer, sizeof (buffer), "%d", image_height);
          gexiv2_metadata_try_set_tag_string (g2metadata,
                                              "Xmp.tiff.ImageLength",
                                              buffer, &error);
          if (error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, "Xmp.tiff.ImageLength", error->message);
              g_clear_error (&error);
            }

          gexiv2_metadata_try_set_tag_string (g2metadata,
                                              "Xmp.tiff.DateTime",
                                              datetime_buf, &error);
          if (error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, "Xmp.tiff.DateTime", error->message);
              g_clear_error (&error);
            }
        }

      /* IPTC */

      if (! gimp_export_iptc () ||
          ! gexiv2_metadata_has_iptc (g2metadata))
        *suggested_flags &= ~GIMP_METADATA_SAVE_IPTC;

      g_free (datetime_buf);
      g_date_time_unref (datetime);
      g_clear_pointer (&comment, g_free);

      /* EXIF Thumbnail */

      if (gimp_export_thumbnail () && gexiv2_metadata_has_exif (g2metadata))
        {
          gchar *value;

          /* Check a required tag for a thumbnail to be present. */
          value = gexiv2_metadata_try_get_tag_string (g2metadata,
                                                      "Exif.Thumbnail.ImageLength",
                                                      NULL);
          if (! value)
            {
              *suggested_flags &= ~GIMP_METADATA_SAVE_THUMBNAIL;
            }
          else
            {
              g_free (value);
            }
        }
      else
        {
          *suggested_flags &= ~GIMP_METADATA_SAVE_THUMBNAIL;
        }
    }
  else
    {
      /* At least initialize the returned flags with preferences defaults */

      if (! gimp_export_exif ())
        *suggested_flags &= ~GIMP_METADATA_SAVE_EXIF;

      if (! gimp_export_xmp ())
        *suggested_flags &= ~GIMP_METADATA_SAVE_XMP;

      if (! gimp_export_iptc ())
        *suggested_flags &= ~GIMP_METADATA_SAVE_IPTC;

      if (! gimp_export_thumbnail ())
        *suggested_flags &= ~GIMP_METADATA_SAVE_THUMBNAIL;
    }

  /* Color profile */

  if (! gimp_export_color_profile ())
    *suggested_flags &= ~GIMP_METADATA_SAVE_COLOR_PROFILE;

  /* Comment */

  if (! gimp_export_comment ())
    *suggested_flags &= ~GIMP_METADATA_SAVE_COMMENT;

  return metadata;
}

static const gchar *
gimp_fix_xmp_tag (const gchar *tag)
{
  gchar *substring;

  /* Due to problems using /Iptc4xmpExt namespace (/iptcExt is used
   * instead by Exiv2) we replace all occurrences with /iptcExt which
   * is valid but less common. Not doing so would cause saving xmp
   * metadata to fail. This has to be done after getting the values
   * from the source metadata since that source uses the original
   * tag names and would otherwise return NULL as value.
   * /Iptc4xmpExt length = 12
   * /iptcExt     length =  8
   */

  substring = strstr (tag, "/Iptc4xmpExt");
  while (substring)
    {
      gint len_tag = strlen (tag);
      gint len_end;

      len_end = len_tag - (substring - tag) - 12;
      strncpy (substring, "/iptcExt", 8);
      substring += 8;
      /* Using memmove: we have overlapping source and dest */
      memmove (substring, substring+4, len_end);
      substring[len_end] = '\0';
      g_debug ("Fixed tag value: %s", tag);

      /* Multiple occurrences are possible: e.g.:
       * Xmp.iptcExt.ImageRegion[3]/Iptc4xmpExt:RegionBoundary/Iptc4xmpExt:rbVertices[1]/Iptc4xmpExt:rbX
       */
      substring = strstr (tag, "/Iptc4xmpExt");
    }
  return tag;
}

static void
gimp_image_metadata_copy_tag (GExiv2Metadata *src,
                              GExiv2Metadata *dest,
                              const gchar    *tag)
{
  gchar  **values = gexiv2_metadata_try_get_tag_multiple (src, tag, NULL);
  GError  *error  = NULL;

  if (values)
    {
      gchar *temp_tag;

      /* Xmp always seems to return multiple values */
      if (g_str_has_prefix (tag, "Xmp."))
        temp_tag = (gchar *) gimp_fix_xmp_tag (g_strdup (tag));
      else
        temp_tag = g_strdup (tag);

      g_debug ("Copy multi tag %s, first value: %s", temp_tag, values[0]);
      gexiv2_metadata_try_set_tag_multiple (dest, temp_tag,
                                            (const gchar **) values,
                                            &error);
      if (error)
        {
          g_warning ("%s: failed to set multiple metadata '%s': %s\n",
                     G_STRFUNC, tag, error->message);
          g_clear_error (&error);
        }

      g_free (temp_tag);
      g_strfreev (values);
    }
  else
    {
      gchar *value = gexiv2_metadata_try_get_tag_string (src, tag, &error);

      if (value)
        {
          g_debug ("Copy tag %s, value: %s", tag, value);
          gexiv2_metadata_try_set_tag_string (dest, tag, value, &error);
          if (error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, tag, error->message);
              g_clear_error (&error);
            }
          g_free (value);
        }
      else if (error)
        {
          g_warning ("%s: failed to get metadata '%s': %s\n",
                     G_STRFUNC, tag, error->message);
          g_clear_error (&error);
        }
    }
}

static gint
gimp_natural_sort_compare (gconstpointer left,
                           gconstpointer right)
{
  gint   compare;
  gchar *left_key  = g_utf8_collate_key_for_filename ((gchar *) left, -1);
  gchar *right_key = g_utf8_collate_key_for_filename ((gchar *) right, -1);

  compare = g_strcmp0 (left_key, right_key);
  g_free (left_key);
  g_free (right_key);

  return compare;
}

static GList*
gimp_image_metadata_convert_tags_to_list (gchar **xmp_tags)
{
  GList *list = NULL;
  gint   i;

  for (i = 0; xmp_tags[i] != NULL; i++)
    {
      g_debug ("Tag: %s, tag type: %s", xmp_tags[i], gexiv2_metadata_try_get_tag_type (xmp_tags[i], NULL));
      list = g_list_prepend (list, xmp_tags[i]);
    }
  return list;
}

static gboolean
gimp_image_metadata_get_xmp_struct_type (const gchar         *tag,
                                         GExiv2StructureType *struct_type)
{
  GError *error = NULL;

  g_debug ("Struct type for tag: %s, type: %s", tag, gexiv2_metadata_try_get_tag_type (tag, NULL));

  if (! g_strcmp0 (gexiv2_metadata_try_get_tag_type (tag, &error), "XmpSeq"))
    {
      *struct_type = GEXIV2_STRUCTURE_XA_SEQ;
      return TRUE;
    }

  if (error)
    {
      g_debug ("%s: failed to get type of tag '%s': %s\n",
               G_STRFUNC, tag, error->message);
      g_clear_error (&error);
      *struct_type = GEXIV2_STRUCTURE_XA_NONE;
      return FALSE;
    }

  *struct_type = GEXIV2_STRUCTURE_XA_BAG;
  return TRUE;
}

static gboolean
gimp_image_metadata_exclude_metadata (GList *exclude_list,
                                      gchar *tag)
{
  GList *list;

  for (list = exclude_list; list != NULL; list = list->next)
    {
      if (strstr (tag, (gchar *) list->data))
        {
          return TRUE;
        }
    }
  return FALSE;
}

static GList*
gimp_image_metadata_set_xmp_structs (GList          *xmp_list,
                                     GExiv2Metadata *metadata)
{
  GList *list;
  GList *exclude  = NULL;
  gchar *prev_one = NULL;
  gchar *prev_two = NULL;

  for (list = xmp_list; list != NULL; list = list->next)
    {
      gchar **tag_split;

      /*
       * Most tags with structs have only one struct part, like:
       * Xmp.xmpMM.History[1]...
       * However there are also Xmp tags that have two
       * structs in one tag, e.g.:
       * Xmp.crs.GradientBasedCorrections[1]/crs:CorrectionMasks[1]...
       */
      tag_split = g_strsplit ((gchar *) list->data, "[1]", 3);
      /* Check if there are at least two parts */
      if (tag_split && tag_split[1])
        {
          GError *error = NULL;

          if (strstr (tag_split[0], "["))
            {
              gchar **splits    = NULL;
              gint    last_part = -1;

              /* Handle things like:
               * Xmp.iptcExt.ImageRegion[3]/Iptc4xmpExt:RegionBoundary/Iptc4xmpExt:rbVertices[1]/Iptc4xmpExt:rbX,
               * where rbVertices only appears in ImageRegion[3].
               * We only want the part starting with the last '/'.
               */
              splits = g_strsplit (tag_split[0], "/", 0);
              if (splits)
                {
                  GExiv2StructureType  type;
                  gchar               *tag_inner = NULL;

                  for (int i = 0; splits[i] != NULL; i++)
                    {
                      last_part = i;
                    }
                  tag_inner = g_strconcat ("/", splits[last_part], NULL);

                  if (gimp_image_metadata_get_xmp_struct_type (gimp_fix_xmp_tag (tag_inner), &type))
                    {
                      gexiv2_metadata_try_set_xmp_tag_struct (GEXIV2_METADATA (metadata),
                                                              tag_inner, type, &error);
                      if (error)
                        {
                          g_printerr ("%s: failed to set XMP struct '%s': %s\n",
                                      G_STRFUNC, tag_inner, error->message);
                          g_clear_error (&error);
                        }
                    }
                  else
                    {
                      /* Add to exclude list */
                      if (! g_list_find_custom (exclude, tag_inner, (GCompareFunc) g_strcmp0))
                        {
                          g_debug ("Adding unsupported tag to exclude list: %s", tag_inner);
                          exclude = g_list_prepend (exclude, g_strdup (tag_inner));
                        }
                    }
                  g_free (tag_inner);
                }
              g_strfreev (splits);
            }
          else if (! prev_one || strcmp (tag_split[0], prev_one) != 0)
            {
              GExiv2StructureType  type;

              g_free (prev_one);
              prev_one = g_strdup (tag_split[0]);

              if (gimp_image_metadata_get_xmp_struct_type (gimp_fix_xmp_tag (tag_split[0]), &type))
                {
                  gexiv2_metadata_try_set_xmp_tag_struct (GEXIV2_METADATA (metadata),
                                                          prev_one, type, &error);
                  if (error)
                    {
                      g_printerr ("%s: failed to set XMP struct '%s': %s\n",
                                  G_STRFUNC, prev_one, error->message);
                      g_clear_error (&error);
                    }
                }
              else
                {
                  /* Add to exclude list */
                  if (! g_list_find_custom (exclude, tag_split[0], (GCompareFunc) g_strcmp0))
                    {
                      g_debug ("Adding unsupported tag to exclude list: %s", tag_split[0]);
                      exclude = g_list_prepend (exclude, g_strdup (tag_split[0]));
                    }
                }
            }
          if (tag_split[2] && (!prev_two || strcmp (tag_split[1], prev_two) != 0))
            {
              gchar               *second_struct;
              GExiv2StructureType  type;

              g_free (prev_two);
              prev_two = g_strdup (tag_split[1]);
              second_struct = g_strdup_printf ("%s[1]%s", prev_one, gimp_fix_xmp_tag(prev_two));

              if (gimp_image_metadata_get_xmp_struct_type (gimp_fix_xmp_tag (tag_split[1]), &type))
                {
                  gexiv2_metadata_try_set_xmp_tag_struct (GEXIV2_METADATA (metadata),
                                                          second_struct, type, &error);
                  if (error)
                    {
                      g_printerr ("%s: failed to set XMP struct '%s': %s\n",
                                   G_STRFUNC, second_struct, error->message);
                      g_clear_error (&error);
                    }
                }
              else
                {
                  /* Add to exclude list */
                  if (! g_list_find_custom (exclude, tag_split[1], (GCompareFunc) g_strcmp0))
                    {
                      g_debug ("Adding unsupported tag to exclude list: %s", tag_split[1]);
                      exclude = g_list_prepend (exclude, g_strdup (tag_split[1]));
                    }
                }
              g_free (second_struct);
            }
        }

      g_strfreev (tag_split);
    }
  g_free (prev_one);
  g_free (prev_two);

  return exclude;
}

/**
 * gimp_image_metadata_save_filter:
 * @image:     The actually saved image
 * @mime_type: The saved file's mime-type
 * @metadata:  The metadata to export
 * @flags:     Flags to specify what of the metadata to save
 * @file:      The file @image was saved to or NULL if file was not saved yet
 * @error:     Return location for error message
 *
 * Filters the @metadata retrieved from the image with
 * gimp_image_metadata_save_prepare(),
 * taking into account the passed @flags.
 *
 * Note that the @image passed to this function might be different
 * from the image passed to gimp_image_metadata_save_prepare(), due
 * to whatever file export conversion happened in the meantime
 *
 * This is an alternative to gimp_image_metadata_save_finish when you
 * want to save metadata yourself and you need only filtering processing.
 *
 * Returns: (transfer full): Filtered metadata or NULL in case of failure.
 *
 * Use g_object_unref() when returned metadata are no longer needed
 *
 * Since: 3.0
 */
GimpMetadata *
gimp_image_metadata_save_filter (GimpImage            *image,
                                 const gchar          *mime_type,
                                 GimpMetadata         *metadata,
                                 GimpMetadataSaveFlags flags,
                                 GFile                *file,
                                 GError              **error)
{
  GimpMetadata   *new_metadata;
  GExiv2Metadata *new_g2metadata;
  /* Error for cases where we have full control, such as metadata tags
   * and contents. So we don't propagate these in @error (for things out
   * of our control, such as read or write errors), but we use them as
   * internal warning for bugs.
   */
  GError         *code_error = NULL;
  gboolean        support_exif;
  gboolean        support_xmp;
  gboolean        support_iptc;
  gint            i;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (GEXIV2_IS_METADATA (metadata), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! (flags & (GIMP_METADATA_SAVE_EXIF |
                  GIMP_METADATA_SAVE_XMP  |
                  GIMP_METADATA_SAVE_IPTC |
                  GIMP_METADATA_SAVE_THUMBNAIL)))
    return NULL;

  if (file)
    {
      /* read metadata from saved file */
      new_metadata = gimp_metadata_load_from_file (file, error);
    }
  else
    {
      new_metadata = gimp_metadata_new ();
    }

  if (! new_metadata)
    return NULL;

  new_g2metadata = GEXIV2_METADATA (new_metadata);
  support_exif = gexiv2_metadata_get_supports_exif (new_g2metadata);
  support_xmp  = gexiv2_metadata_get_supports_xmp  (new_g2metadata);
  support_iptc = gexiv2_metadata_get_supports_iptc (new_g2metadata);

  if ((flags & GIMP_METADATA_SAVE_EXIF) && support_exif)
    {
      gchar **exif_data = gexiv2_metadata_get_exif_tags (GEXIV2_METADATA (metadata));

      for (i = 0; exif_data[i] != NULL; i++)
        {
          if (! gexiv2_metadata_try_has_tag (new_g2metadata, exif_data[i], NULL) &&
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
      gchar         **xmp_data;
      struct timeval  timer_usec;
      gint64          timestamp_usec;
      gchar           ts[128];
      GList          *xmp_list = NULL;
      GList          *exclude_list = NULL;
      GList          *list;

      gettimeofday (&timer_usec, NULL);
      timestamp_usec = ((gint64) timer_usec.tv_sec) * 1000000ll +
                        (gint64) timer_usec.tv_usec;
      g_snprintf (ts, sizeof (ts), "%" G_GINT64_FORMAT, timestamp_usec);

      gimp_metadata_add_xmp_history (metadata, "");

      gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                          "Xmp.GIMP.TimeStamp",
                                          ts, &code_error);
      if (code_error)
        {
          g_warning ("%s: failed to set metadata '%s': %s\n",
                     G_STRFUNC, "Xmp.GIMP.TimeStamp", code_error->message);
          g_clear_error (&code_error);
        }

      gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                          "Xmp.xmp.CreatorTool",
                                          N_("GIMP"), &code_error);
      if (code_error)
        {
          g_warning ("%s: failed to set metadata '%s': %s\n",
                     G_STRFUNC, "Xmp.xmp.CreatorTool", code_error->message);
          g_clear_error (&code_error);
        }

      gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                          "Xmp.GIMP.Version",
                                          GIMP_VERSION, &code_error);
      if (code_error)
        {
          g_warning ("%s: failed to set metadata '%s': %s\n",
                     G_STRFUNC, "Xmp.GIMP.Version", code_error->message);
          g_clear_error (&code_error);
        }

      gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                          "Xmp.GIMP.API",
                                          GIMP_API_VERSION, &code_error);
      if (code_error)
        {
          g_warning ("%s: failed to set metadata '%s': %s\n",
                     G_STRFUNC, "Xmp.GIMP.API", code_error->message);
          g_clear_error (&code_error);
        }

      gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                          "Xmp.GIMP.Platform",
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
                                          "Windows",
#elif defined(__linux__)
                                          "Linux",
#elif defined(__APPLE__) && defined(__MACH__)
                                          "Mac OS",
#elif defined(unix) || defined(__unix__) || defined(__unix)
                                          "Unix",
#else
                                          "Unknown",
#endif
                                          &code_error);
      if (code_error)
        {
          g_warning ("%s: failed to set metadata '%s': %s\n",
                     G_STRFUNC, "Xmp.GIMP.Platform", code_error->message);
          g_clear_error (&code_error);
        }

      xmp_data = gexiv2_metadata_get_xmp_tags (GEXIV2_METADATA (metadata));

      xmp_list = gimp_image_metadata_convert_tags_to_list (xmp_data);
      xmp_list = g_list_sort (xmp_list, (GCompareFunc) gimp_natural_sort_compare);
      exclude_list = gimp_image_metadata_set_xmp_structs (xmp_list, new_g2metadata);

      for (list = xmp_list; list != NULL; list = list->next)
        {
          gchar *fixed = NULL;

          /* Certain XMP metadata tags currently can't be saved by us until
           * we get support in gexiv2 for adding new structs.
           * We remove these from the exported metadata because the XMPSDK
           * in exiv2 would otherwise fail to write all XMP metadata. */

          fixed = (gchar *) gimp_fix_xmp_tag (g_strdup ((gchar *) list->data));
          if (gimp_image_metadata_exclude_metadata (exclude_list, (gchar *) fixed))
            {
              g_warning ("Unsupported XMP metadata tag (not saved): %s", fixed);
            }
          else if (! gexiv2_metadata_try_has_tag (new_g2metadata, (gchar *) list->data, NULL) &&
              gimp_metadata_is_tag_supported ((gchar *) list->data, mime_type))
            {
              gimp_image_metadata_copy_tag (GEXIV2_METADATA (metadata),
                                            new_g2metadata,
                                            (gchar *) list->data);
            }
          else
            g_debug ("Ignored tag: %s", (gchar *) list->data);
          g_free (fixed);
        }

      g_list_free_full (exclude_list, g_free);
      g_list_free (xmp_list);
      g_strfreev (xmp_data);
    }

  if ((flags & GIMP_METADATA_SAVE_IPTC) && support_iptc)
    {
      gchar **iptc_data = gexiv2_metadata_get_iptc_tags (GEXIV2_METADATA (metadata));

      for (i = 0; iptc_data[i] != NULL; i++)
        {
          if (! gexiv2_metadata_try_has_tag (new_g2metadata, iptc_data[i], NULL) &&
              gimp_metadata_is_tag_supported (iptc_data[i], mime_type))
            {
              gimp_image_metadata_copy_tag (GEXIV2_METADATA (metadata),
                                            new_g2metadata,
                                            iptc_data[i]);
            }
        }

      g_strfreev (iptc_data);
    }

  if (flags & GIMP_METADATA_SAVE_THUMBNAIL && support_exif)
    {
      GdkPixbuf *thumb_pixbuf;
      gchar     *thumb_buffer;
      gint       image_width;
      gint       image_height;
      gsize      count;
      gint       thumbw;
      gint       thumbh;

#define EXIF_THUMBNAIL_SIZE 256

      image_width  = gimp_image_get_width  (image);
      image_height = gimp_image_get_height (image);

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

      /* TODO: currently our exported thumbnails are always RGB because
       * GdkPixbuf only supports RGB so far. While RGB thumbnail may make sense,
       * even for CMYK images or other color spaces, grayscale images would be
       * better served with a grayscale thumbnail too.
       */
      thumb_pixbuf = gimp_image_get_thumbnail (image, thumbw, thumbh,
                                               GIMP_PIXBUF_KEEP_ALPHA);

      if (gdk_pixbuf_save_to_buffer (thumb_pixbuf, &thumb_buffer, &count,
                                     "jpeg", NULL,
                                     "quality", "75",
                                     NULL))
        {
          gchar buffer[32];

          gexiv2_metadata_try_set_exif_thumbnail_from_buffer (new_g2metadata,
                                                              (guchar *) thumb_buffer,
                                                              count, &code_error);
          if (code_error)
            {
              g_warning ("%s: failed to set Exif thumbnail: %s\n",
                         G_STRFUNC, code_error->message);
              g_clear_error (&code_error);
            }

          g_snprintf (buffer, sizeof (buffer), "%d", thumbw);
          gexiv2_metadata_try_set_tag_string (new_g2metadata,
                                              "Exif.Thumbnail.ImageWidth",
                                              buffer, &code_error);
          if (code_error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, "Exif.Thumbnail.ImageWidth", code_error->message);
              g_clear_error (&code_error);
            }

          g_snprintf (buffer, sizeof (buffer), "%d", thumbh);
          gexiv2_metadata_try_set_tag_string (new_g2metadata,
                                              "Exif.Thumbnail.ImageLength",
                                              buffer, &code_error);
          if (code_error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, "Exif.Thumbnail.ImageLength", code_error->message);
              g_clear_error (&code_error);
            }

          gexiv2_metadata_try_set_tag_string (new_g2metadata,
                                              "Exif.Thumbnail.BitsPerSample",
                                              "8 8 8", &code_error);
          if (code_error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, "Exif.Thumbnail.BitsPerSample", code_error->message);
              g_clear_error (&code_error);
            }

          gexiv2_metadata_try_set_tag_string (new_g2metadata,
                                              "Exif.Thumbnail.SamplesPerPixel",
                                              "3", &code_error);
          if (code_error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, "Exif.Thumbnail.SamplesPerPixel", code_error->message);
              g_clear_error (&code_error);
            }

          gexiv2_metadata_try_set_tag_string (new_g2metadata,
                                              "Exif.Thumbnail.PhotometricInterpretation",
                                              "6", &code_error); /* old jpeg */
          if (code_error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, "Exif.Thumbnail.PhotometricInterpretation",
                         code_error->message);
              g_clear_error (&code_error);
            }

          gexiv2_metadata_try_set_tag_string (new_g2metadata,
                                              "Exif.Thumbnail.NewSubfileType",
                                              "1", &code_error); /* reduced resolution image */
          if (code_error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, "Exif.Thumbnail.NewSubfileType", code_error->message);
              g_clear_error (&code_error);
            }

          g_free (thumb_buffer);
        }

      g_object_unref (thumb_pixbuf);
    }
  else
    {
      /* Remove Thumbnail */
      gexiv2_metadata_try_erase_exif_thumbnail (new_g2metadata, &code_error);
      if (code_error)
        {
          g_warning ("%s: failed to erase EXIF thumbnail: %s\n",
                     G_STRFUNC, code_error->message);
          g_clear_error (&code_error);
        }
    }

  if (flags & GIMP_METADATA_SAVE_COLOR_PROFILE)
    {
      /* nothing to do, but if we ever need to modify metadata based
       * on the exported color profile, this is probably the place to
       * add it
       */
    }

  if (flags & GIMP_METADATA_SAVE_COMMENT)
    {
      /* nothing to do, blah blah */
    }

  return new_metadata;
}

/**
 * gimp_image_metadata_save_finish:
 * @image:     The actually saved image
 * @mime_type: The saved file's mime-type
 * @metadata:  The metadata to write to @file
 * @flags:     Flags to specify what of the metadata to save
 * @file:      The file @image was saved to
 * @error:     Return location for error message
 *
 * Saves the @metadata retrieved from the image with
 * gimp_image_metadata_save_prepare() to @file, taking into account
 * the passed @flags.
 *
 * Note that the @image passed to this function might be different
 * from the image passed to gimp_image_metadata_save_prepare(), due
 * to whatever file export conversion happened in the meantime
 *
 * Returns: Whether the save was successful.
 *
 * Since: 2.10
 */
gboolean
gimp_image_metadata_save_finish (GimpImage            *image,
                                 const gchar          *mime_type,
                                 GimpMetadata         *metadata,
                                 GimpMetadataSaveFlags flags,
                                 GFile                *file,
                                 GError              **error)
{
  GimpMetadata *new_metadata;
  gboolean      success = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (mime_type != NULL, FALSE);
  g_return_val_if_fail (GEXIV2_IS_METADATA (metadata), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! (flags & (GIMP_METADATA_SAVE_EXIF |
                  GIMP_METADATA_SAVE_XMP  |
                  GIMP_METADATA_SAVE_IPTC |
                  GIMP_METADATA_SAVE_THUMBNAIL)))
    return TRUE;

  new_metadata = gimp_image_metadata_save_filter (image, mime_type, metadata,
                                                  flags, file, error);
  if (! new_metadata)
    {
      return FALSE;
    }

  success = gimp_metadata_save_to_file (new_metadata, file, error);

  g_object_unref (new_metadata);

  return success;
}
