/* GIMP - The GNU Image Manipulation Program
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

#include "core/gimptooloptions.h"

#include "widgets/gimppropwidgets.h"

#include "gimprectangleoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


static void gimp_rectangle_options_iface_base_init (GimpRectangleOptionsInterface *rectangle_options_iface);


GType
gimp_rectangle_options_interface_get_type (void)
{
  static GType iface_type = 0;

  if (! iface_type)
    {
      const GTypeInfo iface_info =
      {
        sizeof (GimpRectangleOptionsInterface),
        (GBaseInitFunc)     gimp_rectangle_options_iface_base_init,
        (GBaseFinalizeFunc) NULL,
      };

      iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                           "GimpRectangleOptionsInterface",
                                           &iface_info, 0);

      g_type_interface_add_prerequisite (iface_type, GIMP_TYPE_TOOL_OPTIONS);
    }

  return iface_type;
}

static void
gimp_rectangle_options_iface_base_init (GimpRectangleOptionsInterface *iface)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("auto-shrink",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("shrink-merged",
                                                                 NULL,
                                                                 N_("Use all visible layers when shrinking "
                                                                    "the selection"),
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("highlight",
                                                                 NULL, NULL,
                                                                 TRUE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_enum ("guide",
                                                              NULL, NULL,
                                                              GIMP_TYPE_RECTANGLE_GUIDE,
                                                              GIMP_RECTANGLE_GUIDE_NONE,
                                                              GIMP_CONFIG_PARAM_FLAGS |
                                                              GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("x0",
                                                                NULL, NULL,
                                                                -GIMP_MAX_IMAGE_SIZE,
                                                                GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("y0",
                                                                NULL, NULL,
                                                                -GIMP_MAX_IMAGE_SIZE,
                                                                GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("fixed-width",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("width",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("fixed-height",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("height",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("fixed-aspect",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("aspect-numerator",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                1.0,
                                                                GIMP_CONFIG_PARAM_FLAGS |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("aspect-denominator",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                1.0,
                                                                GIMP_CONFIG_PARAM_FLAGS |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("fixed-center",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("center-x",
                                                                NULL, NULL,
                                                                -GIMP_MAX_IMAGE_SIZE,
                                                                GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_CONFIG_PARAM_FLAGS |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("center-y",
                                                                NULL, NULL,
                                                                -GIMP_MAX_IMAGE_SIZE,
                                                                GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

     g_object_interface_install_property (iface,
                                          gimp_param_spec_unit ("unit",
                                                                NULL, NULL,
                                                                TRUE, TRUE,
                                                                GIMP_UNIT_PIXEL,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

      initialized = TRUE;
    }
}

static void
gimp_rectangle_options_private_finalize (GimpRectangleOptionsPrivate *private)
{
  g_slice_free (GimpRectangleOptionsPrivate, private);
}

GimpRectangleOptionsPrivate *
gimp_rectangle_options_get_private (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  static GQuark private_key = 0;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("gimp-rectangle-options-private");

  private = g_object_get_qdata (G_OBJECT (options), private_key);

  if (! private)
    {
      private = g_slice_new0 (GimpRectangleOptionsPrivate);

      g_object_set_qdata_full (G_OBJECT (options), private_key, private,
                               (GDestroyNotify) gimp_rectangle_options_private_finalize);
    }

  return private;
}

/**
 * gimp_rectangle_options_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #GimpRectangleOptions. A #GimpRectangleOptionsProp property is installed
 * for each property, using the values from the #GimpRectangleOptionsProp
 * enumeration. The caller must make sure itself that the enumeration
 * values don't collide with some other property values they
 * are using (that's what %GIMP_RECTANGLE_OPTIONS_PROP_LAST is good for).
 **/
void
gimp_rectangle_options_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_AUTO_SHRINK,
                                    "auto-shrink");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_SHRINK_MERGED,
                                    "shrink-merged");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT,
                                    "highlight");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_GUIDE,
                                    "guide");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_X0,
                                    "x0");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_Y0,
                                    "y0");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_FIXED_WIDTH,
                                    "fixed-width");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_WIDTH,
                                    "width");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_FIXED_HEIGHT,
                                    "fixed-height");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_HEIGHT,
                                    "height");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_FIXED_ASPECT,
                                    "fixed-aspect");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR,
                                    "aspect-numerator");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR,
                                    "aspect-denominator");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_FIXED_CENTER,
                                    "fixed-center");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_CENTER_X,
                                    "center-x");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_CENTER_Y,
                                    "center-y");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_UNIT,
                                    "unit");
}

void
gimp_rectangle_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpRectangleOptions        *options  = GIMP_RECTANGLE_OPTIONS (object);
  GimpRectangleOptionsPrivate *private;

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  switch (property_id)
    {
    case GIMP_RECTANGLE_OPTIONS_PROP_AUTO_SHRINK:
      private->auto_shrink = g_value_get_boolean (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_SHRINK_MERGED:
      private->shrink_merged = g_value_get_boolean (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT:
      private->highlight = g_value_get_boolean (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_GUIDE:
      private->guide = g_value_get_enum (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_X0:
      private->x0 = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_Y0:
      private->y0 = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_WIDTH:
      private->fixed_width = g_value_get_boolean (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_WIDTH:
      private->width = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_HEIGHT:
      private->fixed_height = g_value_get_boolean (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_HEIGHT:
      private->height = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_ASPECT:
      private->fixed_aspect = g_value_get_boolean (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR:
      private->aspect_numerator = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR:
      private->aspect_denominator = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_CENTER:
      private->fixed_center = g_value_get_boolean (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_CENTER_X:
      private->center_x = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_CENTER_Y:
      private->center_y = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_UNIT:
      private->unit = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_rectangle_options_get_property (GObject      *object,
                                     guint         property_id,
                                     GValue       *value,
                                     GParamSpec   *pspec)
{
  GimpRectangleOptions        *options  = GIMP_RECTANGLE_OPTIONS (object);
  GimpRectangleOptionsPrivate *private;

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  switch (property_id)
    {
    case GIMP_RECTANGLE_OPTIONS_PROP_AUTO_SHRINK:
      g_value_set_boolean (value, private->auto_shrink);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_SHRINK_MERGED:
      g_value_set_boolean (value, private->shrink_merged);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT:
      g_value_set_boolean (value, private->highlight);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_GUIDE:
      g_value_set_enum (value, private->guide);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_X0:
      g_value_set_double (value, private->x0);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_Y0:
      g_value_set_double (value, private->y0);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_WIDTH:
      g_value_set_boolean (value, private->fixed_width);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_WIDTH:
      g_value_set_double (value, private->width);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_HEIGHT:
      g_value_set_boolean (value, private->fixed_height);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_HEIGHT:
      g_value_set_double (value, private->height);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_ASPECT:
      g_value_set_boolean (value, private->fixed_aspect);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR:
      g_value_set_double (value, private->aspect_numerator);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR:
      g_value_set_double (value, private->aspect_denominator);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_CENTER:
      g_value_set_boolean (value, private->fixed_center);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_CENTER_X:
      g_value_set_double (value, private->center_x);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_CENTER_Y:
      g_value_set_double (value, private->center_y);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_UNIT:
      g_value_set_int (value, private->unit);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_rectangle_options_notify_aspect (GtkWidget            *widget,
                                      GParamSpec           *param_spec,
                                      GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  if (private->fixed_aspect)
    {
      g_object_set (options,
                    "width",  private->height,
                    "height", private->width,
                    NULL);
    }
}

GtkWidget *
gimp_rectangle_options_gui (GimpToolOptions *tool_options)
{
  GimpRectangleOptionsPrivate *private;

  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_tool_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *combo;
  GtkWidget *table;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *aspect;
  GList     *children;
  gint       row = 0;

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (tool_options);

  /* Fixed Center */
  button = gimp_prop_check_button_new (config, "fixed-center",
                                       _("Expand from center"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* Aspect */
  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  button = gimp_prop_check_button_new (config, "fixed-aspect",
                                       _("Fixed aspect ratio"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), button);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  entry = gimp_prop_aspect_ratio_new (config,
                                      "aspect-numerator",
                                      "aspect-denominator",
                                      "fixed-aspect");
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  aspect = gimp_prop_enum_stock_box_new (G_OBJECT (entry),
                                         "aspect", "gimp", -1, -1);
  gtk_box_pack_start (GTK_BOX (hbox), aspect, FALSE, FALSE, 0);
  gtk_widget_show (aspect);

  /* hide "square" */
  children = gtk_container_get_children (GTK_CONTAINER (aspect));
  gtk_widget_hide (children->data);
  g_list_free (children);

  g_signal_connect (entry, "notify::aspect",
                    G_CALLBACK (gimp_rectangle_options_notify_aspect),
                    config);

  /*  Highlight  */
  button = gimp_prop_check_button_new (config, "highlight",
                                       _("Highlight"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  table = gtk_table_new (4, 6, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* X */
  entry = gimp_prop_size_entry_new (config, "x0", "unit", "%a",
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE, 300);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (entry), FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("X:"), 0.0, 0.5,
                             entry, 1, FALSE);

  /* Y */
  entry = gimp_prop_size_entry_new (config, "y0", "unit", "%a",
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE, 300);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (entry), FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Y:"), 0.0, 0.5,
                             entry, 1, FALSE);

  /* Width */
  entry = gimp_prop_size_entry_new (config, "width", "unit", "%a",
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE, 300);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (entry), FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row,
                             _("Width:"), 0.0, 0.5,
                             entry, 1, FALSE);

  button = gimp_prop_check_button_new (config, "fixed-width", _("Fix"));
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_widget_show (button);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 3, row, row + 1);
  row++;

  /* Height */
  entry = gimp_prop_size_entry_new (config, "height", "unit", "%a",
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE, 300);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (entry), FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row,
                             _("Height:"), 0.0, 0.5,
                             entry, 1, FALSE);

  button = gimp_prop_check_button_new (config, "fixed-height", _("Fix"));
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_widget_show (button);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 2, 3, row, row + 1);
  row++;

  /*  Guide  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  combo = gimp_prop_enum_combo_box_new (config, "guide", 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  /*  Auto Shrink  */
  button = gtk_button_new_with_label (_("Auto Shrink Selection"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, FALSE);
  gtk_widget_show (button);

  private->auto_shrink_button = button;

  button = gimp_prop_check_button_new (config, "shrink-merged",
                                       _("Shrink merged"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  return vbox;
}
