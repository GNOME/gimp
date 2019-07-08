/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui.h
 * Copyright (C) 2002-2017 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PROP_GUI_H__
#define __GIMP_PROP_GUI_H__


/*  A view on all of an object's properties  */

GtkWidget * gimp_prop_widget_new            (GObject                 *config,
                                             const gchar             *property_name,
                                             GeglRectangle           *area,
                                             GimpContext             *context,
                                             GimpCreatePickerFunc     create_picker,
                                             GimpCreateControllerFunc create_controller,
                                             gpointer                 creator,
                                             const gchar            **label);
GtkWidget * gimp_prop_widget_new_from_pspec (GObject                 *config,
                                             GParamSpec              *pspec,
                                             GeglRectangle           *area,
                                             GimpContext             *context,
                                             GimpCreatePickerFunc     create_picker,
                                             GimpCreateControllerFunc create_controller,
                                             gpointer                 creator,
                                             const gchar            **label);
GtkWidget * gimp_prop_gui_new               (GObject                 *config,
                                             GType                    owner_type,
                                             GParamFlags              flags,
                                             GeglRectangle           *area,
                                             GimpContext             *context,
                                             GimpCreatePickerFunc     create_picker,
                                             GimpCreateControllerFunc create_controller,
                                             gpointer                 creator);

void        gimp_prop_gui_bind_container    (GtkWidget               *source,
                                             GtkWidget               *target);
void        gimp_prop_gui_bind_label        (GtkWidget               *source,
                                             GtkWidget               *target);
void        gimp_prop_gui_bind_tooltip      (GtkWidget               *source,
                                             GtkWidget               *target);


#endif /* __GIMP_PROP_GUI_H__ */
