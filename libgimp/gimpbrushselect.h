/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbrushselect.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_BRUSH_SELECT_H__
#define __GIMP_BRUSH_SELECT_H__

G_BEGIN_DECLS


typedef void (* GimpRunBrushCallback)   (const gchar          *brush_name,
                                         gdouble               opacity,
                                         gint                  spacing,
                                         GimpLayerModeEffects  paint_mode,
                                         gint                  width,
                                         gint                  height,
                                         const guchar         *mask_data,
                                         gboolean              dialog_closing,
                                         gpointer              user_data);


const gchar * gimp_brush_select_new     (const gchar          *title,
                                         const gchar          *brush_name,
                                         gdouble               opacity,
                                         gint                  spacing,
                                         GimpLayerModeEffects  paint_mode,
                                         GimpRunBrushCallback  callback,
                                         gpointer              data);
void          gimp_brush_select_destroy (const gchar          *brush_callback);


G_END_DECLS

#endif /* __GIMP_BRUSH_SELECT_H__ */
