/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcairo-utils.h
 * Copyright (C) 2007 Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_CAIRO_UTILS_H__
#define __GIMP_CAIRO_UTILS_H__


void              gimp_cairo_set_source_color           (cairo_t   *cr,
                                                         GimpRGB   *color);
cairo_surface_t * gimp_cairo_create_surface_from_pixbuf (GdkPixbuf *pixbuf);


#define INT_MULT(a,b,t) ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))


/*  some useful macros for writing directly to a Cairo surface  */

/**
 * GIMP_CAIRO_RGB24_SET_PIXEL:
 * @d: pointer to the destination buffer
 * @r: red component
 * @g: green component
 * @b: blue component
 *
 * Sets a single pixel in an Cairo image surface in %CAIRO_FORMAT_RGB24.
 **/
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GIMP_CAIRO_RGB24_SET_PIXEL(d, r, g, b) \
  G_STMT_START { d[0] = (b);  d[1] = (g);  d[2] = (r); } G_STMT_END
#else
#define GIMP_CAIRO_RGB24_SET_PIXEL(d, r, g, b) \
  G_STMT_START { d[1] = (r);  d[2] = (g);  d[3] = (b); } G_STMT_END
#endif


/**
 * GIMP_CAIRO_ARGB32_SET_PIXEL:
 * @d: pointer to the destination buffer
 * @r: red component, not pre-multiplied
 * @g: green component, not pre-multiplied
 * @b: blue component, not pre-multiplied
 * @a: alpha component
 *
 * Sets a single pixel in an Cairo image surface in %CAIRO_FORMAT_ARGB32.
 **/
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GIMP_CAIRO_ARGB32_SET_PIXEL(d, r, g, b, a) \
  G_STMT_START {                                   \
    guint t1, t2, t3;                              \
    d[0] = INT_MULT ((b), (a), t1);                \
    d[1] = INT_MULT ((g), (a), t2);                \
    d[2] = INT_MULT ((r), (a), t3);                \
    d[3] = (a);                                    \
  } G_STMT_END
#else
#define GIMP_CAIRO_ARGB32_SET_PIXEL(d, r, g, b, a) \
  G_STMT_START {                                   \
    guint t1, t2, t3;                              \
    d[0] = (a);                                    \
    d[1] = INT_MULT ((r), (a), t1);                \
    d[2] = INT_MULT ((g), (a), t2);                \
    d[3] = INT_MULT ((b), (a), t3);                \
  } G_STMT_END
#endif


#endif /* __GIMP_CAIRO_UTILS_H__ */
