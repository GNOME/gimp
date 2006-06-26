/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpgradientmenu.h
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

#ifndef __GIMP_GRAIDENT_MENU_H__
#define __GIMP_GRADIENT_MENU_H__

/*  These functions are deprecated and should not be used in newly
 *  written code.
 */

#ifndef GIMP_DISABLE_DEPRECATED

G_BEGIN_DECLS


GtkWidget * gimp_gradient_select_widget_new   (const gchar      *title,
                                               const gchar      *gradient_name,
                                               GimpRunGradientCallback  callback,
                                               gpointer          data);

void        gimp_gradient_select_widget_close (GtkWidget        *widget);
void        gimp_gradient_select_widget_set   (GtkWidget        *widget,
                                               const gchar      *gradient_name);


G_END_DECLS

#endif /*  GIMP_DISABLE_DEPRECATED  */

#endif /* __GIMP_GRADIENT_MENU_H__ */
