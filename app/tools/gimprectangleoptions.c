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

#include <string.h>

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


enum
{
  COLUMN_LEFT_NUMBER,
  COLUMN_RIGHT_NUMBER,
  COLUMN_TEXT,
  NUM_COLUMNS
};


static void     gimp_rectangle_options_iface_base_init        (GimpRectangleOptionsInterface *rectangle_options_iface);

static void     gimp_rectangle_options_fixed_rule_changed     (GtkWidget                     *combo_box,
                                                               GimpRectangleOptionsPrivate   *private);

static void     gimp_rectangle_options_string_current_updates (GimpNumberPairEntry           *entry,
                                                               GParamSpec                    *param,
                                                               GimpRectangleOptions          *rectangle_options);
static void     gimp_rectangle_options_setup_ratio_completion (GimpRectangleOptions          *rectangle_options,
                                                               GtkWidget                     *entry,
                                                               GtkListStore                  *history);

static gboolean gimp_number_pair_entry_history_select         (GtkEntryCompletion            *completion,
                                                               GtkTreeModel                  *model,
                                                               GtkTreeIter                   *iter,
                                                               GimpNumberPairEntry           *entry);

static void     gimp_number_pair_entry_history_add            (GtkWidget                     *entry,
                                                               GtkTreeModel                  *model);


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
                                           g_param_spec_double ("x",
                                                                NULL, NULL,
                                                                -GIMP_MAX_IMAGE_SIZE,
                                                                GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("y",
                                                                NULL, NULL,
                                                                -GIMP_MAX_IMAGE_SIZE,
                                                                GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("width",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("height",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

     g_object_interface_install_property (iface,
                                          gimp_param_spec_unit ("position-unit",
                                                                NULL, NULL,
                                                                TRUE, TRUE,
                                                                GIMP_UNIT_PIXEL,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

     g_object_interface_install_property (iface,
                                          gimp_param_spec_unit ("size-unit",
                                                                NULL, NULL,
                                                                TRUE, TRUE,
                                                                GIMP_UNIT_PIXEL,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("fixed-rule-active",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_enum ("fixed-rule",
                                                              NULL, NULL,
                                                              GIMP_TYPE_RECTANGLE_TOOL_FIXED_RULE,
                                                              GIMP_RECTANGLE_TOOL_FIXED_ASPECT,
                                                              GIMP_CONFIG_PARAM_FLAGS |
                                                              GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("desired-fixed-width",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                100.0,
                                                                GIMP_CONFIG_PARAM_FLAGS |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("desired-fixed-height",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                100.0,
                                                                GIMP_CONFIG_PARAM_FLAGS |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("desired-fixed-size-width",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                100.0,
                                                                GIMP_CONFIG_PARAM_FLAGS |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("desired-fixed-size-height",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                100.0,
                                                                GIMP_CONFIG_PARAM_FLAGS |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("default-fixed-size-width",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                100.0,
                                                                GIMP_PARAM_READWRITE |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("default-fixed-size-height",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                100.0,
                                                                GIMP_PARAM_READWRITE |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("overridden-fixed-size",
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
                                           g_param_spec_double ("default-aspect-numerator",
                                                                NULL, NULL,
                                                                0.001, GIMP_MAX_IMAGE_SIZE,
                                                                1.0,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("default-aspect-denominator",
                                                                NULL, NULL,
                                                                0.001, GIMP_MAX_IMAGE_SIZE,
                                                                1.0,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("overridden-fixed-aspect",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("use-string-current",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_PARAM_READWRITE |
                                                                 GIMP_PARAM_STATIC_STRINGS));

     g_object_interface_install_property (iface,
                                          gimp_param_spec_unit ("fixed-unit",
                                                                NULL, NULL,
                                                                TRUE, TRUE,
                                                                GIMP_UNIT_PIXEL,
                                                                GIMP_PARAM_READWRITE |
                                                                G_PARAM_CONSTRUCT));

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

      initialized = TRUE;
    }
}

static void
gimp_rectangle_options_private_finalize (GimpRectangleOptionsPrivate *private)
{
  g_object_unref (private->aspect_history);
  g_object_unref (private->size_history);

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

      private->aspect_history = gtk_list_store_new (NUM_COLUMNS,
                                                    G_TYPE_DOUBLE,
                                                    G_TYPE_DOUBLE,
                                                    G_TYPE_STRING);

      private->size_history = gtk_list_store_new (NUM_COLUMNS,
                                                  G_TYPE_DOUBLE,
                                                  G_TYPE_DOUBLE,
                                                  G_TYPE_STRING);

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
                                    GIMP_RECTANGLE_OPTIONS_PROP_X,
                                    "x");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_Y,
                                    "y");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_WIDTH,
                                    "width");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_HEIGHT,
                                    "height");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_POSITION_UNIT,
                                    "position-unit");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_SIZE_UNIT,
                                    "size-unit");

  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_FIXED_RULE_ACTIVE,
                                    "fixed-rule-active");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_FIXED_RULE,
                                    "fixed-rule");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_WIDTH,
                                    "desired-fixed-width");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_HEIGHT,
                                    "desired-fixed-height");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_WIDTH,
                                    "desired-fixed-size-width");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_HEIGHT,
                                    "desired-fixed-size-height");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_WIDTH,
                                    "default-fixed-size-width");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_HEIGHT,
                                    "default-fixed-size-height");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_SIZE,
                                    "overridden-fixed-size");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR,
                                    "aspect-numerator");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR,
                                    "aspect-denominator");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_NUMERATOR,
                                    "default-aspect-numerator");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_DENOMINATOR,
                                    "default-aspect-denominator");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_ASPECT,
                                    "overridden-fixed-aspect");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_USE_STRING_CURRENT,
                                    "use-string-current");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_FIXED_UNIT,
                                    "fixed-unit");

  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_FIXED_CENTER,
                                    "fixed-center");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_CENTER_X,
                                    "center-x");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_CENTER_Y,
                                    "center-y");
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

    case GIMP_RECTANGLE_OPTIONS_PROP_X:
      private->x = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_Y:
      private->y = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_WIDTH:
      private->width = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_HEIGHT:
      private->height = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_POSITION_UNIT:
      private->position_unit = g_value_get_int (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_SIZE_UNIT:
      private->size_unit = g_value_get_int (value);
      break;

    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_RULE_ACTIVE:
      private->fixed_rule_active = g_value_get_boolean (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_RULE:
      private->fixed_rule = g_value_get_enum (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_WIDTH:
      private->desired_fixed_width = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_HEIGHT:
      private->desired_fixed_height = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_WIDTH:
      private->desired_fixed_size_width = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_HEIGHT:
      private->desired_fixed_size_height = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_WIDTH:
      private->default_fixed_size_width = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_HEIGHT:
      private->default_fixed_size_height = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_SIZE:
      private->overridden_fixed_size = g_value_get_boolean (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR:
      private->aspect_numerator = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR:
      private->aspect_denominator = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_NUMERATOR:
      private->default_aspect_numerator = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_DENOMINATOR:
      private->default_aspect_denominator = g_value_get_double (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_ASPECT:
      private->overridden_fixed_aspect = g_value_get_boolean (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_USE_STRING_CURRENT:
      private->use_string_current = g_value_get_boolean (value);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_UNIT:
      private->fixed_unit = g_value_get_int (value);
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

    case GIMP_RECTANGLE_OPTIONS_PROP_X:
      g_value_set_double (value, private->x);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_Y:
      g_value_set_double (value, private->y);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_WIDTH:
      g_value_set_double (value, private->width);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_HEIGHT:
      g_value_set_double (value, private->height);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_POSITION_UNIT:
      g_value_set_int (value, private->position_unit);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_SIZE_UNIT:
      g_value_set_int (value, private->size_unit);
      break;

    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_RULE_ACTIVE:
      g_value_set_boolean (value, private->fixed_rule_active);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_RULE:
      g_value_set_enum (value, private->fixed_rule);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_WIDTH:
      g_value_set_double (value, private->desired_fixed_width);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_HEIGHT:
      g_value_set_double (value, private->desired_fixed_height);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_WIDTH:
      g_value_set_double (value, private->desired_fixed_size_width);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_HEIGHT:
      g_value_set_double (value, private->desired_fixed_size_height);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_WIDTH:
      g_value_set_double (value, private->default_fixed_size_width);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_HEIGHT:
      g_value_set_double (value, private->default_fixed_size_height);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_SIZE:
      g_value_set_boolean (value, private->overridden_fixed_size);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR:
      g_value_set_double (value, private->aspect_numerator);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR:
      g_value_set_double (value, private->aspect_denominator);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_NUMERATOR:
      g_value_set_double (value, private->default_aspect_numerator);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_DENOMINATOR:
      g_value_set_double (value, private->default_aspect_denominator);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_ASPECT:
      g_value_set_boolean (value, private->overridden_fixed_aspect);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_USE_STRING_CURRENT:
      g_value_set_boolean (value, private->use_string_current);
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_UNIT:
      g_value_set_int (value, private->fixed_unit);
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_rectangle_options_fixed_rule_changed:
 * @combo_box:
 * @private:
 *
 * Updates tool options widgets depending on current fixed rule state.
 */
static void
gimp_rectangle_options_fixed_rule_changed (GtkWidget                   *combo_box,
                                           GimpRectangleOptionsPrivate *private)
{
  /* Setup sensitivity for Width and Height entries */

  gtk_widget_set_sensitive (private->width_entry,
                            ! (private->fixed_rule_active &&
                               (private->fixed_rule ==
                                GIMP_RECTANGLE_TOOL_FIXED_WIDTH ||
                                private->fixed_rule ==
                                GIMP_RECTANGLE_TOOL_FIXED_SIZE)));

  gtk_widget_set_sensitive (private->height_entry,
                            ! (private->fixed_rule_active &&
                               (private->fixed_rule ==
                                GIMP_RECTANGLE_TOOL_FIXED_HEIGHT ||
                                private->fixed_rule ==
                                GIMP_RECTANGLE_TOOL_FIXED_SIZE)));

  /* Setup current fixed rule entries */

  gtk_widget_hide (private->fixed_width_entry);
  gtk_widget_hide (private->fixed_height_entry);
  gtk_widget_hide (private->fixed_aspect_hbox);
  gtk_widget_hide (private->fixed_size_hbox);

  switch (private->fixed_rule)
    {
    case GIMP_RECTANGLE_TOOL_FIXED_ASPECT:
      gtk_widget_show (private->fixed_aspect_hbox);
      break;

    case GIMP_RECTANGLE_TOOL_FIXED_WIDTH:
      gtk_widget_show (private->fixed_width_entry);
      break;

    case GIMP_RECTANGLE_TOOL_FIXED_HEIGHT:
      gtk_widget_show (private->fixed_height_entry);
      break;

    case GIMP_RECTANGLE_TOOL_FIXED_SIZE:
      gtk_widget_show (private->fixed_size_hbox);
      break;
    }
}

static void
gimp_rectangle_options_string_current_updates (GimpNumberPairEntry  *entry,
                                               GParamSpec           *param,
                                               GimpRectangleOptions *rectangle_options)
{
  GimpRectangleOptionsPrivate *private;
  gboolean                     user_override;

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (rectangle_options);

  user_override = gimp_number_pair_entry_get_user_override (entry);

  gimp_number_pair_entry_set_default_text (entry,
                                           private->use_string_current ?
                                           _("Current") : NULL);

  gtk_widget_set_sensitive (private->aspect_button_box,
                            ! private->use_string_current || user_override);
}

GtkWidget *
gimp_rectangle_options_gui (GimpToolOptions *tool_options)
{
  GimpRectangleOptionsPrivate *private;
  GObject                     *config = G_OBJECT (tool_options);
  GtkWidget                   *vbox   = gimp_tool_options_gui (tool_options);
  GtkWidget                   *button;
  GtkWidget                   *combo;
  GtkWidget                   *table;
  gint                         row = 0;

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (tool_options);

  /* Fixed Center */
  button = gimp_prop_check_button_new (config, "fixed-center",
                                       _("Expand from center"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* Rectangle fixed-rules (e.g. aspect or width). */
  {
    GtkWidget    *frame;
    GtkWidget    *vbox2;
    GtkWidget    *hbox;
    GtkWidget    *entry;
    GtkSizeGroup *size_group;
    GList        *children;

    frame = gimp_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show (frame);

    /* Setup frame title widgets */

    hbox = gtk_hbox_new (FALSE, 2);
    gtk_frame_set_label_widget (GTK_FRAME (frame), hbox);
    gtk_widget_show (hbox);

    button = gimp_prop_check_button_new (config, "fixed-rule-active",
                                         _("Fixed:"));
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
    gtk_widget_show (button);

    g_signal_connect (button, "toggled",
                      G_CALLBACK (gimp_rectangle_options_fixed_rule_changed),
                      private);

    combo = gimp_prop_enum_combo_box_new (config, "fixed-rule", 0, 0);
    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
    gtk_widget_show (combo);

    g_signal_connect (combo, "changed",
                      G_CALLBACK (gimp_rectangle_options_fixed_rule_changed),
                      private);

    /* Setup frame content */

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (frame), vbox2);
    gtk_widget_show (vbox2);

    size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

    /* Fixed aspect entry/buttons */
    private->fixed_aspect_hbox = gtk_hbox_new (FALSE, 2);
    gtk_box_pack_start (GTK_BOX (vbox2), private->fixed_aspect_hbox,
                        FALSE, FALSE, 0);
    gtk_size_group_add_widget (size_group, private->fixed_aspect_hbox);
    g_object_unref (size_group);
    /* don't show */

    entry = gimp_prop_number_pair_entry_new (config,
                                             "aspect-numerator",
                                             "aspect-denominator",
                                             "default-aspect-numerator",
                                             "default-aspect-denominator",
                                             "overridden-fixed-aspect",
                                             FALSE, TRUE,
                                             ":/",
                                             TRUE,
                                             0.001, GIMP_MAX_IMAGE_SIZE);
    gtk_box_pack_start (GTK_BOX (private->fixed_aspect_hbox), entry,
                        TRUE, TRUE, 0);
    gtk_widget_show (entry);

    g_signal_connect (entry, "notify::user-override",
                      G_CALLBACK (gimp_rectangle_options_string_current_updates),
                      config);
    g_signal_connect_swapped (config, "notify::use-string-current",
                              G_CALLBACK (gimp_rectangle_options_string_current_updates),
                              entry);

    gimp_rectangle_options_setup_ratio_completion (GIMP_RECTANGLE_OPTIONS (tool_options),
                                                   entry,
                                                   private->aspect_history);

    private->aspect_button_box =
      gimp_prop_enum_stock_box_new (G_OBJECT (entry),
                                    "aspect", "gimp", -1, -1);
    gtk_box_pack_start (GTK_BOX (private->fixed_aspect_hbox),
                        private->aspect_button_box, FALSE, FALSE, 0);
    gtk_widget_show (private->aspect_button_box);

    /* hide "square" */
    children =
      gtk_container_get_children (GTK_CONTAINER (private->aspect_button_box));
    gtk_widget_hide (children->data);
    g_list_free (children);

    /* Fixed width entry */
    private->fixed_width_entry =
      gimp_prop_size_entry_new (config,
                                "desired-fixed-width", TRUE, "fixed-unit", "%a",
                                GIMP_SIZE_ENTRY_UPDATE_SIZE, 300);
    gtk_box_pack_start (GTK_BOX (vbox2), private->fixed_width_entry,
                        FALSE, FALSE, 0);
    gtk_size_group_add_widget (size_group, private->fixed_width_entry);
    /* don't show */

    /* Fixed height entry */
    private->fixed_height_entry =
      gimp_prop_size_entry_new (config,
                                "desired-fixed-height", TRUE, "fixed-unit", "%a",
                                GIMP_SIZE_ENTRY_UPDATE_SIZE, 300);
    gtk_box_pack_start (GTK_BOX (vbox2), private->fixed_height_entry,
                        FALSE, FALSE, 0);
    gtk_size_group_add_widget (size_group, private->fixed_height_entry);
    /* don't show */

    /* Fixed size entry */
    private->fixed_size_hbox = gtk_hbox_new (FALSE, 2);
    gtk_box_pack_start (GTK_BOX (vbox2), private->fixed_size_hbox,
                        FALSE, FALSE, 0);
    gtk_size_group_add_widget (size_group, private->fixed_size_hbox);
    /* don't show */

    entry = gimp_prop_number_pair_entry_new (config,
                                             "desired-fixed-size-width",
                                             "desired-fixed-size-height",
                                             "default-fixed-size-width",
                                             "default-fixed-size-height",
                                             "overridden-fixed-size",
                                             TRUE, FALSE,
                                             "xX*",
                                             FALSE,
                                             1, GIMP_MAX_IMAGE_SIZE);
    gtk_box_pack_start (GTK_BOX (private->fixed_size_hbox), entry,
                        TRUE, TRUE, 0);
    gtk_widget_show (entry);

    gimp_rectangle_options_setup_ratio_completion (GIMP_RECTANGLE_OPTIONS (tool_options),
                                                   entry,
                                                   private->size_history);

    private->size_button_box =
      gimp_prop_enum_stock_box_new (G_OBJECT (entry),
                                    "aspect", "gimp", -1, -1);
    gtk_box_pack_start (GTK_BOX (private->fixed_size_hbox),
                        private->size_button_box, FALSE, FALSE, 0);
    gtk_widget_show (private->size_button_box);

    /* hide "square" */
    children =
      gtk_container_get_children (GTK_CONTAINER (private->size_button_box));
    gtk_widget_hide (children->data);
    g_list_free (children);
  }

  /* X, Y, Width, Height table */

  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* X */
  private->x_entry = gimp_prop_size_entry_new (config,
                                               "x", TRUE,
                                               "position-unit", "%a",
                                               GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                               300);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (private->x_entry), FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("X:"), 0.0, 0.5,
                             private->x_entry, 1, TRUE);

  /* Y */
  private->y_entry = gimp_prop_size_entry_new (config,
                                               "y", TRUE,
                                               "position-unit", "%a",
                                               GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                               300);
#if 0
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (private->y_entry), FALSE);
#endif
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Y:"), 0.0, 0.5,
                             private->y_entry, 1, TRUE);

  /* Width */
  private->width_entry = gimp_prop_size_entry_new (config,
                                                   "width", TRUE,
                                                   "size-unit", "%a",
                                                   GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                                   300);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (private->width_entry),
                                  FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row,
                             _("W:"), 0.0, 0.5,
                             private->width_entry, 1, TRUE);
  row++;

  /* Height */
  private->height_entry = gimp_prop_size_entry_new (config,
                                                    "height", TRUE,
                                                    "size-unit", "%a",
                                                    GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                                    300);
#if 0
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (private->height_entry),
                                  FALSE);
#endif
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row,
                             _("H:"), 0.0, 0.5,
                             private->height_entry, 1, TRUE);
  row++;

  /*  Highlight  */
  button = gimp_prop_check_button_new (config, "highlight",
                                       _("Highlight"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  Guide  */
  combo = gimp_prop_enum_combo_box_new (config, "guide", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  /*  Auto Shrink  */
  private->auto_shrink_button =
    gtk_button_new_with_label (_("Auto Shrink Selection"));
  gtk_box_pack_start (GTK_BOX (vbox), private->auto_shrink_button,
                      FALSE, FALSE, 0);
  gtk_widget_set_sensitive (private->auto_shrink_button, FALSE);
  gtk_widget_show (private->auto_shrink_button);

  button = gimp_prop_check_button_new (config, "shrink-merged",
                                       _("Shrink merged"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* Setup initial fixed rule widgets */
  gimp_rectangle_options_fixed_rule_changed (NULL, private);

  return vbox;
}

/**
 * gimp_rectangle_options_fixed_rule_active:
 * @rectangle_options:
 * @fixed_rule:
 *
 * Return value: %TRUE if @fixed_rule is active, %FALSE otherwise.
 */
gboolean
gimp_rectangle_options_fixed_rule_active (GimpRectangleOptions       *rectangle_options,
                                          GimpRectangleToolFixedRule  fixed_rule)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (rectangle_options), FALSE);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (rectangle_options);

  return private->fixed_rule_active &&
         private->fixed_rule == fixed_rule;
}

static void
gimp_rectangle_options_setup_ratio_completion (GimpRectangleOptions *rectangle_options,
                                               GtkWidget            *entry,
                                               GtkListStore         *history)
{
  GimpRectangleOptionsPrivate *private;
  GtkEntryCompletion          *completion;

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (rectangle_options);

  completion = g_object_new (GTK_TYPE_ENTRY_COMPLETION,
                             "model", history,
                             "inline-completion", TRUE,
                             NULL);

  gtk_entry_completion_set_text_column (completion, COLUMN_TEXT);
  gtk_entry_set_completion (GTK_ENTRY (entry), completion);
  g_object_unref (completion);

  g_signal_connect (entry, "ratio-changed",
                    G_CALLBACK (gimp_number_pair_entry_history_add),
                    history);

  g_signal_connect (completion, "match-selected",
                    G_CALLBACK (gimp_number_pair_entry_history_select),
                    entry);
}

static gboolean
gimp_number_pair_entry_history_select (GtkEntryCompletion  *completion,
                                       GtkTreeModel        *model,
                                       GtkTreeIter         *iter,
                                       GimpNumberPairEntry *entry)
{
  gdouble left_number;
  gdouble right_number;

  gtk_tree_model_get (model, iter,
                      COLUMN_LEFT_NUMBER,  &left_number,
                      COLUMN_RIGHT_NUMBER, &right_number,
                      -1);

  gimp_number_pair_entry_set_values (entry, left_number, right_number);

  return TRUE;
}

static void
gimp_number_pair_entry_history_add (GtkWidget    *entry,
                                    GtkTreeModel *model)
{
  GValue               value = { 0, };
  GtkTreeIter          iter;
  gboolean             iter_valid;
  gdouble              left_number;
  gdouble              right_number;
  const gchar         *text;

  text = gtk_entry_get_text (GTK_ENTRY (entry));
  gimp_number_pair_entry_get_values (GIMP_NUMBER_PAIR_ENTRY (entry),
                                     &left_number,
                                     &right_number);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gtk_tree_model_get_value (model, &iter, COLUMN_TEXT, &value);

      if (strcmp (text, g_value_get_string (&value)) == 0)
        {
          g_value_unset (&value);
          break;
        }

      g_value_unset (&value);
    }

  if (iter_valid)
    {
      gtk_list_store_move_after (GTK_LIST_STORE (model), &iter, NULL);
    }
  else
    {
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          COLUMN_LEFT_NUMBER,  left_number,
                          COLUMN_RIGHT_NUMBER, right_number,
                          COLUMN_TEXT,        text,
                          -1);

      /* FIXME: limit the size of the history */
    }
}
