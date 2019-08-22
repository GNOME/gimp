/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpimagemetadata.h
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

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_IMAGE_METADATA_H__
#define __GIMP_IMAGE_METADATA_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#ifndef GIMP_DEPRECATED_REPLACE_NEW_API

GimpMetadata * gimp_image_metadata_load_prepare (GimpImage             *image,
                                                 const gchar           *mime_type,
                                                 GFile                 *file,
                                                 GError               **error);
void           gimp_image_metadata_load_finish  (GimpImage             *image,
                                                 const gchar           *mime_type,
                                                 GimpMetadata          *metadata,
                                                 GimpMetadataLoadFlags  flags,
                                                 gboolean               interactive);

GimpMetadata * gimp_image_metadata_save_prepare (GimpImage             *image,
                                                 const gchar           *mime_type,
                                                 GimpMetadataSaveFlags *suggested_flags);
gboolean       gimp_image_metadata_save_finish  (GimpImage             *image,
                                                 const gchar           *mime_type,
                                                 GimpMetadata          *metadata,
                                                 GimpMetadataSaveFlags  flags,
                                                 GFile                 *file,
                                                 GError               **error);


/* this is experimental API, to be finished for 2.10 */

GimpImage    * gimp_image_metadata_load_thumbnail (GFile                 *file,
                                                   GError               **error);

#else /* GIMP_DEPRECATED_REPLACE_NEW_API */

#define gimp_image_metadata_load_prepare   gimp_image_metadata_load_prepare_deprecated
#define gimp_image_metadata_load_finish    gimp_image_metadata_load_finish_deprecated
#define gimp_image_metadata_save_prepare   gimp_image_metadata_save_prepare_deprecated
#define gimp_image_metadata_save_finish    gimp_image_metadata_save_finish_deprecated
#define gimp_image_metadata_load_thumbnail gimp_image_metadata_load_thumbnail_deprecated

#endif /* GIMP_DEPRECATED_REPLACE_NEW_API */


GimpMetadata * gimp_image_metadata_load_prepare_deprecated (gint32                 image_id,
                                                            const gchar           *mime_type,
                                                            GFile                 *file,
                                                            GError               **error);
void           gimp_image_metadata_load_finish_deprecated  (gint32                 image_id,
                                                            const gchar           *mime_type,
                                                            GimpMetadata          *metadata,
                                                            GimpMetadataLoadFlags  flags,
                                                            gboolean               interactive);

GimpMetadata * gimp_image_metadata_save_prepare_deprecated (gint32                 image_id,
                                                            const gchar           *mime_type,
                                                            GimpMetadataSaveFlags *suggested_flags);
gboolean       gimp_image_metadata_save_finish_deprecated  (gint32                 image_id,
                                                            const gchar           *mime_type,
                                                            GimpMetadata          *metadata,
                                                            GimpMetadataSaveFlags  flags,
                                                            GFile                 *file,
                                                            GError               **error);

/* this is experimental API, to be finished for 2.10 */

gint32         gimp_image_metadata_load_thumbnail_deprecated (GFile                 *file,
                                                              GError               **error);


G_END_DECLS

#endif /* __GIMP_IMAGE_METADATA_H__ */
