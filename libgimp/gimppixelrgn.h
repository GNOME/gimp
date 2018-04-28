/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppixelrgn.h
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_PIXEL_RGN_H__
#define __GIMP_PIXEL_RGN_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


struct _GimpPixelRgn
{
  guchar       *data;          /* pointer to region data */
  GimpDrawable *drawable;      /* pointer to drawable */
  gint          bpp;           /* bytes per pixel */
  gint          rowstride;     /* bytes per pixel row */
  gint          x, y;          /* origin */
  gint          w, h;          /* width and height of region */
  guint         dirty : 1;     /* will this region be dirtied? */
  guint         shadow : 1;    /* will this region use the shadow or normal tiles */
  gint          process_count; /* used internally */
};


GIMP_DEPRECATED_FOR(gimp_drawable_get_buffer)
void      gimp_pixel_rgn_init       (GimpPixelRgn  *pr,
                                     GimpDrawable  *drawable,
                                     gint           x,
                                     gint           y,
                                     gint           width,
                                     gint           height,
                                     gint           dirty,
                                     gint           shadow);
GIMP_DEPRECATED_FOR(gegl_buffer_sample)
void      gimp_pixel_rgn_get_pixel  (GimpPixelRgn  *pr,
                                     guchar        *buf,
                                     gint           x,
                                     gint           y);
GIMP_DEPRECATED_FOR(gegl_buffer_get)
void      gimp_pixel_rgn_get_row    (GimpPixelRgn  *pr,
                                     guchar        *buf,
                                     gint           x,
                                     gint           y,
                                     gint           width);
GIMP_DEPRECATED_FOR(gegl_buffer_get)
void      gimp_pixel_rgn_get_col    (GimpPixelRgn  *pr,
                                     guchar        *buf,
                                     gint           x,
                                     gint           y,
                                     gint           height);
GIMP_DEPRECATED_FOR(gegl_buffer_get)
void      gimp_pixel_rgn_get_rect   (GimpPixelRgn  *pr,
                                     guchar        *buf,
                                     gint           x,
                                     gint           y,
                                     gint           width,
                                     gint           height);
GIMP_DEPRECATED_FOR(gegl_buffer_set)
void      gimp_pixel_rgn_set_pixel  (GimpPixelRgn  *pr,
                                     const guchar  *buf,
                                     gint           x,
                                     gint           y);
GIMP_DEPRECATED_FOR(gegl_buffer_set)
void      gimp_pixel_rgn_set_row    (GimpPixelRgn  *pr,
                                     const guchar  *buf,
                                     gint           x,
                                     gint           y,
                                     gint           width);
GIMP_DEPRECATED_FOR(gegl_buffer_set)
void      gimp_pixel_rgn_set_col    (GimpPixelRgn  *pr,
                                     const guchar  *buf,
                                     gint           x,
                                     gint           y,
                                     gint           height);
GIMP_DEPRECATED_FOR(gegl_buffer_set)
void      gimp_pixel_rgn_set_rect   (GimpPixelRgn  *pr,
                                     const guchar  *buf,
                                     gint           x,
                                     gint           y,
                                     gint           width,
                                     gint           height);
GIMP_DEPRECATED_FOR(gegl_buffer_iterator_new)
gpointer  gimp_pixel_rgns_register  (gint           nrgns,
                                     ...);
GIMP_DEPRECATED_FOR(gegl_buffer_iterator_new)
gpointer  gimp_pixel_rgns_register2 (gint           nrgns,
                                     GimpPixelRgn **prs);
GIMP_DEPRECATED_FOR(gegl_buffer_iterator_next)
gpointer  gimp_pixel_rgns_process   (gpointer       pri_ptr);


G_END_DECLS

#endif /* __GIMP_PIXEL_RGN_H__ */
