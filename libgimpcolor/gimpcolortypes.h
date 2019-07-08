/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_COLOR_TYPES_H__
#define __GIMP_COLOR_TYPES_H__


#include <libgimpbase/gimpbasetypes.h>
#include <libgimpconfig/gimpconfigtypes.h>


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


typedef struct _GimpColorManaged   GimpColorManaged;  /* dummy typedef */
typedef struct _GimpColorProfile   GimpColorProfile;
typedef struct _GimpColorTransform GimpColorTransform;


/*  usually we don't keep the structure definitions in the types file
 *  but GimpRGB appears in too many header files...
 */

typedef struct _GimpRGB  GimpRGB;
typedef struct _GimpHSV  GimpHSV;
typedef struct _GimpHSL  GimpHSL;
typedef struct _GimpCMYK GimpCMYK;

/**
 * GimpRGB:
 * @r: the red component
 * @g: the green component
 * @b: the blue component
 * @a: the alpha component
 *
 * Used to keep RGB and RGBA colors. All components are in a range of
 * [0.0..1.0].
 **/
struct _GimpRGB
{
  gdouble r, g, b, a;
};

/**
 * GimpHSV:
 * @h: the hue component
 * @s: the saturation component
 * @v: the value component
 * @a: the alpha component
 *
 * Used to keep HSV and HSVA colors. All components are in a range of
 * [0.0..1.0].
 **/
struct _GimpHSV
{
  gdouble h, s, v, a;
};

/**
 * GimpHSL:
 * @h: the hue component
 * @s: the saturation component
 * @l: the lightness component
 * @a: the alpha component
 *
 * Used to keep HSL and HSLA colors. All components are in a range of
 * [0.0..1.0].
 **/
struct _GimpHSL
{
  gdouble h, s, l, a;
};

/**
 * GimpCMYK:
 * @c: the cyan component
 * @m: the magenta component
 * @y: the yellow component
 * @k: the black component
 * @a: the alpha component
 *
 * Used to keep CMYK and CMYKA colors. All components are in a range
 * of [0.0..1.0]. An alpha value is somewhat useless in the CMYK
 * colorspace, but we keep one around anyway so color conversions
 * going to CMYK and back can preserve alpha.
 **/
struct _GimpCMYK
{
  gdouble c, m, y, k, a;
};


typedef void (* GimpRenderFunc)   (gdouble   x,
                                   gdouble   y,
                                   GimpRGB  *color,
                                   gpointer  data);
typedef void (* GimpPutPixelFunc) (gint      x,
                                   gint      y,
                                   GimpRGB  *color,
                                   gpointer  data);
typedef void (* GimpProgressFunc) (gint      min,
                                   gint      max,
                                   gint      current,
                                   gpointer  data);


G_END_DECLS

#endif  /* __GIMP_COLOR_TYPES_H__ */
