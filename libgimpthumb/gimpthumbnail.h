/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * http://triq.net/~pearl/thumbnail-spec/
 *
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_THUMBNAIL_H__
#define __GIMP_THUMBNAIL_H__

G_BEGIN_DECLS


#define GIMP_TYPE_THUMBNAIL            (gimp_thumbnail_get_type ())
#define GIMP_THUMBNAIL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_THUMBNAIL, GimpThumbnail))
#define GIMP_THUMBNAIL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_THUMBNAIL, GimpThumbnailClass))
#define GIMP_IS_THUMBNAIL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_THUMBNAIL))
#define GIMP_IS_THUMBNAIL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_THUMBNAIL))
#define GIMP_THUMBNAIL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_THUMBNAIL, GimpThumbnailClass))


typedef struct _GimpThumbnailClass GimpThumbnailClass;

struct _GimpThumbnail
{
  GObject         parent_instance;

  gchar          *uri;
  GimpThumbState  state;

  gchar          *thumb_filename;
  gint64          thumb_filesize;
  gint64          thumb_mtime;

  gchar          *image_filename;
  gint64          image_filesize;
  gint64          image_mtime;

  gint            image_width;
  gint            image_height;
  gchar          *image_type;
  gint            image_num_layers;
};

struct _GimpThumbnailClass
{
  GObjectClass    parent_class;
};


GType            gimp_thumbnail_get_type     (void) G_GNUC_CONST;

GimpThumbnail  * gimp_thumbnail_new          (void);

void             gimp_thumbnail_set_uri      (GimpThumbnail  *thumbnail,
                                              const gchar    *uri);
gboolean         gimp_thumbnail_set_filename (GimpThumbnail  *thumbnail,
                                              const gchar    *filename,
                                              GError        **error);

GimpThumbState   gimp_thumbnail_get_state    (GimpThumbnail  *thumnail);
GdkPixbuf      * gimp_thumbnail_get_pixbuf   (GimpThumbnail  *thumbnail,
                                              GimpThumbSize   size,
                                              GError        **error);

gboolean         gimp_thumbnail_save_pixbuf  (GimpThumbnail  *thumbnail,
                                              GdkPixbuf      *pixbuf,
                                              const gchar    *software,
                                              GError        **error);
gboolean         gimp_thumbnail_save_failure (GimpThumbnail  *thumbnail,
                                              const gchar    *software,
                                              GError        **error);


G_END_DECLS

#endif /* __GIMP_THUMBNAIL_H__ */
