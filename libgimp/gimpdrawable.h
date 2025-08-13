/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawable.h
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

#ifndef __GIMP_DRAWABLE_H__
#define __GIMP_DRAWABLE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#include <libgimp/gimpitem.h>


#define GIMP_TYPE_DRAWABLE (gimp_drawable_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpDrawable, gimp_drawable, GIMP, DRAWABLE, GimpItem)


struct _GimpDrawableClass
{
  GimpItemClass parent_class;

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


GimpDrawable       * gimp_drawable_get_by_id              (gint32                 drawable_id);

GeglBuffer         * gimp_drawable_get_buffer             (GimpDrawable           *drawable) G_GNUC_WARN_UNUSED_RESULT;
GeglBuffer         * gimp_drawable_get_shadow_buffer      (GimpDrawable           *drawable) G_GNUC_WARN_UNUSED_RESULT;

const Babl         * gimp_drawable_get_format             (GimpDrawable           *drawable);
const Babl         * gimp_drawable_get_thumbnail_format   (GimpDrawable           *drawable);

GBytes             * gimp_drawable_get_thumbnail_data     (GimpDrawable           *drawable,
                                                           gint                    width,
                                                           gint                    height,
                                                           gint                   *actual_width,
                                                           gint                   *actual_height,
                                                           gint                   *bpp);
GdkPixbuf          * gimp_drawable_get_thumbnail          (GimpDrawable           *drawable,
                                                           gint                    width,
                                                           gint                    height,
                                                           GimpPixbufTransparency  alpha);

GBytes             * gimp_drawable_get_sub_thumbnail_data (GimpDrawable           *drawable,
                                                           gint                    src_x,
                                                           gint                    src_y,
                                                           gint                    src_width,
                                                           gint                    src_height,
                                                           gint                    dest_width,
                                                           gint                    dest_height,
                                                           gint                   *actual_width,
                                                           gint                   *actual_height,
                                                           gint                   *bpp);
GdkPixbuf          * gimp_drawable_get_sub_thumbnail      (GimpDrawable           *drawable,
                                                           gint                    src_x,
                                                           gint                    src_y,
                                                           gint                    src_width,
                                                           gint                    src_height,
                                                           gint                    dest_width,
                                                           gint                    dest_height,
                                                           GimpPixbufTransparency  alpha);

void                 gimp_drawable_append_filter          (GimpDrawable           *drawable,
                                                           GimpDrawableFilter     *filter);
void                 gimp_drawable_merge_filter           (GimpDrawable           *drawable,
                                                           GimpDrawableFilter     *filter);

GimpDrawableFilter * gimp_drawable_append_new_filter      (GimpDrawable           *drawable,
                                                           const gchar            *operation_name,
                                                           const gchar            *name,
                                                           GimpLayerMode           mode,
                                                           gdouble                 opacity,
                                                           ...) G_GNUC_NULL_TERMINATED;
void                 gimp_drawable_merge_new_filter       (GimpDrawable           *drawable,
                                                           const gchar            *operation_name,
                                                           const gchar            *name,
                                                           GimpLayerMode           mode,
                                                           gdouble                 opacity,
                                                           ...) G_GNUC_NULL_TERMINATED;


G_END_DECLS

#endif /* __GIMP_DRAWABLE_H__ */
