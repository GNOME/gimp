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

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpwidgets-utils.h"

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


static void   gimp_rectangle_options_class_init (GimpRectangleOptionsClass *options_class);

static void   gimp_rectangle_options_set_property (GObject         *object,
                                                   guint            property_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);
static void   gimp_rectangle_options_get_property (GObject         *object,
                                                   guint            property_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);


static GimpToolOptionsClass *parent_class = NULL;


GType
gimp_rectangle_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpRectangleOptionsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_rectangle_options_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpRectangleOptions),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) NULL
      };

      type = g_type_register_static (GIMP_TYPE_SELECTION_OPTIONS,
                                     "GimpRectangleOptions",
                                     &info, 0);
    }

  return type;
}

static void
gimp_rectangle_options_class_init (GimpRectangleOptionsClass *klass)
{
  GObjectClass         *object_class  = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_rectangle_options_set_property;
  object_class->get_property = gimp_rectangle_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_HIGHLIGHT,
                                    "highlight",
                                    N_("Highlight rectangle"),
                                    TRUE,
                                    0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FIXED_WIDTH,
                                    "fixed-width", N_("Fixed width"),
                                    FALSE, 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_WIDTH,
                                   "width", N_("Width"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FIXED_HEIGHT,
                                    "fixed-height", N_("Fixed height"),
                                    FALSE, 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_HEIGHT,
                                   "height", N_("Height"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FIXED_ASPECT,
                                    "fixed-aspect", N_("Fixed aspect"),
                                    FALSE, 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_ASPECT,
                                   "aspect", N_("Aspect"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FIXED_CENTER,
                                    "fixed-center", N_("Fixed center"),
                                    FALSE, 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_CENTER_X,
                                   "center-x", N_("Center X"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_CENTER_Y,
                                   "center-y", N_("Center Y"),
                                   0.0, GIMP_MAX_IMAGE_SIZE, 1.0,
                                   0);

  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_UNIT,
                                    "unit", NULL,
                                    TRUE, FALSE, GIMP_UNIT_PIXEL, 0);
}

static void
gimp_rectangle_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpRectangleOptions *options = GIMP_RECTANGLE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_HIGHLIGHT:
      options->highlight = g_value_get_boolean (value);
      break;
    case PROP_FIXED_WIDTH:
      options->fixed_width = g_value_get_boolean (value);
      break;
    case PROP_WIDTH:
      options->width = g_value_get_double (value);
      break;
    case PROP_FIXED_HEIGHT:
      options->fixed_height = g_value_get_boolean (value);
      break;
    case PROP_HEIGHT:
      options->height = g_value_get_double (value);
      break;
    case PROP_FIXED_ASPECT:
      options->fixed_aspect = g_value_get_boolean (value);
      break;
    case PROP_ASPECT:
      options->aspect = g_value_get_double (value);
      break;
    case PROP_FIXED_CENTER:
      options->fixed_center = g_value_get_boolean (value);
      break;
    case PROP_CENTER_X:
      options->center_x = g_value_get_double (value);
      break;
    case PROP_CENTER_Y:
      options->center_y = g_value_get_double (value);
      break;
    case PROP_UNIT:
      options->unit = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_rectangle_options_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpRectangleOptions *options = GIMP_RECTANGLE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_HIGHLIGHT:
      g_value_set_boolean (value, options->highlight);
      break;
    case PROP_FIXED_WIDTH:
      g_value_set_boolean (value, options->fixed_width);
      break;
    case PROP_WIDTH:
      g_value_set_double (value, options->width);
      break;
    case PROP_FIXED_HEIGHT:
      g_value_set_boolean (value, options->fixed_height);
      break;
    case PROP_HEIGHT:
      g_value_set_double (value, options->height);
      break;
    case PROP_FIXED_ASPECT:
      g_value_set_boolean (value, options->fixed_aspect);
      break;
    case PROP_ASPECT:
      g_value_set_double (value, options->aspect);
      break;
    case PROP_FIXED_CENTER:
      g_value_set_boolean (value, options->fixed_center);
      break;
    case PROP_CENTER_X:
      g_value_set_double (value, options->center_x);
      break;
    case PROP_CENTER_Y:
      g_value_set_double (value, options->center_y);
      break;
    case PROP_UNIT:
      g_value_set_int (value, options->unit);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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

  vbox = gimp_selection_options_gui (tool_options);

  /*  the highlight toggle button  */
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
