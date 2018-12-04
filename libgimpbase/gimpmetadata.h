/* LIBGIMPBASE - The GIMP Basic Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmetadata.h
 * Copyright (C) 2013 Hartmut Kuhse <hartmutkuhse@src.gnome.org>
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

#ifndef __GIMP_METADATA_H__
#define __GIMP_METADATA_H__

G_BEGIN_DECLS

#define GIMP_TYPE_METADATA            (gimp_metadata_get_type ())
#define GIMP_METADATA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_METADATA, GimpMetadata))
#define GIMP_METADATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_METADATA, GimpMetadataClass))
#define GIMP_IS_METADATA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_METADATA))
#define GIMP_IS_METADATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_METADATA))
#define GIMP_METADATA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_METADATA, GimpMetadataClass))


/**
 * GimpMetadataLoadFlags:
 * @GIMP_METADATA_LOAD_COMMENT:     Load the comment
 * @GIMP_METADATA_LOAD_RESOLUTION:  Load the resolution
 * @GIMP_METADATA_LOAD_ORIENTATION: Load the orientation (rotation)
 * @GIMP_METADATA_LOAD_COLORSPACE:  Load the colorspace
 * @GIMP_METADATA_LOAD_ALL:         Load all of the above
 *
 * What metadata to load when importing images.
 **/
typedef enum
{
  GIMP_METADATA_LOAD_COMMENT     = 1 << 0,
  GIMP_METADATA_LOAD_RESOLUTION  = 1 << 1,
  GIMP_METADATA_LOAD_ORIENTATION = 1 << 2,
  GIMP_METADATA_LOAD_COLORSPACE  = 1 << 3,

  GIMP_METADATA_LOAD_ALL         = 0xffffffff
} GimpMetadataLoadFlags;


/**
 * GimpMetadataSaveFlags:
 * @GIMP_METADATA_SAVE_EXIF:          Save EXIF
 * @GIMP_METADATA_SAVE_XMP:           Save XMP
 * @GIMP_METADATA_SAVE_IPTC:          Save IPTC
 * @GIMP_METADATA_SAVE_THUMBNAIL:     Save a thumbnail of the image
 * @GIMP_METADATA_SAVE_COLOR_PROFILE: Save the image's color profile
 *                                    Since: 2.10.10
 * @GIMP_METADATA_SAVE_ALL:           Save all of the above
 *
 * What kinds of metadata to save when exporting images.
 **/
typedef enum
{
  GIMP_METADATA_SAVE_EXIF          = 1 << 0,
  GIMP_METADATA_SAVE_XMP           = 1 << 1,
  GIMP_METADATA_SAVE_IPTC          = 1 << 2,
  GIMP_METADATA_SAVE_THUMBNAIL     = 1 << 3,
  GIMP_METADATA_SAVE_COLOR_PROFILE = 1 << 4,

  GIMP_METADATA_SAVE_ALL       = 0xffffffff
} GimpMetadataSaveFlags;


/**
 * GimpMetadataColorspace:
 * @GIMP_METADATA_COLORSPACE_UNSPECIFIED:  Unspecified
 * @GIMP_METADATA_COLORSPACE_UNCALIBRATED: Uncalibrated
 * @GIMP_METADATA_COLORSPACE_SRGB:         sRGB
 * @GIMP_METADATA_COLORSPACE_ADOBERGB:     Adobe RGB
 *
 * Well-defined colorspace information available from metadata
 **/
typedef enum
{
  GIMP_METADATA_COLORSPACE_UNSPECIFIED,
  GIMP_METADATA_COLORSPACE_UNCALIBRATED,
  GIMP_METADATA_COLORSPACE_SRGB,
  GIMP_METADATA_COLORSPACE_ADOBERGB
} GimpMetadataColorspace;


GType          gimp_metadata_get_type            (void) G_GNUC_CONST;

GimpMetadata * gimp_metadata_new                 (void);
GimpMetadata * gimp_metadata_duplicate           (GimpMetadata           *metadata);

GimpMetadata * gimp_metadata_deserialize         (const gchar            *metadata_xml);
gchar        * gimp_metadata_serialize           (GimpMetadata           *metadata);
gchar        * gimp_metadata_get_guid            (void);

void           gimp_metadata_add_xmp_history     (GimpMetadata           *metadata,
                                                  gchar                  *state_status);

GimpMetadata * gimp_metadata_load_from_file      (GFile                  *file,
                                                  GError                **error);
gboolean       gimp_metadata_save_to_file        (GimpMetadata           *metadata,
                                                  GFile                  *file,
                                                  GError                **error);

gboolean       gimp_metadata_set_from_exif       (GimpMetadata           *metadata,
                                                  const guchar           *exif_data,
                                                  gint                    exif_data_length,
                                                  GError                **error);
gboolean       gimp_metadata_set_from_iptc       (GimpMetadata           *metadata,
                                                  const guchar           *iptc_data,
                                                  gint                    iptc_data_length,
                                                  GError                **error);
gboolean       gimp_metadata_set_from_xmp        (GimpMetadata           *metadata,
                                                  const guchar           *xmp_data,
                                                  gint                    xmp_data_length,
                                                  GError                **error);

void           gimp_metadata_set_pixel_size      (GimpMetadata           *metadata,
                                                  gint                    width,
                                                  gint                    height);
void           gimp_metadata_set_bits_per_sample (GimpMetadata           *metadata,
                                                  gint                    bits_per_sample);

gboolean       gimp_metadata_get_resolution      (GimpMetadata           *metadata,
                                                  gdouble                *xres,
                                                  gdouble                *yres,
                                                  GimpUnit               *unit);
void           gimp_metadata_set_resolution      (GimpMetadata           *metadata,
                                                  gdouble                 xres,
                                                  gdouble                 yres,
                                                  GimpUnit                unit);

GimpMetadataColorspace
               gimp_metadata_get_colorspace      (GimpMetadata           *metadata);
void           gimp_metadata_set_colorspace      (GimpMetadata           *metadata,
                                                  GimpMetadataColorspace  colorspace);

gboolean       gimp_metadata_is_tag_supported    (const gchar            *tag,
                                                  const gchar            *mime_type);

G_END_DECLS

#endif /* __GIMP_METADATA_H__ */
