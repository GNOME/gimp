/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmaimagemetadata.h
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

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif


#ifndef __LIGMA_IMAGE_METADATA_H__
#define __LIGMA_IMAGE_METADATA_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


LigmaMetadata * ligma_image_metadata_load_prepare   (LigmaImage             *image,
                                                   const gchar           *mime_type,
                                                   GFile                 *file,
                                                   GError               **error);
void           ligma_image_metadata_load_finish    (LigmaImage             *image,
                                                   const gchar           *mime_type,
                                                   LigmaMetadata          *metadata,
                                                   LigmaMetadataLoadFlags  flags);

LigmaMetadata * ligma_image_metadata_save_prepare   (LigmaImage             *image,
                                                   const gchar           *mime_type,
                                                   LigmaMetadataSaveFlags *suggested_flags);
LigmaMetadata * ligma_image_metadata_save_filter    (LigmaImage             *image,
                                                   const gchar           *mime_type,
                                                   LigmaMetadata          *metadata,
                                                   LigmaMetadataSaveFlags  flags,
                                                   GFile                 *file,
                                                   GError               **error);
gboolean       ligma_image_metadata_save_finish    (LigmaImage             *image,
                                                   const gchar           *mime_type,
                                                   LigmaMetadata          *metadata,
                                                   LigmaMetadataSaveFlags  flags,
                                                   GFile                 *file,
                                                   GError               **error);

LigmaImage    * ligma_image_metadata_load_thumbnail (GFile                 *file,
                                                   GError               **error);


G_END_DECLS

#endif /* __LIGMA_IMAGE_METADATA_H__ */
