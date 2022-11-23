/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * https://specifications.freedesktop.org/thumbnail-spec/
 *
 * Copyright (C) 2001-2004  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_THUMBNAIL_H__
#define __LIGMA_THUMBNAIL_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_THUMBNAIL            (ligma_thumbnail_get_type ())
#define LIGMA_THUMBNAIL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_THUMBNAIL, LigmaThumbnail))
#define LIGMA_THUMBNAIL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_THUMBNAIL, LigmaThumbnailClass))
#define LIGMA_IS_THUMBNAIL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_THUMBNAIL))
#define LIGMA_IS_THUMBNAIL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_THUMBNAIL))
#define LIGMA_THUMBNAIL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_THUMBNAIL, LigmaThumbnailClass))


typedef struct _LigmaThumbnailPrivate LigmaThumbnailPrivate;
typedef struct _LigmaThumbnailClass   LigmaThumbnailClass;

/**
 * LigmaThumbnail:
 *
 * All members of #LigmaThumbnail are private and should only be accessed
 * using object properties.
 **/
struct _LigmaThumbnail
{
  GObject               parent_instance;

  LigmaThumbnailPrivate *priv;

  /* FIXME MOVE TO PRIVATE */
  /*< private >*/
  LigmaThumbState  image_state;
  gchar          *image_uri;
  gchar          *image_filename;
  gint64          image_filesize;
  gint64          image_mtime;
  gint            image_not_found_errno;
  gint            image_width;
  gint            image_height;
  gchar          *image_type;
  gint            image_num_layers;

  LigmaThumbState  thumb_state;
  LigmaThumbSize   thumb_size;
  gchar          *thumb_filename;
  gint64          thumb_filesize;
  gint64          thumb_mtime;

  gchar          *image_mimetype;
};

struct _LigmaThumbnailClass
{
  GObjectClass    parent_class;

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType            ligma_thumbnail_get_type         (void) G_GNUC_CONST;

LigmaThumbnail  * ligma_thumbnail_new              (void);

void             ligma_thumbnail_set_uri          (LigmaThumbnail  *thumbnail,
                                                  const gchar    *uri);
gboolean         ligma_thumbnail_set_filename     (LigmaThumbnail  *thumbnail,
                                                  const gchar    *filename,
                                                  GError        **error);
gboolean         ligma_thumbnail_set_from_thumb   (LigmaThumbnail  *thumbnail,
                                                  const gchar    *filename,
                                                  GError        **error);

LigmaThumbState   ligma_thumbnail_peek_image       (LigmaThumbnail  *thumbnail);
LigmaThumbState   ligma_thumbnail_peek_thumb       (LigmaThumbnail  *thumbnail,
                                                  LigmaThumbSize   size);

LigmaThumbState   ligma_thumbnail_check_thumb      (LigmaThumbnail  *thumbnail,
                                                  LigmaThumbSize   size);

GdkPixbuf      * ligma_thumbnail_load_thumb       (LigmaThumbnail  *thumbnail,
                                                  LigmaThumbSize   size,
                                                  GError        **error);

gboolean         ligma_thumbnail_save_thumb       (LigmaThumbnail  *thumbnail,
                                                  GdkPixbuf      *pixbuf,
                                                  const gchar    *software,
                                                  GError        **error);
gboolean         ligma_thumbnail_save_thumb_local (LigmaThumbnail  *thumbnail,
                                                  GdkPixbuf      *pixbuf,
                                                  const gchar    *software,
                                                  GError        **error);

gboolean         ligma_thumbnail_save_failure     (LigmaThumbnail  *thumbnail,
                                                  const gchar    *software,
                                                  GError        **error);
void             ligma_thumbnail_delete_failure   (LigmaThumbnail  *thumbnail);
void             ligma_thumbnail_delete_others    (LigmaThumbnail  *thumbnail,
                                                  LigmaThumbSize   size);

gboolean         ligma_thumbnail_has_failed       (LigmaThumbnail  *thumbnail);


G_END_DECLS

#endif /* __LIGMA_THUMBNAIL_H__ */
