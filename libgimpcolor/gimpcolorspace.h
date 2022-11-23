/* LIBLIGMA - The LIGMA Library
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

#if !defined (__LIGMA_COLOR_H_INSIDE__) && !defined (LIGMA_COLOR_COMPILATION)
#error "Only <libligmacolor/ligmacolor.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_SPACE_H__
#define __LIGMA_COLOR_SPACE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*  Color conversion routines  */


/*  LigmaRGB function  */

void   ligma_rgb_to_hsv          (const LigmaRGB  *rgb,
                                 LigmaHSV        *hsv);
void   ligma_rgb_to_hsl          (const LigmaRGB  *rgb,
                                 LigmaHSL        *hsl);
void   ligma_rgb_to_cmyk         (const LigmaRGB  *rgb,
                                 gdouble         pullout,
                                 LigmaCMYK       *cmyk);

void   ligma_hsv_to_rgb          (const LigmaHSV  *hsv,
                                 LigmaRGB        *rgb);
void   ligma_hsl_to_rgb          (const LigmaHSL  *hsl,
                                 LigmaRGB        *rgb);
void   ligma_cmyk_to_rgb         (const LigmaCMYK *cmyk,
                                 LigmaRGB        *rgb);

G_END_DECLS

#endif  /* __LIGMA_COLOR_SPACE_H__ */
