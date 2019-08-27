/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpimage.h
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_IMAGE_H__
#define __GIMP_IMAGE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

#define GIMP_TYPE_IMAGE            (gimp_image_get_type ())
#define GIMP_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE, GimpImage))
#define GIMP_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE, GimpImageClass))
#define GIMP_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE))
#define GIMP_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE))
#define GIMP_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE, GimpImageClass))


typedef struct _GimpImageClass   GimpImageClass;
typedef struct _GimpImagePrivate GimpImagePrivate;

struct _GimpImage
{
  GObject           parent_instance;

  GimpImagePrivate *priv;
};

struct _GimpImageClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
  void (*_gimp_reserved5) (void);
  void (*_gimp_reserved6) (void);
  void (*_gimp_reserved7) (void);
  void (*_gimp_reserved8) (void);
  void (*_gimp_reserved9) (void);
};

GType          gimp_image_get_type           (void) G_GNUC_CONST;

gint32         gimp_image_get_id             (GimpImage    *image);
GimpImage    * gimp_image_get_by_id          (gint32        image_id);

GimpImage   ** gimp_get_images               (gint         *num_images);
GList        * gimp_list_images              (void);

#ifndef GIMP_DEPRECATED_REPLACE_NEW_API

GimpLayer   ** gimp_image_get_layers         (GimpImage    *image,
                                              gint          *num_layers);
GimpChannel ** gimp_image_get_channels       (GimpImage    *image,
                                              gint          *num_channels);
GimpVectors ** gimp_image_get_vectors        (GimpImage    *image,
                                              gint          *num_vectors);

GList        * gimp_image_list_layers        (GimpImage    *image);
GList        * gimp_image_list_channels      (GimpImage    *image);
GList        * gimp_image_list_vectors       (GimpImage    *image);

guchar       * gimp_image_get_colormap       (GimpImage    *image,
                                              gint         *num_colors);
gboolean       gimp_image_set_colormap       (GimpImage    *image,
                                              const guchar *colormap,
                                              gint          num_colors);

guchar       * gimp_image_get_thumbnail_data (GimpImage    *image,
                                              gint         *width,
                                              gint         *height,
                                              gint         *bpp);
GdkPixbuf    * gimp_image_get_thumbnail      (GimpImage    *image,
                                              gint          width,
                                              gint          height,
                                              GimpPixbufTransparency  alpha);

GimpMetadata * gimp_image_get_metadata       (GimpImage    *image);
gboolean       gimp_image_set_metadata       (GimpImage    *image,
                                              GimpMetadata *metadata);

#else /* GIMP_DEPRECATED_REPLACE_NEW_API */

#define gimp_image_get_layers         gimp_image_get_layers_deprecated
#define gimp_image_get_colormap       gimp_image_get_colormap_deprecated
#define gimp_image_set_colormap       gimp_image_set_colormap_deprecated
#define gimp_image_get_metadata       gimp_image_get_metadata_deprecated
#define gimp_image_set_metadata       gimp_image_set_metadata_deprecated

#endif /* GIMP_DEPRECATED_REPLACE_NEW_API */


gint         * gimp_image_get_layers_deprecated         (gint32         image_id,
                                                         gint          *num_layers);
guchar       * gimp_image_get_colormap_deprecated       (gint32        image_id,
                                                         gint         *num_colors);
gboolean       gimp_image_set_colormap_deprecated       (gint32        image_id,
                                                         const guchar *colormap,
                                                         gint          num_colors);

GimpMetadata * gimp_image_get_metadata_deprecated       (gint32        image_id);
gboolean       gimp_image_set_metadata_deprecated       (gint32        image_id,
                                                         GimpMetadata *metadata);


G_END_DECLS

#endif /* __GIMP_IMAGE_H__ */
