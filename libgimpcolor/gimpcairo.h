/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcairo.h
 * Copyright (C) 2007      Sven Neumann <sven@gimp.org>
 *               2010-2012 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_CAIRO_H__
#define __GIMP_CAIRO_H__


cairo_pattern_t * gimp_cairo_checkerboard_create   (cairo_t         *cr,
                                                    gint             size,
                                                    const GeglColor *light,
                                                    const GeglColor *dark);

const Babl      * gimp_cairo_surface_get_format    (cairo_surface_t *surface);
GeglBuffer      * gimp_cairo_surface_create_buffer (cairo_surface_t *surface,
                                                    const Babl      *format);


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
 * Since: 2.6
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
 * Since: 2.8
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
 * Since: 2.6
 **/
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GIMP_CAIRO_ARGB32_SET_PIXEL(d, r, g, b, a) \
  G_STMT_START {                                   \
    const guint tr = (a) * (r) + 0x80;             \
    const guint tg = (a) * (g) + 0x80;             \
    const guint tb = (a) * (b) + 0x80;             \
    (d)[0] = (((tb) >> 8) + (tb)) >> 8;            \
    (d)[1] = (((tg) >> 8) + (tg)) >> 8;            \
    (d)[2] = (((tr) >> 8) + (tr)) >> 8;            \
    (d)[3] = (a);                                  \
  } G_STMT_END
#else
#define GIMP_CAIRO_ARGB32_SET_PIXEL(d, r, g, b, a) \
  G_STMT_START {                                   \
    const guint tr = (a) * (r) + 0x80;             \
    const guint tg = (a) * (g) + 0x80;             \
    const guint tb = (a) * (b) + 0x80;             \
    (d)[0] = (a);                                  \
    (d)[1] = (((tr) >> 8) + (tr)) >> 8;            \
    (d)[2] = (((tg) >> 8) + (tg)) >> 8;            \
    (d)[3] = (((tb) >> 8) + (tb)) >> 8;            \
  } G_STMT_END
#endif


/**
 * GIMP_CAIRO_ARGB32_GET_PIXEL:
 * @s: pointer to the source buffer
 * @r: red component, not pre-multiplied
 * @g: green component, not pre-multiplied
 * @b: blue component, not pre-multiplied
 * @a: alpha component
 *
 * Gets a single pixel from a Cairo image surface in %CAIRO_FORMAT_ARGB32.
 *
 * Since: 2.8
 **/
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GIMP_CAIRO_ARGB32_GET_PIXEL(s, r, g, b, a) \
  G_STMT_START {                                   \
    const guint tb = (s)[0];                       \
    const guint tg = (s)[1];                       \
    const guint tr = (s)[2];                       \
    const guint ta = (s)[3];                       \
    (r) = (tr << 8) / (ta + 1);                    \
    (g) = (tg << 8) / (ta + 1);                    \
    (b) = (tb << 8) / (ta + 1);                    \
    (a) = ta;                                      \
  } G_STMT_END
#else
#define GIMP_CAIRO_ARGB32_GET_PIXEL(s, r, g, b, a) \
  G_STMT_START {                                   \
    const guint ta = (s)[0];                       \
    const guint tr = (s)[1];                       \
    const guint tg = (s)[2];                       \
    const guint tb = (s)[3];                       \
    (r) = (tr << 8) / (ta + 1);                    \
    (g) = (tg << 8) / (ta + 1);                    \
    (b) = (tb << 8) / (ta + 1);                    \
    (a) = ta;                                      \
  } G_STMT_END
#endif


#endif /* __GIMP_CAIRO_H__ */
