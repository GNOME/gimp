/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorscales.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on color_notebook module
 * Copyright (C) 1998 Austin Donnelly <austin@greenend.org.uk>
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

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_SCALES_H__
#define __GIMP_COLOR_SCALES_H__

#include <libgimpwidgets/gimpcolorselector.h>

G_BEGIN_DECLS


#define GIMP_TYPE_COLOR_SCALES (gimp_color_scales_get_type ())
G_DECLARE_FINAL_TYPE (GimpColorScales, gimp_color_scales, GIMP, COLOR_SCALES, GimpColorSelector)


void       gimp_color_scales_set_show_rgb_u8 (GimpColorScales *scales,
                                              gboolean         show_rgb_u8);
gboolean   gimp_color_scales_get_show_rgb_u8 (GimpColorScales *scales);


G_END_DECLS

#endif /* __GIMP_COLOR_SCALES_H__ */
