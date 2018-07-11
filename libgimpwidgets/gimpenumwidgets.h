/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenumwidgets.h
 * Copyright (C) 2002-2004  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_ENUM_WIDGETS_H__
#define __GIMP_ENUM_WIDGETS_H__

G_BEGIN_DECLS


GtkWidget * gimp_enum_radio_box_new               (GType         enum_type,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   GtkWidget   **first_button);
GtkWidget * gimp_enum_radio_box_new_with_range    (GType         enum_type,
                                                   gint          minimum,
                                                   gint          maximum,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   GtkWidget   **first_button);

GtkWidget * gimp_enum_radio_frame_new             (GType         enum_type,
                                                   GtkWidget    *label_widget,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   GtkWidget   **first_button);
GtkWidget * gimp_enum_radio_frame_new_with_range  (GType         enum_type,
                                                   gint          minimum,
                                                   gint          maximum,
                                                   GtkWidget    *label_widget,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   GtkWidget   **first_button);

GIMP_DEPRECATED_FOR(gimp_enum_icon_box_new)
GtkWidget * gimp_enum_stock_box_new               (GType         enum_type,
                                                   const gchar  *stock_prefix,
                                                   GtkIconSize   icon_size,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   GtkWidget   **first_button);
GIMP_DEPRECATED_FOR(gimp_enum_icon_box_new_with_range)
GtkWidget * gimp_enum_stock_box_new_with_range    (GType         enum_type,
                                                   gint          minimum,
                                                   gint          maximum,
                                                   const gchar  *stock_prefix,
                                                   GtkIconSize   icon_size,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   GtkWidget   **first_button);
GIMP_DEPRECATED_FOR(gimp_enum_icon_box_set_child_padding)
void        gimp_enum_stock_box_set_child_padding (GtkWidget    *stock_box,
                                                   gint          xpad,
                                                   gint          ypad);

GtkWidget * gimp_enum_icon_box_new                (GType         enum_type,
                                                   const gchar  *icon_prefix,
                                                   GtkIconSize   icon_size,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   GtkWidget   **first_button);
GtkWidget * gimp_enum_icon_box_new_with_range     (GType         enum_type,
                                                   gint          minimum,
                                                   gint          maximum,
                                                   const gchar  *icon_prefix,
                                                   GtkIconSize   icon_size,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   GtkWidget   **first_button);
void        gimp_enum_icon_box_set_child_padding  (GtkWidget    *icon_box,
                                                   gint          xpad,
                                                   gint          ypad);

G_END_DECLS

#endif  /* __GIMP_ENUM_WIDGETS_H__ */
