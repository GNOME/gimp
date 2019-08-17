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

#define GIMP_TYPE_DRAWABLE            (gimp_drawable_get_type ())
#define GIMP_DRAWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DRAWABLE, GimpDrawable))
#define GIMP_DRAWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAWABLE, GimpDrawableClass))
#define GIMP_IS_DRAWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DRAWABLE))
#define GIMP_IS_DRAWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAWABLE))
#define GIMP_DRAWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DRAWABLE, GimpDrawableClass))


typedef struct _GimpDrawableClass   GimpDrawableClass;

struct _GimpDrawable
{
  GimpItem      parent_instance;
};

struct _GimpDrawableClass
{
  GimpItemClass parent_class;

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

GType        gimp_drawable_get_type               (void) G_GNUC_CONST;

GeglBuffer * gimp_drawable_get_buffer             (gint32         drawable_ID);
GeglBuffer * gimp_drawable_get_shadow_buffer      (gint32         drawable_ID);

const Babl * gimp_drawable_get_format             (gint32         drawable_ID);
const Babl * gimp_drawable_get_thumbnail_format   (gint32         drawable_ID);

guchar     * gimp_drawable_get_thumbnail_data     (gint32         drawable_ID,
                                                   gint          *width,
                                                   gint          *height,
                                                   gint          *bpp);
GdkPixbuf  * gimp_drawable_get_thumbnail          (gint32         drawable_ID,
                                                   gint           width,
                                                   gint           height,
                                                   GimpPixbufTransparency alpha);

guchar     * gimp_drawable_get_sub_thumbnail_data (gint32         drawable_ID,
                                                   gint           src_x,
                                                   gint           src_y,
                                                   gint           src_width,
                                                   gint           src_height,
                                                   gint          *dest_width,
                                                   gint          *dest_height,
                                                   gint          *bpp);
GdkPixbuf  * gimp_drawable_get_sub_thumbnail      (gint32         drawable_ID,
                                                   gint           src_x,
                                                   gint           src_y,
                                                   gint           src_width,
                                                   gint           src_height,
                                                   gint           dest_width,
                                                   gint           dest_height,
                                                   GimpPixbufTransparency alpha);


G_END_DECLS

#endif /* __GIMP_DRAWABLE_H__ */
