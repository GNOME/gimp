/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbrushselect.h
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

#ifndef __GIMP_BRUSH_SELECT_H__
#define __GIMP_BRUSH_SELECT_H__

G_BEGIN_DECLS


typedef void (* GimpRunBrushCallback)   (const gchar          *brush_name,
                                         gdouble               opacity,
                                         gint                  spacing,
                                         GimpLayerMode         paint_mode,
                                         gint                  width,
                                         gint                  height,
                                         const guchar         *mask_data,
                                         gboolean              dialog_closing,
                                         gpointer              user_data);


const gchar * gimp_brush_select_new     (const gchar          *title,
                                         const gchar          *brush_name,
                                         gdouble               opacity,
                                         gint                  spacing,
                                         GimpLayerMode         paint_mode,
                                         GimpRunBrushCallback  callback,
                                         gpointer              data);
void          gimp_brush_select_destroy (const gchar          *brush_callback);


G_END_DECLS

#endif /* __GIMP_BRUSH_SELECT_H__ */
