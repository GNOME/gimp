/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpmetadata.h
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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __LIBGIMP_GIMP_METADATA_H__
#define __LIBGIMP_GIMP_METADATA_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


void           gimp_image_metadata_load         (gint32              image_ID,
                                                 const gchar        *mime_type,
                                                 GFile              *file,
                                                 gboolean            interactive);
GimpMetadata * gimp_image_metadata_save_prepare (gint32              image_ID,
                                                 const gchar        *mime_type);
gboolean       gimp_image_metadata_save_finish  (gint32              image_ID,
                                                 const gchar        *mime_type,
                                                 GimpMetadata       *metadata,
                                                 GFile              *file,
                                                 GimpMetadataSaveFlags flags,
                                                 GError            **error);

G_END_DECLS

#endif /* ___LIBGIMP_GIMP_METADATA_H__ */
