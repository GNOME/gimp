/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppixbuf.h
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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

#ifndef __LIBGIMP_GIMP_PIXBUF_H__
#define __LIBGIMP_GIMP_PIXBUF_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpPixbufTransparency:
 * @GIMP_PIXBUF_KEEP_ALPHA:   Create a pixbuf with alpha
 * @GIMP_PIXBUF_SMALL_CHECKS: Show transparency as small checks
 * @GIMP_PIXBUF_LARGE_CHECKS: Show transparency as large checks
 *
 * How to deal with transparency when creating thubnail pixbufs from
 * images and drawables.
 **/
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

G_END_DECLS

#endif /* __LIBGIMP_GIMP_PIXBUF_H__ */
