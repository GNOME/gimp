/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcairo-utils.h
 * Copyright (C) 2007 Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_CAIRO_UTILS_H__
#define __GIMP_CAIRO_UTILS_H__


void              gimp_cairo_set_source_rgb             (cairo_t       *cr,
                                                         const GimpRGB *color);
void              gimp_cairo_set_source_rgba            (cairo_t       *cr,
                                                         const GimpRGB *color);

gboolean          gimp_cairo_set_focus_line_pattern     (cairo_t       *cr,
                                                         GtkWidget     *widget);

cairo_pattern_t * gimp_cairo_checkerboard_create        (cairo_t       *cr,
                                                         gint           size,
                                                         const GimpRGB *light,
                                                         const GimpRGB *dark);

cairo_surface_t * gimp_cairo_surface_create_from_pixbuf (GdkPixbuf     *pixbuf);


/*  some useful macros for writing directly to a Cairo surface  */

/**
 * GIMP_CAIRO_RGB24_SET_PIXEL:
 * @d: pointer to the destination buffer
 * @r: red component
 * @g: green component
 * @b: blue component
 *
 * Sets a single pixel in an Cairo image surface in %CAIRO_FORMAT_RGB24.
 *
 * Since: GIMP 2.6
 **/
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GIMP_CAIRO_RGB24_SET_PIXEL(d, r, g, b) \
  G_STMT_START { d[0] = (b);  d[1] = (g);  d[2] = (r); } G_STMT_END
#else
#define GIMP_CAIRO_RGB24_SET_PIXEL(d, r, g, b) \
  G_STMT_START { d[1] = (r);  d[2] = (g);  d[3] = (b); } G_STMT_END
#endif


/**
 * GIMP_CAIRO_RGB24_GET_PIXEL:
 * @s: pointer to the source buffer
 * @r: red component
 * @g: green component
 * @b: blue component
 *
 * Gets a single pixel from a Cairo image surface in %CAIRO_FORMAT_RGB24.
 *
 * Since: GIMP 2.8
 **/
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GIMP_CAIRO_RGB24_GET_PIXEL(s, r, g, b) \
  G_STMT_START { (b) = s[0]; (g) = s[1]; (r) = s[2]; } G_STMT_END
#else
#define GIMP_CAIRO_RGB24_GET_PIXEL(s, r, g, b) \
  G_STMT_START { (r) = s[1]; (g) = s[2]; (b) = s[3]; } G_STMT_END
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
 *
 * Since: GIMP 2.6
 **/

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GIMP_CAIRO_ARGB32_SET_PIXEL(d, r, g, b, a) \
  G_STMT_START {                                   \
    guint tr = (a) * (r) + 0x80;                   \
    guint tg = (a) * (g) + 0x80;                   \
    guint tb = (a) * (b) + 0x80;                   \
    d[0] = (((tb) >> 8) + (tb)) >> 8;              \
    d[1] = (((tg) >> 8) + (tg)) >> 8;              \
    d[2] = (((tr) >> 8) + (tr)) >> 8;              \
    d[3] = (a);                                    \
  } G_STMT_END
#else
#define GIMP_CAIRO_ARGB32_SET_PIXEL(d, r, g, b, a) \
  G_STMT_START {                                   \
    guint tr = (a) * (r) + 0x80;                   \
    guint tg = (a) * (g) + 0x80;                   \
    guint tb = (a) * (b) + 0x80;                   \
    d[0] = (a);                                    \
    d[1] = (((tr) >> 8) + (tr)) >> 8;              \
    d[2] = (((tg) >> 8) + (tg)) >> 8;              \
    d[3] = (((tb) >> 8) + (tb)) >> 8;              \
  } G_STMT_END
#endif


#endif /* __GIMP_CAIRO_UTILS_H__ */
