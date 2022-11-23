/* LIBLIGMABASE - The LIGMA Basic Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmametadata.h
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

#ifndef __LIGMA_METADATA_H__
#define __LIGMA_METADATA_H__

G_BEGIN_DECLS

#define LIGMA_TYPE_METADATA            (ligma_metadata_get_type ())
#define LIGMA_METADATA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_METADATA, LigmaMetadata))
#define LIGMA_METADATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_METADATA, LigmaMetadataClass))
#define LIGMA_IS_METADATA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_METADATA))
#define LIGMA_IS_METADATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_METADATA))
#define LIGMA_METADATA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_METADATA, LigmaMetadataClass))


/**
 * LigmaMetadataLoadFlags:
 * @LIGMA_METADATA_LOAD_COMMENT:     Load the comment
 * @LIGMA_METADATA_LOAD_RESOLUTION:  Load the resolution
 * @LIGMA_METADATA_LOAD_ORIENTATION: Load the orientation (rotation)
 * @LIGMA_METADATA_LOAD_COLORSPACE:  Load the colorspace
 * @LIGMA_METADATA_LOAD_ALL:         Load all of the above
 *
 * What metadata to load when importing images.
 **/
typedef enum
{
  LIGMA_METADATA_LOAD_COMMENT     = 1 << 0,
  LIGMA_METADATA_LOAD_RESOLUTION  = 1 << 1,
  LIGMA_METADATA_LOAD_ORIENTATION = 1 << 2,
  LIGMA_METADATA_LOAD_COLORSPACE  = 1 << 3,

  LIGMA_METADATA_LOAD_ALL         = 0xffffffff
} LigmaMetadataLoadFlags;


/**
 * LigmaMetadataSaveFlags:
 * @LIGMA_METADATA_SAVE_EXIF:          Save EXIF
 * @LIGMA_METADATA_SAVE_XMP:           Save XMP
 * @LIGMA_METADATA_SAVE_IPTC:          Save IPTC
 * @LIGMA_METADATA_SAVE_THUMBNAIL:     Save a thumbnail of the image
 * @LIGMA_METADATA_SAVE_COLOR_PROFILE: Save the image's color profile
 *                                    Since: 2.10.10
 * @LIGMA_METADATA_SAVE_COMMENT:       Save the image's comment
 *                                    Since: 3.0
 * @LIGMA_METADATA_SAVE_ALL:           Save all of the above
 *
 * What kinds of metadata to save when exporting images.
 **/
typedef enum
{
  LIGMA_METADATA_SAVE_EXIF          = 1 << 0,
  LIGMA_METADATA_SAVE_XMP           = 1 << 1,
  LIGMA_METADATA_SAVE_IPTC          = 1 << 2,
  LIGMA_METADATA_SAVE_THUMBNAIL     = 1 << 3,
  LIGMA_METADATA_SAVE_COLOR_PROFILE = 1 << 4,
  LIGMA_METADATA_SAVE_COMMENT       = 1 << 5,

  LIGMA_METADATA_SAVE_ALL       = 0xffffffff
} LigmaMetadataSaveFlags;


/**
 * LigmaMetadataColorspace:
 * @LIGMA_METADATA_COLORSPACE_UNSPECIFIED:  Unspecified
 * @LIGMA_METADATA_COLORSPACE_UNCALIBRATED: Uncalibrated
 * @LIGMA_METADATA_COLORSPACE_SRGB:         sRGB
 * @LIGMA_METADATA_COLORSPACE_ADOBERGB:     Adobe RGB
 *
 * Well-defined colorspace information available from metadata
 **/
typedef enum
{
  LIGMA_METADATA_COLORSPACE_UNSPECIFIED,
  LIGMA_METADATA_COLORSPACE_UNCALIBRATED,
  LIGMA_METADATA_COLORSPACE_SRGB,
  LIGMA_METADATA_COLORSPACE_ADOBERGB
} LigmaMetadataColorspace;


GType          ligma_metadata_get_type            (void) G_GNUC_CONST;

LigmaMetadata * ligma_metadata_new                 (void);
LigmaMetadata * ligma_metadata_duplicate           (LigmaMetadata           *metadata);

LigmaMetadata * ligma_metadata_deserialize         (const gchar            *metadata_xml);
gchar        * ligma_metadata_serialize           (LigmaMetadata           *metadata);
gchar        * ligma_metadata_get_guid            (void);

void           ligma_metadata_add_xmp_history     (LigmaMetadata           *metadata,
                                                  gchar                  *state_status);

LigmaMetadata * ligma_metadata_load_from_file      (GFile                  *file,
                                                  GError                **error);
gboolean       ligma_metadata_save_to_file        (LigmaMetadata           *metadata,
                                                  GFile                  *file,
                                                  GError                **error);

gboolean       ligma_metadata_set_from_exif       (LigmaMetadata           *metadata,
                                                  const guchar           *exif_data,
                                                  gint                    exif_data_length,
                                                  GError                **error);
gboolean       ligma_metadata_set_from_iptc       (LigmaMetadata           *metadata,
                                                  const guchar           *iptc_data,
                                                  gint                    iptc_data_length,
                                                  GError                **error);
gboolean       ligma_metadata_set_from_xmp        (LigmaMetadata           *metadata,
                                                  const guchar           *xmp_data,
                                                  gint                    xmp_data_length,
                                                  GError                **error);

void           ligma_metadata_set_pixel_size      (LigmaMetadata           *metadata,
                                                  gint                    width,
                                                  gint                    height);
void           ligma_metadata_set_bits_per_sample (LigmaMetadata           *metadata,
                                                  gint                    bits_per_sample);

gboolean       ligma_metadata_get_resolution      (LigmaMetadata           *metadata,
                                                  gdouble                *xres,
                                                  gdouble                *yres,
                                                  LigmaUnit               *unit);
void           ligma_metadata_set_resolution      (LigmaMetadata           *metadata,
                                                  gdouble                 xres,
                                                  gdouble                 yres,
                                                  LigmaUnit                unit);

LigmaMetadataColorspace
               ligma_metadata_get_colorspace      (LigmaMetadata           *metadata);
void           ligma_metadata_set_colorspace      (LigmaMetadata           *metadata,
                                                  LigmaMetadataColorspace  colorspace);

gboolean       ligma_metadata_is_tag_supported    (const gchar            *tag,
                                                  const gchar            *mime_type);

G_END_DECLS

#endif /* __LIGMA_METADATA_H__ */
