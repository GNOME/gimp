/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorarea.h
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

/* This provides a color preview area. The preview
 * can handle transparency by showing the checkerboard and
 * handles drag'n'drop.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_AREA_H__
#define __GIMP_COLOR_AREA_H__

G_BEGIN_DECLS


#define GIMP_TYPE_COLOR_AREA (gimp_color_area_get_type ())
G_DECLARE_FINAL_TYPE (GimpColorArea, gimp_color_area, GIMP, COLOR_AREA, GtkDrawingArea)


GtkWidget * gimp_color_area_new              (GeglColor         *color,
                                              GimpColorAreaType  type,
                                              GdkModifierType    drag_mask);

void        gimp_color_area_set_color        (GimpColorArea     *area,
                                              GeglColor         *color);
GeglColor * gimp_color_area_get_color        (GimpColorArea     *area);

gboolean    gimp_color_area_has_alpha        (GimpColorArea     *area);
void        gimp_color_area_set_type         (GimpColorArea     *area,
                                              GimpColorAreaType  type);
void        gimp_color_area_enable_drag      (GimpColorArea     *area,
                                              GdkModifierType    drag_mask);
void        gimp_color_area_set_draw_border  (GimpColorArea     *area,
                                              gboolean           draw_border);
void        gimp_color_area_set_out_of_gamut (GimpColorArea     *area,
                                              gboolean           out_of_gamut);

void        gimp_color_area_set_color_config (GimpColorArea     *area,
                                              GimpColorConfig   *config);


G_END_DECLS

#endif /* __GIMP_COLOR_AREA_H__ */
