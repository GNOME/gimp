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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_COLOR_H_INSIDE__) && !defined (GIMP_COLOR_COMPILATION)
#error "Only <libgimpcolor/gimpcolor.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_SPACE_H__
#define __GIMP_COLOR_SPACE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*  Color conversion routines  */


/*  GimpRGB function  */

void   gimp_rgb_to_hsv          (const GimpRGB  *rgb,
                                 GimpHSV        *hsv);
void   gimp_rgb_to_hsl          (const GimpRGB  *rgb,
                                 GimpHSL        *hsl);
void   gimp_rgb_to_cmyk         (const GimpRGB  *rgb,
                                 gdouble         pullout,
                                 GimpCMYK       *cmyk);

void   gimp_hsv_to_rgb          (const GimpHSV  *hsv,
                                 GimpRGB        *rgb);
void   gimp_hsl_to_rgb          (const GimpHSL  *hsl,
                                 GimpRGB        *rgb);
void   gimp_cmyk_to_rgb         (const GimpCMYK *cmyk,
                                 GimpRGB        *rgb);


/*  gint functions  */

GIMP_DEPRECATED_FOR (gimp_rgb_to_hsv)
void    gimp_rgb_to_hsv_int     (gint    *red         /* returns hue        */,
                                 gint    *green       /* returns saturation */,
                                 gint    *blue        /* returns value      */);

GIMP_DEPRECATED_FOR (gimp_hsv_to_rgb)
void    gimp_hsv_to_rgb_int     (gint    *hue         /* returns red        */,
                                 gint    *saturation  /* returns green      */,
                                 gint    *value       /* returns blue       */);

GIMP_DEPRECATED_FOR (gimp_rgb_to_hsl)
void    gimp_rgb_to_hsl_int     (gint    *red         /* returns hue        */,
                                 gint    *green       /* returns saturation */,
                                 gint    *blue        /* returns lightness  */);

GIMP_DEPRECATED_FOR (gimp_rgb_to_hsl)
gint    gimp_rgb_to_l_int       (gint     red,
                                 gint     green,
                                 gint     blue);

GIMP_DEPRECATED_FOR (gimp_hsl_to_rgb)
void    gimp_hsl_to_rgb_int     (gint    *hue         /* returns red        */,
                                 gint    *saturation  /* returns green      */,
                                 gint    *lightness   /* returns blue       */);


G_END_DECLS

#endif  /* __GIMP_COLOR_SPACE_H__ */
