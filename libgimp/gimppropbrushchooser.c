
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
#include <gegl-paramspecs.h>
#include <gtk/gtk.h>

#include "gimpui.h"

/* Several of these local functions were copied from gimppropwidgets.
 * FUTURE: make utility functions shared by prop widgets.
 */

/* When property_name of object changes, call the callback passing callback_data. */
static void
connect_notify_property_changed (GObject     *object,
                                 const gchar *property_name,
                                 GCallback    callback,
                                 gpointer     callback_data)
{
  gchar *notify_name;

  notify_name = g_strconcat ("notify::", property_name, NULL);

  g_signal_connect_object (object, notify_name, callback, callback_data, 0);

  g_free (notify_name);
}


/* Setter and getters for a config property bound to a widget.
 *
 * A prop widget is responsible for knowing the config property it is bound to.
 * The config property is a pair [config, property_name]
 *
 * The property name is unique among properties of the config.
 * But it is not constant, and the config can have many similarly named properties,
 * when the config has many properties each a value of type say GimpBrush.
 * For example, the widget may have property "brush" but the config have "brush2"
 *
 * We arbitrarily store it in data, not in yet another property.
 */

static void
gimp_widget_set_bound_property (GtkWidget   *widget,
                                GObject     *config,
                                const gchar *property_name)
{
  g_debug ("%s: %s", G_STRFUNC, property_name);

  g_return_if_fail (GTK_IS_WIDGET (widget));

  /* Widget destruction notifies this and frees the string. */
  g_object_set_data_full (G_OBJECT (widget),
                          "gimp-widget-property-name",
                          g_strdup (property_name),
                          (GDestroyNotify) g_free);
  g_object_set_data_full (G_OBJECT (widget),
                          "gimp-widget-property-config",
                          g_object_ref (config),
                          (GDestroyNotify) g_object_unref);
}

/* The result string is owned by the widget and is destroyed with the widget.
 * Caller must copy it if caller needs it after the widget is destroyed.
 */
static const gchar *
gimp_widget_get_bound_property_name (GtkWidget   *widget)
{
  gchar * result=NULL;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  result = (gchar *) g_object_get_data (G_OBJECT (widget),
                "gimp-widget-property-name");

  /* Ensure result is NULL when set_bound_property was not called prior. */
  g_debug ("bound to %s", result);
  return result;
}

static GObject *
gimp_widget_get_bound_property_owner (GtkWidget   *widget)
{
  GObject * result=NULL;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  result = g_object_get_data (G_OBJECT (widget),
                "gimp-widget-property-config");

  /* Ensure result is NULL if the widget lacks the property.
   * When set_bound_property was not called prior.
   */
   g_debug("bound to owner %p", result);
  return result;
}

static GParamSpec *
find_param_spec (GObject     *object,
                 const gchar *property_name,
                 const gchar *strloc)
{
  GParamSpec *param_spec;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                             property_name);

  if (! param_spec)
    g_warning ("%s: %s has no property named '%s'",
               strloc,
               g_type_name (G_TYPE_FROM_INSTANCE (object)),
               property_name);

  return param_spec;
}

#ifdef DEBUG
/* Print the ID of a GimpResource */
static void
debug_resource_id(GimpBrush *property_value)
{
  gchar * id;

  g_object_get (GIMP_RESOURCE(property_value), "id", &id, NULL);
  g_debug ("brush's id: %s", id);
}
#else
static void debug_resource_id(GimpBrush *property_value) {}
#endif


/* Called when the "brush" property of the widget changes.
 *
 * Do a prop widget's essential behavior:
 * copy from widget property to GimpProcedureConfig property.
 *
 * Require the widget to have previously bound the config's property to itself.

 * We don't compare current value with new value:
 * that is an optimization to reduce signals.
 * Only we are subscribed to this notify signal.
 * The config is only being used to marshall args to a procedure call.
 */
static void
gimp_prop_brush_chooser_button_notify (GtkWidget *button)
{
  GimpBrush   *widget_property_value = NULL;
  GimpBrush   *config_property_value = NULL;

  /* This pair designates a property of a GimpProcedureConfig. */
  GObject     *config;
  const gchar *config_property_name;

  /* Get changed value of the widget's property */
  g_object_get (button, "brush", &widget_property_value, NULL);

  debug_resource_id (widget_property_value);

  /* Get the name of the config property the widget is bound to. */
  config = gimp_widget_get_bound_property_owner (button);
  config_property_name = gimp_widget_get_bound_property_name (button);
  g_assert (config_property_name != NULL);

   /* FUTURE: migrate much of this code to a new method
    * GimpProcedureConfig.replace_object_value(property_name, new_value)
    * It would know to free the existing value.
    */

  /* Free the existing config property.
    *
    * Properties of type GObject might not have default value,
    * so expect it can be NULL, when the config (settings) have never been serialized.
    *
    * Here we get a pointer to the object, and g_object_clear it.
    * The config still points to the freed object, but we soon replace it.
    */
  g_debug ("notify callback, get property: %s from config. ", config_property_name);
  g_object_get (config, config_property_name, &config_property_value, NULL);
  g_clear_object (&config_property_value);

  /* Set the config's property to the widget's property */
  g_object_set (config, config_property_name, widget_property_value, NULL);
  /* Assert that g_object_set refs the value,
   * so value won't be freed when widget closes.
   */
}


/**
 * gimp_prop_brush_chooser_button_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of property controlled by button.
 *
 * Creates a #GimpBrushSelectButton that displays and sets the property,
 * which is a GimpBrush.
 *
 * The button is labeled with the @property_name's nick.
 *
 * When pushed, the button shows a dialog that lets the user choose a brush.
 *
 * Returns: (transfer full): The newly created #GimpBrushSelectButton widget.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_brush_chooser_button_new (GObject             *config,
                                    const gchar          *property_name,
                                    const gchar          *title)
{
  GParamSpec *param_spec;
  GtkWidget  *button;
  GimpBrush  *brush;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  g_debug ("config is %p", config);
  g_debug ("property name is %s", property_name);

  param_spec = find_param_spec (config, property_name, G_STRFUNC);
  if (! param_spec)
    {
      g_warning ("%s: %s has no property named '%s'",
                 G_STRFUNC, g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return NULL;
    }

  if (! G_IS_PARAM_SPEC_OBJECT (param_spec) || param_spec->value_type != GIMP_TYPE_BRUSH)
    {
      g_warning ("%s: property '%s' of %s is not a G_PARAM_SPEC_OBJECT of value type GIMP_TYPE_BRUSH.",
                 G_STRFUNC, param_spec->name,
                 g_type_name (param_spec->owner_type));
      return NULL;
    }

  if (! title)
    title = g_param_spec_get_nick (param_spec);

  /* Initial choice in chooser is the brush of the config. */
  /* Assert the property exists, but might be NULL. */
  g_object_get (config, property_name, &brush, NULL);
  if (brush == NULL)
    {
      /* Property has no default.
       * Get a reasonable value from context.
       * The widget will show some value selected,
       * ensure it is the the value in context.
       */
      g_warning ("No brush property in config. Using brush from context.");
      brush = gimp_context_get_brush();
    }

  debug_resource_id (brush);

  /* FIXME: use a widget that chooses only a brush,
   * and not opacity, spacing, layer_mode.
   */
  button = gimp_brush_select_button_new (title, brush,
                                         1.0, 2, GIMP_LAYER_MODE_NORMAL);

  /* Unlike other prop widgets, we don't use a callback for a signal of the widget.
   * And we don't bind directly to a property.
   * Instead get a callback for the notify signal of the widget "brush" property.
   * Handler will copy widget's property to config's property.
   */
  connect_notify_property_changed (
    (GObject *)button, "brush", /* Source of signal. */
    G_CALLBACK (gimp_prop_brush_chooser_button_notify), /* signal handler. */
    button);  /* Data passed to handler. */

  /* widget knows what config property it is bound to. */
  gimp_widget_set_bound_property (button, config, property_name);

  /* FIXME: label the widget with the nick of the paramspec. */

  gtk_widget_show (button);

  return button;
}
