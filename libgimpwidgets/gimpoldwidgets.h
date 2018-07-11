/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpoldwidgets.h
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
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

/*  These functions are deprecated and should not be used in newly
 *  written code.
 */

#ifndef GIMP_DISABLE_DEPRECATED

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_OLD_WIDGETS_H__
#define __GIMP_OLD_WIDGETS_H__

G_BEGIN_DECLS


/*
 *  Widget Constructors
 */

GtkWidget * gimp_int_option_menu_new         (gboolean        menu_only,
                                              GCallback       menu_item_callback,
                                              gpointer        menu_item_callback_data,
                                              gint            initial, /* item_data */

                                              /* specify menu items as va_list:
                                               *  gchar      *label,
                                               *  gint        item_data,
                                               *  GtkWidget **widget_ptr,
                                               */

                                              ...) G_GNUC_NULL_TERMINATED;

void        gimp_int_option_menu_set_history (GtkOptionMenu  *option_menu,
                                              gint            item_data);

typedef gboolean (* GimpIntOptionMenuSensitivityCallback) (gint     item_data,
                                                           gpointer callback_data);

void  gimp_int_option_menu_set_sensitive (GtkOptionMenu    *option_menu,
                                          GimpIntOptionMenuSensitivityCallback callback,
                                          gpointer          callback_data);


GtkWidget * gimp_option_menu_new     (gboolean         menu_only,

                                      /* specify menu items as va_list:
                                       *  gchar       *label,
                                       *  GCallback    callback,
                                       *  gpointer     callback_data,
                                       *  gpointer     item_data,
                                       *  GtkWidget  **widget_ptr,
                                       *  gboolean     active
                                       */

                                      ...) G_GNUC_NULL_TERMINATED;
GtkWidget * gimp_option_menu_new2    (gboolean          menu_only,
                                      GCallback         menu_item_callback,
                                      gpointer          menu_item_callback_data,
                                      gpointer          initial, /* item_data */

                                      /* specify menu items as va_list:
                                       *  gchar        *label,
                                       *  gpointer      item_data,
                                       *  GtkWidget   **widget_ptr,
                                       */
                                      ...) G_GNUC_NULL_TERMINATED;
void  gimp_option_menu_set_history   (GtkOptionMenu    *option_menu,
                                      gpointer          item_data);


typedef gboolean (* GimpOptionMenuSensitivityCallback) (gpointer item_data,
                                                        gpointer callback_data);

void  gimp_option_menu_set_sensitive (GtkOptionMenu    *option_menu,
                                      GimpOptionMenuSensitivityCallback callback,
                                      gpointer          callback_data);


void   gimp_menu_item_update         (GtkWidget        *widget,
                                      gpointer          data);

GtkWidget * gimp_pixmap_button_new   (gchar           **xpm_data,
                                      const gchar      *text);


/*
 *  Standard Callbacks
 */

void   gimp_toggle_button_sensitive_update (GtkToggleButton *toggle_button);

void   gimp_unit_menu_update               (GtkWidget       *widget,
                                            gpointer         data);


G_END_DECLS

#endif /* __GIMP_OLD_WIDGETS_H__ */

#endif /*  GIMP_DISABLE_DEPRECATED  */
