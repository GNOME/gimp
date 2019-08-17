/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimplayer.h
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

#ifndef __GIMP_LAYER_H__
#define __GIMP_LAYER_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

#define GIMP_TYPE_LAYER            (gimp_layer_get_type ())
#define GIMP_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LAYER, GimpLayer))
#define GIMP_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER, GimpLayerClass))
#define GIMP_IS_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LAYER))
#define GIMP_IS_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER))
#define GIMP_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LAYER, GimpLayerClass))


typedef struct _GimpLayerClass   GimpLayerClass;
typedef struct _GimpLayerPrivate GimpLayerPrivate;

struct _GimpLayer
{
  GimpDrawable      parent_instance;
};

struct _GimpLayerClass
{
  GimpDrawableClass parent_class;

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

GType    gimp_layer_get_type           (void) G_GNUC_CONST;

gint32   gimp_layer_copy               (gint32           layer_ID);


#ifndef GIMP_DEPRECATED_REPLACE_NEW_API

gint32   gimp_layer_new                (GimpImage       *image,
                                        const gchar     *name,
                                        gint             width,
                                        gint             height,
                                        GimpImageType    type,
                                        gdouble          opacity,
                                        GimpLayerMode    mode);

gint32   gimp_layer_new_from_pixbuf    (GimpImage       *image,
                                        const gchar     *name,
                                        GdkPixbuf       *pixbuf,
                                        gdouble          opacity,
                                        GimpLayerMode    mode,
                                        gdouble          progress_start,
                                        gdouble          progress_end);
gint32   gimp_layer_new_from_surface   (GimpImage       *image,
                                        const gchar     *name,
                                        cairo_surface_t *surface,
                                        gdouble          progress_start,
                                        gdouble          progress_end);

#else /* GIMP_DEPRECATED_REPLACE_NEW_API */

#define gimp_layer_new              gimp_layer_new_deprecated
#define gimp_layer_new_from_pixbuf  gimp_layer_new_from_pixbuf_deprecated
#define gimp_layer_new_from_surface gimp_layer_new_from_surface_deprecated

#endif /* GIMP_DEPRECATED_REPLACE_NEW_API */


gint32   gimp_layer_new_deprecated              (gint32           image_id,
                                                 const gchar     *name,
                                                 gint             width,
                                                 gint             height,
                                                 GimpImageType    type,
                                                 gdouble          opacity,
                                                 GimpLayerMode    mode);

gint32   gimp_layer_new_from_pixbuf_deprecated  (gint32           image_id,
                                                 const gchar     *name,
                                                 GdkPixbuf       *pixbuf,
                                                 gdouble          opacity,
                                                 GimpLayerMode    mode,
                                                 gdouble          progress_start,
                                                 gdouble          progress_end);
gint32   gimp_layer_new_from_surface_deprecated (gint32           image_id,
                                                 const gchar     *name,
                                                 cairo_surface_t *surface,
                                                 gdouble          progress_start,
                                                 gdouble          progress_end);


G_END_DECLS

#endif /* __GIMP_LAYER_H__ */
