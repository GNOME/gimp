/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpenummenu.h
 * Copyright (C) 2002  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_ENUM_MENU_H__
#define __GIMP_ENUM_MENU_H__

#include <gtk/gtkmenu.h>


#define GIMP_TYPE_ENUM_MENU            (gimp_enum_menu_get_type ())
#define GIMP_ENUM_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ENUM_MENU, GimpEnumMenu))
#define GIMP_ENUM_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ENUM_MENU, GimpEnumMenuClass))
#define GIMP_IS_ENUM_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ENUM_MENU))
#define GIMP_IS_ENUM_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ENUM_MENU))
#define GIMP_ENUM_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ENUM_MENU, GimpEnumMenuClass))


typedef struct _GimpEnumMenuClass  GimpEnumMenuClass;

struct _GimpEnumMenuClass
{
  GtkMenuClass  parent_instance;
};

struct _GimpEnumMenu
{
  GtkMenu       parent_instance;

  GEnumClass   *enum_class;
};


GType       gimp_enum_menu_get_type               (void) G_GNUC_CONST;

GtkWidget * gimp_enum_menu_new                    (GType         enum_type,
                                                   GCallback     callback,
                                                   gpointer      callback_data);
GtkWidget * gimp_enum_menu_new_with_range         (GType         enum_type,
                                                   gint          minimum,
                                                   gint          maximum,
                                                   GCallback     callback,
                                                   gpointer      callback_data);
GtkWidget * gimp_enum_menu_new_with_values        (GType         enum_type,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   gint          n_values,
                                                   ...);
GtkWidget * gimp_enum_menu_new_with_values_valist (GType         enum_type,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   gint          n_values,
                                                   va_list       args);

void        gimp_enum_menu_set_stock_prefix       (GimpEnumMenu *enum_menu,
                                                   const gchar  *stock_prefix);

GtkWidget * gimp_enum_option_menu_new             (GType         enum_type,
                                                   GCallback     callback,
                                                   gpointer      callback_data);
GtkWidget * gimp_enum_option_menu_new_with_range  (GType         enum_type,
                                                   gint          minimum,
                                                   gint          maximum,
                                                   GCallback     callback,
                                                   gpointer      callback_data);
GtkWidget * gimp_enum_option_menu_new_with_values (GType         enum_type,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   gint          n_values,
                                                   ...);
GtkWidget *
gimp_enum_option_menu_new_with_values_valist      (GType         enum_type,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   gint          n_values,
                                                   va_list       args);

void      gimp_enum_option_menu_set_stock_prefix (GtkOptionMenu *option_menu,
                                                  const gchar   *stock_prefix);

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
                                                   gint          border_width,
                                                   GCallback     callback,
                                                   gpointer      callback_data,
                                                   GtkWidget   **first_button);
GtkWidget * gimp_enum_radio_frame_new_with_range  (GType         enum_type,
                                                   gint          minimum,
                                                   gint          maximum,
                                                   GtkWidget    *label_widget,
                                                   gint          border_width,
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


#endif  /* __GIMP_ENUM_MENU_H__ */
