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

#ifndef __LIGMA_HSV_H__
#define __LIGMA_HSV_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 * LIGMA_TYPE_HSV
 */

#define LIGMA_TYPE_HSV       (ligma_hsv_get_type ())

GType   ligma_hsv_get_type   (void) G_GNUC_CONST;

void    ligma_hsv_set        (LigmaHSV       *hsv,
                             gdouble        hue,
                             gdouble        saturation,
                             gdouble        value);
void    ligma_hsv_clamp      (LigmaHSV       *hsv);

void    ligma_hsva_set       (LigmaHSV       *hsva,
                             gdouble        hue,
                             gdouble        saturation,
                             gdouble        value,
                             gdouble        alpha);


G_END_DECLS

#endif  /* __LIGMA_HSV_H__ */
