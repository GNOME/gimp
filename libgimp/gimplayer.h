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


#include <libgimp/gimpdrawable.h>


#define GIMP_TYPE_LAYER (gimp_layer_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpLayer, gimp_layer, GIMP, LAYER, GimpDrawable)

struct _GimpLayerClass
{
  GimpDrawableClass parent_class;

  /* Padding for future expansion */
  void (*_gimp_reserved0) (void);
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

GimpLayer * gimp_layer_get_by_id          (gint32           layer_id);

GimpLayer * gimp_layer_new_from_pixbuf    (GimpImage       *image,
                                           const gchar     *name,
                                           GdkPixbuf       *pixbuf,
                                           gdouble          opacity,
                                           GimpLayerMode    mode,
                                           gdouble          progress_start,
                                           gdouble          progress_end) G_GNUC_WARN_UNUSED_RESULT;
GimpLayer * gimp_layer_new_from_surface   (GimpImage       *image,
                                           const gchar     *name,
                                           cairo_surface_t *surface,
                                           gdouble          progress_start,
                                           gdouble          progress_end) G_GNUC_WARN_UNUSED_RESULT;


G_END_DECLS

#endif /* __GIMP_LAYER_H__ */
