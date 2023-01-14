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
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */


#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __LIBGIMP_GIMP_RESOURCE_SUBCLASS_H__
#define __LIBGIMP_GIMP_RESOURCE_SUBCLASS_H__

G_BEGIN_DECLS


/*
 * Subclasses of GimpResource.
 *
 * id_is_valid and other methods are in pdb/groups/<foo>.pd
 */


#define GIMP_TYPE_BRUSH (gimp_brush_get_type ())
G_DECLARE_FINAL_TYPE (GimpBrush, gimp_brush, GIMP, BRUSH, GimpResource)

gboolean  gimp_brush_is_valid (GimpBrush *self);


#define GIMP_TYPE_FONT (gimp_font_get_type ())
G_DECLARE_FINAL_TYPE (GimpFont, gimp_font, GIMP, FONT, GimpResource)

gboolean  gimp_font_is_valid (GimpFont *self);


#define GIMP_TYPE_GRADIENT (gimp_gradient_get_type ())
G_DECLARE_FINAL_TYPE (GimpGradient, gimp_gradient, GIMP, GRADIENT, GimpResource)

gboolean  gimp_gradient_is_valid (GimpGradient *self);


#define GIMP_TYPE_PALETTE (gimp_palette_get_type ())
G_DECLARE_FINAL_TYPE (GimpPalette, gimp_palette, GIMP, PALETTE, GimpResource)

gboolean  gimp_palette_is_valid (GimpPalette *self);


#define GIMP_TYPE_PATTERN (gimp_pattern_get_type ())
G_DECLARE_FINAL_TYPE (GimpPattern, gimp_pattern, GIMP, PATTERN, GimpResource)

gboolean  gimp_pattern_is_valid (GimpPattern *self);


G_END_DECLS

#endif  /*  __LIBGIMP_GIMP_RESOURCE_SUBCLASS_H__  */
