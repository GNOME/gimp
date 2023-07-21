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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

static GimpResource * get_initial_resource_from_config (GObject     *config,
                                                        const gchar *property_name);

/**
* gimp_prop_chooser_factory:
* @widget_creator_func: (scope async): Function that creates a chooser widget
* @config:        Object to which property is attached.
* @property_name: Name of property set by the widget.
* @chooser_title: Title for the popup chooser dialog.
*
* Creates a #GimpResourceSelectButton that displays
* and sets the named property of the config.
*
* The factory makes many kinds of widgets.
* Parameterized by passing a creator function for a kind of widget.
* E.G. creator function is gimp_brush_select_button_new.
* The created widget must have a property named "resource".
*
* The factory wraps the widget so that it is a *prop* widget.
* A prop widget gets the initial choice from the @config
* and binds the property named @property_name
* of the @config to the widget's "resource" property.
*
* @config is usually a #GimpProcedureConfig (but it could be otherwise.)
* The @config must have a property with name @property_name.
* The property must be of type that matches that of the @widget_creator_func,
* e.g. #GimpBrush.
* The @property_name need not be "brush",
* since the @config may have more than one property of the same type e.g. #GimpBrush.
*
* Returns: (transfer full): The newly created #GimpResourceSelectButton widget.
*
* Since: 3.0
*/
GtkWidget *
gimp_prop_chooser_factory (GimpResourceWidgetCreator  widget_creator_func,
                           GObject                   *config,
                           const gchar               *property_name,
                           const gchar               *chooser_title)
{
  GtkWidget    *result_widget;
  GimpResource *initial_resource;

  g_debug ("%s called", G_STRFUNC);

  initial_resource = get_initial_resource_from_config (config, property_name);

  /* initial_resource may be NULL.
   * When NULL, the widget creator will set it's resource property from context.
   * We bind with G_BINDING_SYNC_CREATE which immediately flows widget to config.
   * So the config property is not NULL after this.
   */

  /* Create the wrapped widget. For example, call gimp_font_select_button_new.*/
  result_widget = widget_creator_func (chooser_title, initial_resource);
  g_object_unref (initial_resource);

  /* Bind the wrapped widget's property to the config's property.
   *
   * The property name of the config can vary, e.g. "font" or "font1"
   * The property name on widget is generic "resource" not specific e.g. "font"
   *
   * We bind G_BINDING_BIDIRECTIONAL.
   * But we expect no other actor beside widget will change the config,
   * (at least while the widget lives.)
   * Bind from widget source to config target,
   * because G_BINDING_SYNC_CREATE initially flows that way.
   */
  g_object_bind_property (result_widget, /* Source for initial transfer */
                          "resource",
                          config,        /* Target of initial transfer. */
                          property_name,
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  return result_widget;
}


static GimpResource *
get_initial_resource_from_config (GObject     *config,
                                  const gchar *property_name)
{
  GimpResource *initial_resource;

  g_debug ("%s called", G_STRFUNC);

  g_object_get (config,
                property_name, &initial_resource,
                NULL);

  return initial_resource;
}
