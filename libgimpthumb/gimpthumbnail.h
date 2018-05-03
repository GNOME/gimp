/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * http://triq.net/~pearl/thumbnail-spec/
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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_THUMB_H_INSIDE__) && !defined (GIMP_THUMB_COMPILATION)
#error "Only <libgimpthumb/gimpthumb.h> can be included directly."
#endif

#ifndef __GIMP_THUMBNAIL_H__
#define __GIMP_THUMBNAIL_H__

G_BEGIN_DECLS


#define GIMP_TYPE_THUMBNAIL            (gimp_thumbnail_get_type ())
#define GIMP_THUMBNAIL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_THUMBNAIL, GimpThumbnail))
#define GIMP_THUMBNAIL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_THUMBNAIL, GimpThumbnailClass))
#define GIMP_IS_THUMBNAIL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_THUMBNAIL))
#define GIMP_IS_THUMBNAIL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_THUMBNAIL))
#define GIMP_THUMBNAIL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_THUMBNAIL, GimpThumbnailClass))


typedef struct _GimpThumbnailPrivate GimpThumbnailPrivate;
typedef struct _GimpThumbnailClass   GimpThumbnailClass;

/**
 * GimpThumbnail:
 *
 * All members of #GimpThumbnail are private and should only be accessed
 * using object properties.
 **/
struct _GimpThumbnail
{
  GObject               parent_instance;

  GimpThumbnailPrivate *priv;

  /* FIXME MOVE TO PRIVATE */
  /*< private >*/
  GimpThumbState  image_state;
  gchar          *image_uri;
  gchar          *image_filename;
  gint64          image_filesize;
  gint64          image_mtime;
  gint            image_not_found_errno;
  gint            image_width;
  gint            image_height;
  gchar          *image_type;
  gint            image_num_layers;

  GimpThumbState  thumb_state;
  GimpThumbSize   thumb_size;
  gchar          *thumb_filename;
  gint64          thumb_filesize;
  gint64          thumb_mtime;

  gchar          *image_mimetype;
};

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


GType            gimp_thumbnail_get_type         (void) G_GNUC_CONST;

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
