/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "gimprectangleoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_HIGHLIGHT,
  PROP_FIXED_WIDTH,
  PROP_WIDTH,
  PROP_FIXED_HEIGHT,
  PROP_HEIGHT,
  PROP_FIXED_ASPECT,
  PROP_ASPECT,
  PROP_FIXED_CENTER,
  PROP_CENTER_X,
  PROP_CENTER_Y,
  PROP_UNIT
};


static void   gimp_rectangle_options_iface_base_init    (GimpRectangleOptionsInterface *rectangle_options_iface);


GType
gimp_rectangle_options_interface_get_type (void)
{
  static GType rectangle_options_iface_type = 0;

  if (!rectangle_options_iface_type)
    {
      static const GTypeInfo rectangle_options_iface_info =
      {
        sizeof (GimpRectangleOptionsInterface),
	      (GBaseInitFunc)     gimp_rectangle_options_iface_base_init,
	      (GBaseFinalizeFunc) NULL,
      };

      rectangle_options_iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                                             "GimpRectangleOptionsInterface",
                                                             &rectangle_options_iface_info,
                                                             0);
    }

  return rectangle_options_iface_type;
}

static void
gimp_rectangle_options_iface_base_init (GimpRectangleOptionsInterface *rectangle_options_iface)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      initialized = TRUE;
    }
}

void
gimp_rectangle_options_set_highlight (GimpRectangleOptions *options,
                                      gboolean              highlight)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->set_highlight)
    options_iface->set_highlight (options, highlight);
}

gboolean
gimp_rectangle_options_get_highlight (GimpRectangleOptions *options)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->get_highlight)
    return options_iface->get_highlight (options);

  return FALSE;
}

void
gimp_rectangle_options_set_fixed_width (GimpRectangleOptions *options,
                                        gboolean              fixed_width)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->set_fixed_width)
    options_iface->set_fixed_width (options, fixed_width);
}

gboolean
gimp_rectangle_options_get_fixed_width (GimpRectangleOptions *options)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->get_fixed_width)
    return options_iface->get_fixed_width (options);

  return FALSE;
}

void
gimp_rectangle_options_set_width (GimpRectangleOptions *options,
                                  gdouble               width)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->set_width)
    options_iface->set_width (options, width);
}

gdouble
gimp_rectangle_options_get_width (GimpRectangleOptions *options)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), 0);

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->get_width)
    return options_iface->get_width (options);

  return 0;
}

void
gimp_rectangle_options_set_fixed_height (GimpRectangleOptions *options,
                                         gboolean              fixed_height)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->set_fixed_height)
    options_iface->set_fixed_height (options, fixed_height);
}

gboolean
gimp_rectangle_options_get_fixed_height (GimpRectangleOptions *options)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->get_fixed_height)
    return options_iface->get_fixed_height (options);

  return FALSE;
}

void
gimp_rectangle_options_set_height (GimpRectangleOptions *options,
                                   gdouble               height)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->set_height)
    options_iface->set_height (options, height);
}

gdouble
gimp_rectangle_options_get_height (GimpRectangleOptions *options)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), 0);

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->get_height)
    return options_iface->get_height (options);

  return 0;
}

void
gimp_rectangle_options_set_fixed_aspect (GimpRectangleOptions *options,
                                         gboolean              fixed_aspect)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->set_fixed_aspect)
    options_iface->set_fixed_aspect (options, fixed_aspect);
}

gboolean
gimp_rectangle_options_get_fixed_aspect (GimpRectangleOptions *options)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->get_fixed_aspect)
    return options_iface->get_fixed_aspect (options);

  return FALSE;
}

void
gimp_rectangle_options_set_aspect (GimpRectangleOptions *options,
                                   gdouble               aspect)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->set_aspect)
    options_iface->set_aspect (options, aspect);
}

gdouble
gimp_rectangle_options_get_aspect (GimpRectangleOptions *options)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), 0);

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->get_aspect)
    return options_iface->get_aspect (options);

  return 0;
}

void
gimp_rectangle_options_set_fixed_center (GimpRectangleOptions *options,
                                         gboolean              fixed_center)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->set_fixed_center)
    options_iface->set_fixed_center (options, fixed_center);
}

gboolean
gimp_rectangle_options_get_fixed_center (GimpRectangleOptions *options)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->get_fixed_center)
    return options_iface->get_fixed_center (options);

  return FALSE;
}

void
gimp_rectangle_options_set_center_x (GimpRectangleOptions *options,
                                     gdouble               center_x)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->set_center_x)
    options_iface->set_center_x (options, center_x);
}

gdouble
gimp_rectangle_options_get_center_x (GimpRectangleOptions *options)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), 0);

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->get_center_x)
    return options_iface->get_center_x (options);

  return 0;
}

void
gimp_rectangle_options_set_center_y (GimpRectangleOptions *options,
                                     gdouble               center_y)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->set_center_y)
    options_iface->set_center_y (options, center_y);
}

gdouble
gimp_rectangle_options_get_center_y (GimpRectangleOptions *options)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), 0);

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->get_center_y)
    return options_iface->get_center_y (options);

  return 0;
}

void
gimp_rectangle_options_set_unit (GimpRectangleOptions *options,
                                 GimpUnit              unit)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->set_unit)
    options_iface->set_unit (options, unit);
}

GimpUnit
gimp_rectangle_options_get_unit (GimpRectangleOptions *options)
{
  GimpRectangleOptionsInterface *options_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), GIMP_UNIT_PIXEL);

  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (options_iface->get_unit)
    return options_iface->get_unit (options);

  return GIMP_UNIT_PIXEL;
}

GtkWidget *
gimp_rectangle_options_gui (GimpToolOptions *tool_options)
{
  GObject     *config  = G_OBJECT (tool_options);
  GtkWidget   *vbox;
  GtkWidget   *button;
  GtkWidget   *controls_container;
  GtkWidget   *table;
  GtkWidget   *entry;
  GtkWidget   *hbox;
  GtkWidget   *label;
  GtkWidget   *spinbutton;

  vbox = gimp_tool_options_gui (tool_options);

  button = gimp_prop_check_button_new (config, "highlight",
                                       _("Highlight"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  table = gtk_table_new (6, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);

  label = gtk_label_new (_("Fix"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  button = gimp_prop_check_button_new (config, "fixed-width",
                                       _("Width"));
  gtk_widget_show (button);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 1, 2);
  entry = gimp_prop_size_entry_new (config, "width", "unit", "%a",
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE, 300);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (entry), FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 1, 2);
  gtk_widget_show (entry);

  button = gimp_prop_check_button_new (config, "fixed-height",
                                       _("Height"));
  gtk_widget_show (button);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 2, 3);
  entry = gimp_prop_size_entry_new (config, "height", "unit", "%a",
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE, 300);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (entry), FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 2, 3);
  gtk_widget_show (entry);

  button = gimp_prop_check_button_new (config, "fixed-aspect",
                                       _("Aspect"));
  gtk_widget_show (button);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 3, 4);
  spinbutton = gimp_prop_spin_button_new (config, "aspect", 0.01, 0.1, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 1, 2, 3, 4);
  gtk_widget_show (spinbutton);

  button = gimp_prop_check_button_new (config, "fixed-center",
                                       _("Center"));
  gtk_widget_show (button);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 4, 6);
  entry = gimp_prop_size_entry_new (config, "center-x", "unit", "%a",
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE, 300);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (entry), FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 4, 5);
  gtk_widget_show (entry);
  entry = gimp_prop_size_entry_new (config, "center-y", "unit", "%a",
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE, 300);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (entry), FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 5, 6);
  gtk_widget_show (entry);

  gtk_widget_show (table);

  controls_container = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), controls_container, FALSE, FALSE, 0);
  gtk_widget_show (controls_container);
  g_object_set_data (G_OBJECT (tool_options),
                     "controls-container", controls_container);

  return vbox;
}
