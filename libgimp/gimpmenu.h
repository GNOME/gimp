/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmenu.h
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

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_MENU_H__
#define __GIMP_MENU_H__

/*  These functions are deprecated and should not be used in newly
 *  written code.
 */

G_BEGIN_DECLS

#ifndef GIMP_DISABLE_DEPRECATED

typedef gboolean (* GimpConstraintFunc) (gint32   image_id,
                                         gint32   drawable_id,
                                         gpointer data);
typedef void     (* GimpMenuCallback)   (gint32   any_id,
                                         gpointer data);


GIMP_DEPRECATED_FOR(gimp_image_combo_box_new)
GtkWidget * gimp_image_menu_new    (GimpConstraintFunc constraint,
                                    GimpMenuCallback   callback,
                                    gpointer           data,
                                    gint32             active_image);
GIMP_DEPRECATED_FOR(gimp_layer_combo_box_new)
GtkWidget * gimp_layer_menu_new    (GimpConstraintFunc constraint,
                                    GimpMenuCallback   callback,
                                    gpointer           data,
                                    gint32             active_layer);
GIMP_DEPRECATED_FOR(gimp_channel_combo_box_new)
GtkWidget * gimp_channel_menu_new  (GimpConstraintFunc constraint,
                                    GimpMenuCallback   callback,
                                    gpointer           data,
                                    gint32             active_channel);
GIMP_DEPRECATED_FOR(gimp_drawable_combo_box_new)
GtkWidget * gimp_drawable_menu_new (GimpConstraintFunc constraint,
                                    GimpMenuCallback   callback,
                                    gpointer           data,
                                    gint32             active_drawable);

#endif /*  GIMP_DISABLE_DEPRECATED  */

G_END_DECLS

#endif /* __GIMP_MENU_H__ */
