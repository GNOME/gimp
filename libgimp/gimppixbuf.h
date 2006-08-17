/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppixbuf.h
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_PIXBUF_H__
#define __GIMP_PIXBUF_H__


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


typedef enum
{
  GIMP_PIXBUF_KEEP_ALPHA,
  GIMP_PIXBUF_SMALL_CHECKS,
  GIMP_PIXBUF_LARGE_CHECKS
} GimpPixbufTransparency;


GdkPixbuf * gimp_image_get_thumbnail        (gint32                  image_ID,
                                             gint                    width,
                                             gint                    height,
                                             GimpPixbufTransparency  alpha);
GdkPixbuf * gimp_drawable_get_thumbnail     (gint32                  drawable_ID,
                                             gint                    width,
                                             gint                    height,
                                             GimpPixbufTransparency  alpha);
GdkPixbuf * gimp_drawable_get_sub_thumbnail (gint32                  drawable_ID,
                                             gint                    src_x,
                                             gint                    src_y,
                                             gint                    src_width,
                                             gint                    src_height,
                                             gint                    dest_width,
                                             gint                    dest_height,
                                             GimpPixbufTransparency  alpha);

gint32      gimp_layer_new_from_pixbuf      (gint32                  image_ID,
                                             const gchar            *name,
                                             GdkPixbuf              *pixbuf,
                                             gdouble                 opacity,
                                             GimpLayerModeEffects    mode,
                                             gdouble                 progress_start,
                                             gdouble                 progress_end);


G_END_DECLS

#endif /* __GIMP_PIXBUF_H__ */
