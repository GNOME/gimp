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
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

 #if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
 #error "Only <libgimp/gimpui.h> can be included directly."
 #endif

#ifndef __GIMP_PROP_CHOOSER_FACTORY_H__
#define __GIMP_PROP_CHOOSER_FACTORY_H__

G_BEGIN_DECLS

/**
* GimpResourceWidgetCreator:
* @title: title of the popup chooser dialog
* @initial_resource: Resource to show as initial choice
*
* Function producing a chooser widget for a specific GimpResource object type.
* e.g. gimp_font_select_button_new
* Widgets for resource subclass choosers all have the same signature.
**/
typedef GtkWidget* (*GimpResourceWidgetCreator)(const gchar* title, GimpResource * initial_resource);

GtkWidget *gimp_prop_chooser_factory (GimpResourceWidgetCreator  widget_creator_func,
                                      GObject                   *config,
                                      const gchar               *property_name,
                                      const gchar               *chooser_title);

G_END_DECLS

#endif /* __GIMP_PROP_CHOOSER_FACTORY_H__ */
