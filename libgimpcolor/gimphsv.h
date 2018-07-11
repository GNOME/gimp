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

#if !defined (__GIMP_COLOR_H_INSIDE__) && !defined (GIMP_COLOR_COMPILATION)
#error "Only <libgimpcolor/gimpcolor.h> can be included directly."
#endif

#ifndef __GIMP_HSV_H__
#define __GIMP_HSV_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 * GIMP_TYPE_HSV
 */

#define GIMP_TYPE_HSV       (gimp_hsv_get_type ())

GType   gimp_hsv_get_type   (void) G_GNUC_CONST;

void    gimp_hsv_set        (GimpHSV       *hsv,
                             gdouble        hue,
                             gdouble        saturation,
                             gdouble        value);
void    gimp_hsv_clamp      (GimpHSV       *hsv);

void    gimp_hsva_set       (GimpHSV       *hsva,
                             gdouble        hue,
                             gdouble        saturation,
                             gdouble        value,
                             gdouble        alpha);


G_END_DECLS

#endif  /* __GIMP_HSV_H__ */
