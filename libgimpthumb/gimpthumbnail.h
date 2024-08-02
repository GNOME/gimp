/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * https://specifications.freedesktop.org/thumbnail-spec/
 *
 * Copyright (C) 2001-2004  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
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

#if !defined (__GIMP_THUMB_H_INSIDE__) && !defined (GIMP_THUMB_COMPILATION)
#error "Only <libgimpthumb/gimpthumb.h> can be included directly."
#endif

#ifndef __GIMP_THUMBNAIL_H__
#define __GIMP_THUMBNAIL_H__

G_BEGIN_DECLS


#define GIMP_TYPE_THUMBNAIL (gimp_thumbnail_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpThumbnail, gimp_thumbnail, GIMP, THUMBNAIL, GObject)

struct _GimpThumbnailClass
{
  GObjectClass    parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GimpThumbnail  * gimp_thumbnail_new              (void);

void             gimp_thumbnail_set_uri          (GimpThumbnail  *thumbnail,
                                                  const gchar    *uri);
gboolean         gimp_thumbnail_set_filename     (GimpThumbnail  *thumbnail,
                                                  const gchar    *filename,
                                                  GError        **error);
gboolean         gimp_thumbnail_set_from_thumb   (GimpThumbnail  *thumbnail,
                                                  const gchar    *filename,
                                                  GError        **error);

GimpThumbState   gimp_thumbnail_peek_image       (GimpThumbnail  *thumbnail);
GimpThumbState   gimp_thumbnail_peek_thumb       (GimpThumbnail  *thumbnail,
                                                  GimpThumbSize   size);

GimpThumbState   gimp_thumbnail_check_thumb      (GimpThumbnail  *thumbnail,
                                                  GimpThumbSize   size);

GdkPixbuf      * gimp_thumbnail_load_thumb       (GimpThumbnail  *thumbnail,
                                                  GimpThumbSize   size,
                                                  GError        **error);

gboolean         gimp_thumbnail_save_thumb       (GimpThumbnail  *thumbnail,
                                                  GdkPixbuf      *pixbuf,
                                                  const gchar    *software,
                                                  GError        **error);
gboolean         gimp_thumbnail_save_thumb_local (GimpThumbnail  *thumbnail,
                                                  GdkPixbuf      *pixbuf,
                                                  const gchar    *software,
                                                  GError        **error);

gboolean         gimp_thumbnail_save_failure     (GimpThumbnail  *thumbnail,
                                                  const gchar    *software,
                                                  GError        **error);
void             gimp_thumbnail_delete_failure   (GimpThumbnail  *thumbnail);
void             gimp_thumbnail_delete_others    (GimpThumbnail  *thumbnail,
                                                  GimpThumbSize   size);

gboolean         gimp_thumbnail_has_failed       (GimpThumbnail  *thumbnail);


G_END_DECLS

#endif /* __GIMP_THUMBNAIL_H__ */
