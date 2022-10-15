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


/* PropChooser
 * A GtkWidget, more specifically a GtkButtonWidget.
 * Pops up a chooser widget.
 * Has Prop widget trait, i.e. binds property of the chooser widget
 * to a property of a ProcedureConfig.
 *
 * For now, only for Resource choosers,
 * which are named GimpResourceSelectButton ("select" means "choose")
 *
 * Call a factory to create PropWidgets, passing a creator function of
 * a kind of wrapped widget.
 *
 * Stack of objects:
 * PropWidget<Resource> (this file)
 * <Resource>SelectButton button pops up a
 * <Resource>Select dialog lets user choose <resource>
 * libgimp wire protocol to core
 * PDBDialog (app/widgets/gimppdbdialog.c)
 */

/* Only brush is annotated.
 * Usually these are created by GimpProcedureDialog.
 * FIXME: It is not clear that these need to be public API,
 * unless a plugin creates its own dialog.
 */

/**
 * gimp_prop_chooser_brush_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of property controlled by button.
 * @chooser_title: Title for the popup chooser dialog.
 *
 * Creates a #GimpBrushSelectButton that displays and sets the property.
 *
 * @config is usually a GimpProcedureConfig (but it could be otherwise.)
 * The @config must have a property with name @property_name.
 * The property must be type #GimpBrush.
 * The @property_name need not be "brush",
 * since the @config may have more than one property of type #GimpBrush.
 *
 * The button is labeled with the @property_name's nick.
 *
 * When pushed, the button pops up a dialog that lets the user choose a brush.
 * The dialog will have the given title.
 *
 * Returns: (transfer full): The newly created #GimpBrushSelectButton widget.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_chooser_brush_new (GObject *config, const gchar *property_name, const gchar *title)
{
 return gimp_prop_chooser_factory (gimp_brush_select_button_new, config, property_name, title);
};

GtkWidget *
gimp_prop_chooser_font_new (GObject *config, const gchar *property_name, const gchar *title)
{
  return gimp_prop_chooser_factory (gimp_font_select_button_new, config, property_name, title);
};

GtkWidget *
gimp_prop_chooser_gradient_new (GObject *config, const gchar *property_name, const gchar *title)
{
  return gimp_prop_chooser_factory (gimp_gradient_select_button_new, config, property_name, title);
};

GtkWidget *
gimp_prop_chooser_palette_new (GObject *config, const gchar *property_name, const gchar *title)
{
  return gimp_prop_chooser_factory (gimp_palette_select_button_new, config, property_name, title);
};

GtkWidget *
gimp_prop_chooser_pattern_new (GObject *config, const gchar *property_name, const gchar *title)
{
  return gimp_prop_chooser_factory (gimp_pattern_select_button_new, config, property_name, title);
};
