/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * https://specifications.freedesktop.org/thumbnail-spec/
 *
 * Copyright (C) 2001-2003  Sven Neumann <sven@ligma.org>
 *                          Michael Natterer <mitch@ligma.org>
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

#if !defined (__LIGMA_THUMB_H_INSIDE__) && !defined (LIGMA_THUMB_COMPILATION)
#error "Only <libligmathumb/ligmathumb.h> can be included directly."
#endif

#ifndef __LIGMA_THUMB_UTILS_H__
#define __LIGMA_THUMB_UTILS_H__

G_BEGIN_DECLS


gboolean            ligma_thumb_init                   (const gchar    *creator,
                                                       const gchar    *thumb_basedir);

const gchar       * ligma_thumb_get_thumb_base_dir     (void);

gchar             * ligma_thumb_find_thumb             (const gchar    *uri,
                                                       LigmaThumbSize  *size) G_GNUC_MALLOC;

LigmaThumbFileType   ligma_thumb_file_test              (const gchar    *filename,
                                                       gint64         *mtime,
                                                       gint64         *size,
                                                       gint           *err_no);

gchar             * ligma_thumb_name_from_uri          (const gchar    *uri,
                                                       LigmaThumbSize   size) G_GNUC_MALLOC;
const gchar       * ligma_thumb_get_thumb_dir          (LigmaThumbSize   size);
gboolean            ligma_thumb_ensure_thumb_dir       (LigmaThumbSize   size,
                                                       GError        **error);
void                ligma_thumbs_delete_for_uri        (const gchar    *uri);

gchar             * ligma_thumb_name_from_uri_local    (const gchar    *uri,
                                                       LigmaThumbSize   size) G_GNUC_MALLOC;
gchar             * ligma_thumb_get_thumb_dir_local    (const gchar    *dirname,
                                                       LigmaThumbSize   size) G_GNUC_MALLOC;
gboolean            ligma_thumb_ensure_thumb_dir_local (const gchar    *dirname,
                                                       LigmaThumbSize   size,
                                                       GError        **error);
void                ligma_thumbs_delete_for_uri_local  (const gchar    *uri);


/*  for internal use only   */
G_GNUC_INTERNAL void    _ligma_thumbs_delete_others    (const gchar    *uri,
                                                       LigmaThumbSize   size);
G_GNUC_INTERNAL gchar * _ligma_thumb_filename_from_uri (const gchar    *uri);


G_END_DECLS

#endif /* __LIGMA_THUMB_UTILS_H__ */
