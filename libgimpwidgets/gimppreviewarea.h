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

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_PREVIEW_AREA_H__
#define __GIMP_PREVIEW_AREA_H__

G_BEGIN_DECLS


#define GIMP_TYPE_PREVIEW_AREA (gimp_preview_area_get_type ())
G_DECLARE_FINAL_TYPE (GimpPreviewArea, gimp_preview_area, GIMP, PREVIEW_AREA, GtkDrawingArea)


GtkWidget * gimp_preview_area_new              (void);

void        gimp_preview_area_draw             (GimpPreviewArea *area,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height,
                                                GimpImageType    type,
                                                const guchar    *buf,
                                                gint             rowstride);
void        gimp_preview_area_blend            (GimpPreviewArea *area,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height,
                                                GimpImageType    type,
                                                const guchar    *buf1,
                                                gint             rowstride1,
                                                const guchar    *buf2,
                                                gint             rowstride2,
                                                guchar           opacity);
void        gimp_preview_area_mask             (GimpPreviewArea *area,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height,
                                                GimpImageType    type,
                                                const guchar    *buf1,
                                                gint             rowstride1,
                                                const guchar    *buf2,
                                                gint             rowstride2,
                                                const guchar    *mask,
                                                gint             rowstride_mask);
void        gimp_preview_area_fill             (GimpPreviewArea *area,
                                                gint             x,
                                                gint             y,
                                                gint             width,
                                                gint             height,
                                                guchar           red,
                                                guchar           green,
                                                guchar           blue);
void        gimp_preview_area_reset            (GimpPreviewArea *area);

void        gimp_preview_area_set_offsets      (GimpPreviewArea *area,
                                                gint             x,
                                                gint             y);

void        gimp_preview_area_set_colormap     (GimpPreviewArea *area,
                                                const guchar    *colormap,
                                                gint             num_colors);

void        gimp_preview_area_set_color_config (GimpPreviewArea *area,
                                                GimpColorConfig *config);

void        gimp_preview_area_get_size         (GimpPreviewArea *area,
                                                gint            *width,
                                                gint            *height);
void        gimp_preview_area_set_max_size     (GimpPreviewArea *area,
                                                gint             width,
                                                gint             height);

void        gimp_preview_area_menu_popup       (GimpPreviewArea *area,
                                                GdkEventButton  *event);


G_END_DECLS

#endif /* __GIMP_PREVIEW_AREA_H__ */
