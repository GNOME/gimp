/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpenumwidgets.h
 * Copyright (C) 2002-2004  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_ENUM_WIDGETS_H__
#define __GIMP_ENUM_WIDGETS_H__


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

GtkWidget * gimp_enum_stock_box_new               (GType         enum_type,
                                                   const gchar  *stock_prefix,
                                                   GtkIconSize   icon_size,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   GtkWidget   **first_button);
GtkWidget * gimp_enum_stock_box_new_with_range    (GType         enum_type,
                                                   gint          minimum,
                                                   gint          maximum,
                                                   const gchar  *stock_prefix,
                                                   GtkIconSize   icon_size,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   GtkWidget   **first_button);

void        gimp_enum_stock_box_set_child_padding (GtkWidget    *stock_box,
                                                   gint          xpad,
                                                   gint          ypad);


#endif  /* __GIMP_ENUM_WIDGETS_H__ */
